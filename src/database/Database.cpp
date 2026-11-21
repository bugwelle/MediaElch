#include "Database.h"

#include "data/Subtitle.h"
#include "database/SqliteTransaction.h"
#include "globals/Helper.h"
#include "globals/Manager.h"
#include "log/Log.h"
#include "settings/Settings.h"

#include <QDateTime>
#include <QDir>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

using namespace mediaelch;

/// \brief This mutex is used for initializing new connections.
static QMutex s_initializingDatabaseMutex;
/// \brief Used for creating a new connection name.
static size_t s_connectionCount = 0;

Database::Database(QObject* parent) : Database(QString(), parent)
{
}

Database::Database(const QString& databaseName, QObject* parent) : QObject(parent)
{
    // This lock is required to ensure that multithreaded access only initializes
    // the database once.  Each instance of this class has its own connection name.
    QMutexLocker lock(&s_initializingDatabaseMutex);
    ++s_connectionCount;

    QString dbName = databaseName;
    if (dbName.isEmpty()) {
        m_dataLocation = Settings::instance()->databaseDir();
        QDir dir(m_dataLocation.dir());
        if (!dir.exists()) {
            dir.mkpath(m_dataLocation.toString());
        }
        dbName = m_dataLocation.filePath("MediaElch.sqlite");
    }

    const QString connectionName = QStringLiteral("mediaDb_%1").arg(s_connectionCount);
    m_db = std::make_unique<QSqlDatabase>(QSqlDatabase::addDatabase("QSQLITE", connectionName));
    m_db->setDatabaseName(dbName);

    if (!m_db->open()) {
        qCCritical(generic) << "Could not open cache database";
    } else if (!setupDatabase()) {
        qCCritical(generic) << "[Database] Initialization failed:" << m_initializationError;
        m_db->close();
    }
}

Database::~Database()
{
    if (m_db != nullptr && m_db->isOpen()) {
        m_db->close();
    }
}

Database* Database::newConnection(QObject* parent)
{
    return new Database(parent);
}

namespace {

bool executeSqlStatementsFromFile(const QString& filePath, QSqlQuery& query, QString& error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        error = "Could not open migration: " + filePath;
        return false;
    }
    const auto statements = QString::fromUtf8(file.readAll()).split("-- next", ElchSplitBehavior::SkipEmptyParts);
    for (const auto& statement : statements) {
        if (!query.exec(statement.trimmed())) {
            error = filePath + ": " + query.lastError().text();
            return false;
        }
    }
    return true;
}

} // namespace

bool Database::updateDbVersion(int version)
{
    QSqlQuery query(*m_db);
    query.prepare("INSERT OR REPLACE INTO settings(idSettings, value) VALUES(1, :version)");
    query.bindValue(":version", QString::number(version));
    if (!query.exec()) {
        m_initializationError = query.lastError().text();
        return false;
    }
    return true;
}

QSqlDatabase Database::db()
{
    return *m_db;
}

void Database::addImport(QString fileName, QString type, DirectoryPath path)
{
    int id = 1;
    QSqlQuery query(db());
    query.prepare("SELECT MAX(id) FROM importCache");
    query.exec();
    if (query.next()) {
        id = query.value(0).toInt() + 1;
    }

    query.prepare("INSERT INTO importCache(id, filename, type, path) VALUES(:id, :filename, :type, :path)");
    query.bindValue(":id", id);
    query.bindValue(":filename", fileName);
    query.bindValue(":type", type);
    query.bindValue(":path", path.toString());
    query.exec();
}

bool Database::guessImport(QString fileName, QString& type, QString& path)
{
    qreal bestMatch = 0;

    QSqlQuery query(db());
    query.prepare("SELECT filename, type, path FROM importCache");
    query.exec();
    while (query.next()) {
        qreal p = helper::similarity(fileName, query.value(query.record().indexOf("filename")).toString());
        if (p > 0.7 && p > bestMatch) {
            bestMatch = p;
            type = query.value(query.record().indexOf("type")).toString();
            path = query.value(query.record().indexOf("path")).toString();
        }
    }

    return (bestMatch != 0);
}

void Database::setLabel(const FileList& fileNames, ColorLabel colorLabel)
{
    // no locker, as this function is called by add()

    int color = static_cast<int>(colorLabel);
    QSqlQuery query(db());
    for (const FilePath& fileName : fileNames) {
        query.prepare("SELECT idLabel FROM labels WHERE fileName=:fileName");
        query.bindValue(":fileName", fileName.toString());
        query.exec();
        if (query.next()) {
            int idLabel = query.value(query.record().indexOf("idLabel")).toInt();
            query.prepare("UPDATE labels SET color=:color WHERE idLabel=:idLabel");
            query.bindValue(":idLabel", idLabel);
            query.bindValue(":color", color);
            query.exec();
        } else {
            query.prepare("INSERT INTO labels(color, fileName) VALUES(:color, :fileName)");
            query.bindValue(":color", color);
            query.bindValue(":fileName", fileName.toString());
            query.exec();
        }
    }
}

ColorLabel Database::getLabel(const mediaelch::FileList& fileNames)
{
    if (fileNames.isEmpty()) {
        return ColorLabel::NoLabel;
    }

    QSqlQuery query(db());
    query.prepare("SELECT color FROM labels WHERE fileName=:fileName");
    query.bindValue(":fileName", fileNames.first().toString());
    bool success = query.exec();
    if (success && query.next()) {
        return static_cast<ColorLabel>(query.value("color").toInt());
    }

    return ColorLabel::NoLabel;
}

QString Database::getSchema()
{
    QSqlQuery query(*m_db);
    query.exec("select sql from sqlite_schema;");
    QStringList tables;
    while (query.next()) {
        tables << query.value(0).toString();
    }
    return tables.join(";\n\n");
}

bool Database::normalizeMovieTimestamps()
{
    QSqlQuery query(*m_db);
    if (!query.exec("SELECT idMovie,lastModified FROM movies")) {
        m_initializationError = query.lastError().text();
        return false;
    }
    QVector<QPair<qlonglong, qlonglong>> timestamps;
    while (query.next()) {
        bool numeric = false;
        qlonglong millis = query.value(1).toLongLong(&numeric);
        if (!numeric) {
            const auto date = query.value(1).toDateTime();
            if (!date.isValid()) {
                m_initializationError = "Invalid legacy movie timestamp";
                return false;
            }
            millis = date.toMSecsSinceEpoch();
        }
        timestamps.append(qMakePair(query.value(0).toLongLong(), millis));
    }
    if (query.lastError().isValid()) {
        m_initializationError = query.lastError().text();
        return false;
    }
    query.finish();
    for (const auto& timestamp : timestamps) {
        query.prepare("UPDATE movies SET lastModified=:time WHERE idMovie=:id");
        query.bindValue(":time", timestamp.second);
        query.bindValue(":id", timestamp.first);
        if (!query.exec()) {
            m_initializationError = query.lastError().text();
            return false;
        }
    }
    return true;
}

int Database::schemaVersion()
{
    return 19;
}

bool Database::setupDatabase()
{
    const int latestVersion = schemaVersion();
    // Apply durability before migrations can write persistent preferences.
    QSqlQuery durability(*m_db);
    if (!durability.exec("PRAGMA synchronous=FULL")) {
        m_initializationError = durability.lastError().text();
        return false;
    }
    durability.finish();
    SqliteTransaction transaction(*m_db);
    if (!transaction.isActive()) {
        m_initializationError = "Could not start schema transaction";
        return false;
    }
    QSqlQuery query(*m_db);
    if (!query.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'")) {
        m_initializationError = query.lastError().text();
        return false;
    }
    QStringList tables;
    while (query.next()) {
        tables << query.value(0).toString();
    }
    if (query.lastError().isValid()) {
        m_initializationError = query.lastError().text();
        return false;
    }
    int version = 0;
    if (!tables.isEmpty()) {
        if (!tables.contains("settings") || !query.exec("SELECT value FROM settings WHERE idSettings=1")
            || !query.next()) {
            m_initializationError = "Missing or unreadable database version: " + query.lastError().text();
            return false;
        }
        bool ok = false;
        version = query.value(0).toString().toInt(&ok);
        if (!ok || version < 14 || version > latestVersion) {
            m_initializationError = "Invalid or unsupported database version: " + query.value(0).toString();
            return false;
        }
    }
    query.finish();
    if (tables.isEmpty()) {
        if (!executeSqlStatementsFromFile(":/sql/014_initial.sql", query, m_initializationError)
            || !executeSqlStatementsFromFile(":/sql/000_settings.sql", query, m_initializationError)
            || !updateDbVersion(14)) {
            return false;
        }
        version = 14;
    }
    const QStringList migrations{"015_artists_albums.sql",
        "016_movieSubtitles.sql",
        "017_showEpisodes.sql",
        "018_index_fix.sql",
        "019_storage_types.sql"};
    for (int nextVersion = version + 1; nextVersion <= latestVersion; ++nextVersion) {
        if (!executeSqlStatementsFromFile(":/sql/" + migrations.at(nextVersion - 15), query, m_initializationError)
            || (nextVersion == 19 && !normalizeMovieTimestamps()) || !updateDbVersion(nextVersion)) {
            return false;
        }
    }
    query.finish();
    if (!transaction.commit()) {
        m_initializationError = "Could not commit schema migration";
        return false;
    }
    for (const auto& sql : {"PRAGMA synchronous=FULL", "PRAGMA journal_mode=WAL", "PRAGMA cache_size=20000"}) {
        if (!query.exec(sql)) {
            m_initializationError = query.lastError().text();
            return false;
        }
        query.finish();
    }
    return true;
}
