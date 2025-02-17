#include "test/test_helpers.h"

#include <database/Database.h>

TEST_CASE("Ensure Schema is Up-To-Date", "[database]")
{
    SECTION("Get and store final SQLite schema")
    {
        auto* db = Database::newConnection(nullptr);
        QString schema = db->getSchema();

        schema = "-- GENERATED BY testSchema.cpp\n"
                 "-- DO NOT EDIT MANUALLY!\n"
                 "--\n"
                 "-- instead, introduce new migration scripts and run the\n"
                 "-- tests to update the reference file.\n\n"
                 + schema;

        test::compareStringAgainstResourceFile(schema, "../../src/database/schema/999_full_schema.sql");
    }
}
