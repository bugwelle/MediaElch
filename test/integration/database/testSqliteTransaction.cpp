#include "test/integration/database/DatabaseTestHelpers.h"

#include "database/Database.h"
#include "database/SqliteTransaction.h"

#include <QTemporaryDir>

using namespace mediaelch;

TEST_CASE("SQLite transaction lifetime controls persistence", "[database][transaction]")
{
    Database database{QStringLiteral(":memory:")};
    test::executeSql(database.db(), "CREATE TABLE items(value INTEGER)");
    bool committed = false;
    {
        SqliteTransaction transaction(database.db());
        QSqlQuery query(database.db());
        REQUIRE(query.prepare("INSERT INTO items VALUES(1)"));
        REQUIRE(transaction.exec(query));
        SECTION("Commit")
        {
            REQUIRE(transaction.commit());
            committed = true;
            CHECK_FALSE(transaction.isActive());
            CHECK_FALSE(transaction.commit());
        }
        SECTION("Destructor rollback")
        {
        }
        SECTION("Invalid SQL is reported")
        {
            CHECK_FALSE(query.prepare("INSERT INTO missing_table VALUES(2)"));
            CHECK_FALSE(transaction.exec(query));
            CHECK(query.lastError().isValid());
        }
    }
    CHECK(test::rowCount(database.db(), "items") == (committed ? 1 : 0));
}

TEST_CASE("Nested SQLite savepoints preserve outer writes", "[database][transaction]")
{
    Database database{QStringLiteral(":memory:")};
    test::executeSql(database.db(), "CREATE TABLE items(value INTEGER)");
    SqliteTransaction outer(database.db());
    test::executeSql(database.db(), "INSERT INTO items VALUES(1)");
    {
        SqliteTransaction inner(database.db());
        REQUIRE(inner.isActive());
        test::executeSql(database.db(), "INSERT INTO items VALUES(2)");
    }
    REQUIRE(outer.commit());
    CHECK(test::rowCount(database.db(), "items") == 1);
}

TEST_CASE("A failed SQLite commit rolls back deferred constraint violations", "[database][transaction]")
{
    Database database{QStringLiteral(":memory:")};
    test::executeSql(database.db(), "PRAGMA foreign_keys=ON");
    test::executeSql(database.db(), "CREATE TABLE parents(id INTEGER PRIMARY KEY)");
    test::executeSql(database.db(),
        "CREATE TABLE children(parent INTEGER REFERENCES parents(id) "
        "DEFERRABLE INITIALLY DEFERRED)");
    {
        SqliteTransaction transaction(database.db());
        QSqlQuery query(database.db());
        REQUIRE(query.prepare("INSERT INTO children VALUES(123)"));
        REQUIRE(transaction.exec(query));
        CHECK_FALSE(transaction.commit());
    }
    CHECK(test::rowCount(database.db(), "children") == 0);
    CHECK(database.db().transaction());
    CHECK(database.db().rollback());
}

TEST_CASE("SQLite savepoint creation reports a closed connection", "[database][transaction]")
{
    Database database{QStringLiteral(":memory:")};
    database.db().close();
    SqliteTransaction transaction(database.db());
    CHECK_FALSE(transaction.isActive());
    CHECK_FALSE(transaction.commit());
}

TEST_CASE("A read-only SQLite connection reports failed writes without changing data", "[database][transaction]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    Database database{dir.filePath("cache.sqlite")};
    test::executeSql(database.db(), "CREATE TABLE items(value INTEGER)");
    database.db().close();
    database.db().setConnectOptions("QSQLITE_OPEN_READONLY");
    REQUIRE(database.db().open());
    {
        SqliteTransaction transaction(database.db());
        REQUIRE(transaction.isActive());
        QSqlQuery query(database.db());
        REQUIRE(query.prepare("INSERT INTO items VALUES(1)"));
        CHECK_FALSE(transaction.exec(query));
        CHECK(query.lastError().isValid());
    }
    CHECK(test::rowCount(database.db(), "items") == 0);
}

TEST_CASE("A locked SQLite writer fails cleanly and can retry after the lock is released", "[database][transaction]")
{
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString path = dir.filePath("cache.sqlite");
    Database first{path};
    test::executeSql(first.db(), "CREATE TABLE items(value INTEGER)");
    Database second{path};
    test::executeSql(second.db(), "PRAGMA busy_timeout=0");
    REQUIRE(first.db().transaction());
    test::executeSql(first.db(), "INSERT INTO items VALUES(1)");
    {
        SqliteTransaction transaction(second.db());
        REQUIRE(transaction.isActive());
        QSqlQuery query(second.db());
        REQUIRE(query.prepare("INSERT INTO items VALUES(2)"));
        CHECK_FALSE(transaction.exec(query));
        CHECK(query.lastError().isValid());
    }
    REQUIRE(first.db().rollback());
    {
        SqliteTransaction transaction(second.db());
        REQUIRE(transaction.isActive());
        QSqlQuery query(second.db());
        REQUIRE(query.prepare("INSERT INTO items VALUES(2)"));
        REQUIRE(transaction.exec(query));
        REQUIRE(transaction.commit());
    }
    CHECK(test::rowCount(first.db(), "items") == 1);
}

TEST_CASE("SQLite automatic rollback removes earlier writes in the failed transaction", "[database][transaction]")
{
    Database database{QStringLiteral(":memory:")};
    test::executeSql(database.db(), "CREATE TABLE items(value INTEGER)");
    test::executeSql(database.db(),
        "CREATE TRIGGER abort_transaction BEFORE INSERT ON items WHEN NEW.value=2 "
        "BEGIN SELECT RAISE(ROLLBACK, 'test failure'); END");
    {
        SqliteTransaction transaction(database.db());
        QSqlQuery query(database.db());
        REQUIRE(query.prepare("INSERT INTO items VALUES(1)"));
        REQUIRE(transaction.exec(query));
        REQUIRE(query.prepare("INSERT INTO items VALUES(2)"));
        CHECK_FALSE(transaction.exec(query));
    }
    CHECK(test::rowCount(database.db(), "items") == 0);
    REQUIRE(database.db().transaction());
    REQUIRE(database.db().rollback());
}
