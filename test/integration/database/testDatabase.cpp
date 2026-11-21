#include "test/integration/database/DatabaseTestHelpers.h"

#include "database/Database.h"

#include <QTemporaryDir>

using namespace mediaelch;

TEST_CASE("Database connections isolate memory and persist files", "[database]")
{
    SECTION("Memory connections do not share data")
    {
        Database first{QStringLiteral(":memory:")};
        Database second{QStringLiteral(":memory:")};
        REQUIRE(first.db().isOpen());
        REQUIRE(second.db().isOpen());
        CHECK(first.db().connectionName() != second.db().connectionName());
        first.addImport("movie.mkv", "movie", DirectoryPath(QStringLiteral("/movies")));
        CHECK(test::rowCount(first.db(), "importCache") == 1);
        CHECK(test::rowCount(second.db(), "importCache") == 0);
    }
    SECTION("File data survives closing and reopening a connection")
    {
        QTemporaryDir dir;
        REQUIRE(dir.isValid());
        const QString filename = dir.filePath("cache.sqlite");
        {
            Database first{filename};
            first.addImport("movie.mkv", "movie", DirectoryPath(dir.path()));
        }
        Database second{filename};
        CHECK(test::rowCount(second.db(), "importCache") == 1);
        QString type;
        QString path;
        REQUIRE(second.guessImport("movie.mkv", type, path));
        CHECK(type == QStringLiteral("movie"));
        CHECK(path == DirectoryPath(dir.path()).toString());
    }
    SECTION("An inaccessible database remains closed")
    {
        QTemporaryDir dir;
        REQUIRE(dir.isValid());
        Database database{dir.filePath("missing/cache.sqlite")};
        CHECK_FALSE(database.db().isOpen());
    }
}

TEST_CASE("Import history chooses the best matching filename", "[database]")
{
    Database database{QStringLiteral(":memory:")};
    QString type = "unchanged";
    QString path = "unchanged";
    CHECK_FALSE(database.guessImport("Example.Movie.2026.mkv", type, path));
    CHECK(type == QStringLiteral("unchanged"));
    CHECK(path == QStringLiteral("unchanged"));

    database.addImport("Example.Movie.2025.mkv", "concert", DirectoryPath(QStringLiteral("/concerts")));
    database.addImport("Example.Movie.2026.mkv", "movie", DirectoryPath(QStringLiteral("/movies/ä's")));
    REQUIRE(database.guessImport("Example.Movie.2026.mkv", type, path));
    CHECK(type == QStringLiteral("movie"));
    CHECK(path == DirectoryPath(QStringLiteral("/movies/ä's")).toString());
    CHECK(test::rowCount(database.db(), "importCache") == 2);
    CHECK_FALSE(database.guessImport("zzzzzzzzzzzzzzzzzzzzzzzzzzzz", type, path));
}

TEST_CASE("Labels cover every file and update without duplicate rows", "[database]")
{
    Database database{QStringLiteral(":memory:")};
    const FileList files{
        FilePath(QStringLiteral("/movies/ä's part1.mkv")), FilePath(QStringLiteral("/movies/ä's part2.mkv"))};
    CHECK(database.getLabel({}) == ColorLabel::NoLabel);
    CHECK(database.getLabel(files) == ColorLabel::NoLabel);
    const auto firstColor = ColorLabel::Red;
    const auto secondColor = ColorLabel::Orange;
    database.setLabel(files, firstColor);
    for (const auto& file : files) {
        CHECK(database.getLabel(FileList{file}) == firstColor);
    }
    database.setLabel(files, secondColor);
    CHECK(database.getLabel(files) == secondColor);
    CHECK(test::rowCount(database.db(), "labels") == 2);
    database.setLabel(files, ColorLabel::NoLabel);
    CHECK(database.getLabel(files) == ColorLabel::NoLabel);
    database.setLabel({}, firstColor);
    CHECK(test::rowCount(database.db(), "labels") == 2);
}

TEST_CASE("Destroying a database owner closes its connection", "[database]")
{
    QSqlDatabase connection;
    {
        QObject owner;
        auto* database = new Database(QStringLiteral(":memory:"), &owner);
        connection = database->db();
        REQUIRE(connection.isOpen());
        CHECK(database->parent() == &owner);
    }
    CHECK_FALSE(connection.isOpen());
}

TEST_CASE("Every file connection uses durable WAL writes", "[database]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const auto path = dir.filePath("cache.sqlite");
    Database first{path};
    Database second{path};
    for (auto* database : {&first, &second}) {
        REQUIRE(database->db().isOpen());
        QSqlQuery query(database->db());
        REQUIRE(query.exec("PRAGMA synchronous"));
        REQUIRE(query.next());
        CHECK(query.value(0).toInt() == 2);
        query.finish();
        REQUIRE(query.exec("PRAGMA journal_mode"));
        REQUIRE(query.next());
        CHECK(query.value(0).toString() == QStringLiteral("wal"));
    }
}
