#include "test/test_helpers.h"

#include "database/DatabaseId.h"

TEST_CASE("Database IDs retain validity and diagnostic representation", "[database][data]")
{
    CHECK_FALSE(mediaelch::DatabaseId().isValid());
    CHECK(mediaelch::DatabaseId().toInt() == -1);
    for (const int value : {-2, -1, 0, 1, 2147483647}) {
        CAPTURE(value);
        const mediaelch::DatabaseId id(value);
        CHECK(id.toInt() == value);
        CHECK(id.isValid() == (value >= 0));
        QString text;
        QDebug(&text) << id;
        CHECK(text.trimmed() == QStringLiteral("DbId(%1)").arg(value));
    }
}
