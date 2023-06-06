
// Must be included before test_helpers.h
#include "test/unit/renamer/testRenamerHelpers.h"

#include "renamer/parser/RenamerParser.h"
#include "test/test_helpers.h"

using namespace mediaelch::renamer::impl;

namespace test {
namespace {

void checkTokenList(const std::string& input, const std::vector<TemplateLexer::Token>& expected)
{
    auto tokens = TemplateLexer::create(input);
    std::size_t count{0};
    for (const auto& expectedToken : expected) {
        CAPTURE(count);
        CHECK(tokens.consume() == expectedToken);
        ++count;
    }
    CHECK(tokens.current().offset == safe_int_cast<elch_ssize_t>(input.size()));
    CHECK_FALSE(tokens.hasNext());
}

QString makeTree(const std::string& input)
{
    CAPTURE(input);
    auto result = TemplateParser::parse(input);
    REQUIRE_FALSE(result.hasError());
    test::PlaceholderTreeVisitor visitor;
    result.placeholder->accept(visitor);
    return visitor.str;
}

QString renderWithData(const PlaceholderDataProvider& provider, const QString& input)
{
    CAPTURE(input);
    auto result = TemplateParser::parse(input.toStdString());
    REQUIRE_FALSE(result.hasError());
    QString output;
    result.placeholder->render(provider, output);
    return output;
}

struct PlaceholderTests
{
    QString input;
    QString expected;
};

} // namespace
} // namespace test

TEST_CASE("Renamer Lexer", "[renamer][lexer]")
{
    using Kind = TemplateLexer::TokenKind;
    using Token = TemplateLexer::Token;


    SECTION("Tokenizes empty string")
    {
        test::checkTokenList("", {});
    }

    SECTION("current() / consume() / lookahead() work on empty strings / first token")
    {
        {
            std::string empty;
            auto tokens = TemplateLexer::create(empty);
            CHECK(tokens.current() == Token{Kind::EOL, 0, 0});
            CHECK(tokens.lookahead() == Token{Kind::EOL, 0, 0});
            CHECK(tokens.consume() == Token{Kind::EOL, 0, 0}); // moves cursor
        }
        {
            std::string input{"?"};
            auto tokens = TemplateLexer::create(input);
            CHECK(tokens.current() == Token{Kind::EOL, 0, 0});
            CHECK(tokens.lookahead() == Token{Kind::Unknown, 0, 1});
            CHECK(tokens.consume() == Token{Kind::Unknown, 0, 1}); // moves cursor
            CHECK(tokens.current() == Token{Kind::Unknown, 0, 1});
            CHECK(tokens.lookahead() == Token{Kind::EOL, 1, 0});
            CHECK(tokens.consume() == Token{Kind::EOL, 1, 0}); // moves cursor
        }
    }

    SECTION("repeated consume() return EOL token")
    {
        std::string input{"tok"};
        auto tokens = TemplateLexer::create(input);
        CHECK(tokens.hasNext());
        REQUIRE(tokens.consume() == Token{Kind::Identifier, 0, 3});
        REQUIRE(tokens.current() == Token{Kind::Identifier, 0, 3});

        for (std::size_t i = 0; i < 5; ++i) {
            REQUIRE(tokens.lookahead() == Token{Kind::EOL, 3, 0});
            REQUIRE(tokens.consume() == Token{Kind::EOL, 3, 0});
            REQUIRE_FALSE(tokens.hasNext());
        }
    }

    SECTION("Tokenizes common pattern")
    {
        test::checkTokenList("<title>-<year>{imdbId}_<imdbId>{/imdbId}  test",
            {
                Token{Kind::OpenAngleBracket, 0, 1},
                Token{Kind::Identifier, 1, 5},
                Token{Kind::CloseAngleBracket, 6, 1},
                Token{Kind::Unknown, 7, 1}, // -
                Token{Kind::OpenAngleBracket, 8, 1},
                Token{Kind::Identifier, 9, 4},
                Token{Kind::CloseAngleBracket, 13, 1},
                Token{Kind::OpenBrace, 14, 1},
                Token{Kind::Identifier, 15, 6},
                Token{Kind::CloseBrace, 21, 1},
                Token{Kind::Unknown, 22, 1},
                Token{Kind::OpenAngleBracket, 23, 1},
                Token{Kind::Identifier, 24, 6},
                Token{Kind::CloseAngleBracket, 30, 1},
                Token{Kind::OpenBrace, 31, 1},
                Token{Kind::Slash, 32, 1},
                Token{Kind::Identifier, 33, 6},
                Token{Kind::CloseBrace, 39, 1},
                Token{Kind::Whitespace, 40, 2},
                Token{Kind::Identifier, 42, 4},
                Token{Kind::EOL, 46, 0},
            });
    }

    SECTION("Tokenizes invalid patterns; errors are for parser")
    {
        test::checkTokenList("<<<&&  \n abc \tabc",
            {
                Token{Kind::OpenAngleBracket, 0, 1},
                Token{Kind::OpenAngleBracket, 1, 1},
                Token{Kind::OpenAngleBracket, 2, 1},
                Token{Kind::Unknown, 3, 1},
                Token{Kind::Unknown, 4, 1},
                Token{Kind::Whitespace, 5, 4},
                Token{Kind::Identifier, 9, 3},
                Token{Kind::Whitespace, 12, 2},
                Token{Kind::Identifier, 14, 3},
                Token{Kind::EOL, 17, 0},
            });
    }
}

TEST_CASE("Renamer Parser", "[renamer][parser]")
{
    SECTION("Parses simple strings without placeholders")
    {
        CHECK(test::makeTree("my string_is-here") == R"(
- Placeholder
  - PlaceholderSourceSegment
)");
    }

    SECTION("Parses simple conditions with placeholders")
    {
        CHECK(test::makeTree("{cond}<field>-{/cond}post") == R"(
- Placeholder
  - PlaceholderConditionSegment
    - Placeholder
      - PlaceholderVariableSegment
      - PlaceholderSourceSegment
  - PlaceholderSourceSegment
)");
    }

    SECTION("Parses nested conditions with placeholders")
    {
        CHECK(test::makeTree("my string<title>{cond}{otherCond}<field>-{/otherCond}{/cond}post") == R"(
- Placeholder
  - PlaceholderSourceSegment
  - PlaceholderVariableSegment
  - PlaceholderConditionSegment
    - Placeholder
      - PlaceholderConditionSegment
        - Placeholder
          - PlaceholderVariableSegment
          - PlaceholderSourceSegment
  - PlaceholderSourceSegment
)");
    }
}

TEST_CASE("Renamer Placeholder", "[renamer][template]")
{
    QHash<QString, QString> data;
    data.insert("field", "My Field");
    data.insert("other", "Other Field");
    data.insert("author", "Andre");
    data.insert("title", "Titel");
    data.insert("empty", "");
    test::PlaceholderTestData provider(data);

    SECTION("Correctly replaces conditions and variables with data")
    {
        QVector<test::PlaceholderTests> testCases{
            {"simple!string", "simple!string"},
            {"<field>", "My Field"},
            {"<field><other>", "My FieldOtherField"},
            {"<field>-<other>", "My Field-OtherField"},
            // all truthy conditions
            {"<title>{field}{other} Author: <author>{/other};{/field}>", "Titel Author: Andre;"},
            // inner falsey condition
            {"<title>{field}{empty} Author: <author>{/empty};{/field}>", "Titel;"},
            // output falsey condition
            {"<title>{empty}{other} Author: <author>{/other};{/empty}>", "Titel"},
            // without validation, just replaced by nothing
            {"<unknown>", ""},
        };

        for (const auto& entry : testCases) {
            CAPTURE(entry.input);
            CHECK(test::renderWithData(provider, entry.input) == entry.expected);
        }
    }
}
