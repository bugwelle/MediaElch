#pragma once

#include "utils/Meta.h"

#include <QString>
#include <QVector>

#include <memory>
#include <string>

namespace mediaelch {
namespace renamer {
namespace impl {


/// \brief A lexer for our template strings.  Tokenizes the input string on-demand.
///
/// \details
///     This class tokenizes the input string on-demand via method consume(), which returns
///     a Token.  The EOL token is emitted when the end of the input string is reached.
///     Unknown characters emit an "Unknown" token.
///
/// \see TemplateParser
class TemplateLexer
{
public:
    enum class TokenKind
    {
        Whitespace,
        Identifier,
        Number,
        Pipe,
        Slash,
        OpenAngleBracket,
        CloseAngleBracket,
        OpenBrace,
        CloseBrace,
        OpenBracket,
        CloseBracket,
        Unknown,
        EOL,
    };

    struct Token
    {
        TokenKind kind{TokenKind::EOL};
        elch_ssize_t offset{0};
        elch_ssize_t length{0};

        bool operator==(const Token& rhs) const
        {
            return kind == rhs.kind && offset == rhs.offset && length == rhs.length;
        }
    };

    QString tokenString(const Token& token)
    { //
        return QString::fromStdString(m_data.substr(token.offset, token.length));
    }
    QString substr(elch_ssize_t start, elch_ssize_t end)
    { //
        return QString::fromStdString(m_data.substr(start, end - start));
    }

public:
    ELCH_NODISCARD static TemplateLexer create(const std::string& str);

public:
    ELCH_NODISCARD bool hasNext();
    Token consume();
    ELCH_NODISCARD Token current() { return m_currentToken; }
    ELCH_NODISCARD Token lookahead() { return m_nextToken; }

    ELCH_NODISCARD Token parseWhitespace();
    ELCH_NODISCARD Token parseNumberToken();
    ELCH_NODISCARD Token parseIdentifierToken();

private:
    TemplateLexer(const std::string& str) : m_data{str} {};
    ELCH_NODISCARD Token lexNextToken();

private:
    Token m_currentToken{};
    Token m_nextToken{};
    const std::string& m_data;
    elch_ssize_t m_startIndex{0};

#ifdef QT_DEBUG
    // Sanity check: Counter is increased each time some "next" function is called.
    std::size_t m_debug_counter{0};
#endif
};

struct PlaceholderDataProvider
{
    explicit PlaceholderDataProvider() = default;
    virtual ~PlaceholderDataProvider() = default;

    ELCH_NODISCARD virtual bool has(const QString& field) const = 0;
    ELCH_NODISCARD virtual QString data(const QString& field) const = 0;

    ELCH_NODISCARD virtual QString transformed(const QString& functionName, const QString& data) const
    {
        if (functionName == "upper") {
            return data.toUpper();
        } else if (functionName == "lower") {
            return data.toLower();
        } else {
            MediaElch_Unreachable();
        }
    }
};

struct PlaceholderVisitor
{
    explicit PlaceholderVisitor() = default;
    virtual ~PlaceholderVisitor() = default;
    virtual void visitBegin(const struct Placeholder& segment) = 0;
    virtual void visitEnd(const struct Placeholder& segment) = 0;
    virtual void visitBegin(const struct PlaceholderSourceSegment& segment) = 0;
    virtual void visitEnd(const struct PlaceholderSourceSegment& segment) = 0;
    virtual void visitBegin(const struct PlaceholderVariableSegment& segment) = 0;
    virtual void visitEnd(const struct PlaceholderVariableSegment& segment) = 0;
    virtual void visitBegin(const struct PlaceholderConditionSegment& segment) = 0;
    virtual void visitEnd(const struct PlaceholderConditionSegment& segment) = 0;
};

struct PlaceholderSegment
{
    explicit PlaceholderSegment() = default;
    virtual ~PlaceholderSegment();

    virtual void render(const PlaceholderDataProvider& data, QString& output) const = 0;
    virtual void accept(PlaceholderVisitor& visitor) const = 0;
};

struct Placeholder : public PlaceholderSegment
{
    explicit Placeholder() = default;
    Placeholder(Placeholder&&) = default;
    ~Placeholder() override = default;
    Placeholder& operator=(Placeholder&&) = default;

    void render(const PlaceholderDataProvider& data, QString& output) const override;
    void accept(PlaceholderVisitor& visitor) const override;

private:
    friend class TemplateParser;
    std::vector<std::unique_ptr<PlaceholderSegment>> m_children;
};


struct PlaceholderSourceSegment : public PlaceholderSegment
{
    explicit PlaceholderSourceSegment(QString content) : m_content{std::move(content)} {}
    ~PlaceholderSourceSegment() override = default;

    void render(const PlaceholderDataProvider& data, QString& output) const override;
    void accept(PlaceholderVisitor& visitor) const override;

private:
    QString m_content;
};

struct PlaceholderVariableSegment : public PlaceholderSegment
{
    PlaceholderVariableSegment() = default;
    ~PlaceholderVariableSegment() override = default;

    void render(const PlaceholderDataProvider& data, QString& output) const override;
    void accept(PlaceholderVisitor& visitor) const override;

private:
    friend class TemplateParser;

    struct Field
    {
        QString name;
        int index{-1};
    };
    QVector<Field> m_fields;
    QString m_function;
};

struct PlaceholderConditionSegment : public PlaceholderSegment
{
    PlaceholderConditionSegment() = default;
    ~PlaceholderConditionSegment() override = default;

    void render(const PlaceholderDataProvider& data, QString& output) const override;
    void accept(PlaceholderVisitor& visitor) const override;

private:
    friend class TemplateParser;

    PlaceholderVariableSegment m_condition{};
    Placeholder m_body{};
};


class TemplateParser
{
public:
    struct Result
    {
        std::unique_ptr<Placeholder> placeholder;
        QString error;
        ELCH_NODISCARD bool hasError() const { return !error.isEmpty(); }
    };

public:
    ELCH_NODISCARD static Result parse(const std::string& str);

private:
    using Kind = TemplateLexer::TokenKind;

    explicit TemplateParser(const std::string& str);

    ELCH_NODISCARD std::unique_ptr<Placeholder> parseRule();
    ELCH_NODISCARD std::unique_ptr<PlaceholderVariableSegment> parseVariable();
    ELCH_NODISCARD std::unique_ptr<PlaceholderConditionSegment> parseCondition();
    ELCH_NODISCARD std::unique_ptr<PlaceholderSourceSegment> parseSource();
    ELCH_NODISCARD std::unique_ptr<PlaceholderVariableSegment> parseVariableContent(Kind closingToken);

    /// \brief   Moves the lexer to the next token if it is the expected kind.
    /// \details If the next token is unexpected, it moves the cursor to the end and sets an error message.
    // void expectNext(TemplateLexer::TokenKind kind);
    // TODO: ODer Funktion errorAt(Token)?

private:
    TemplateLexer m_lexer;
};


//// \brief Pseudo-Optional class. To be replaced by proper optional.
template<class T>
struct RenamerOptional
{
    bool hasValue() { return m_hasValue; }

    explicit RenamerOptional(T value) : m_value{std::move(value)}, m_hasValue{true} {}
    explicit RenamerOptional() : m_hasValue{false} {}

private:
    T m_value{};
    bool m_hasValue{false};
};

/*

auto parsePlaceholderVariable(const std::string& templateString, elch_ssize_t offset)
    -> SegmentParseResult<PlaceholderVariable>
{
    MediaElch_Debug_Expects(templateString[offset] == '<');

    SegmentParseResult<PlaceholderVariable> segment;
    elch_ssize_t current_offset{offset + 1};
    elch_ssize_t last_start_offset{offset + 1};

    enum class State
    {
        VariableName,
        Index,
        Function,
        End,
    };
    State current_state{State::VariableName};

    const auto insert = [&](elch_ssize_t start, elch_ssize_t end) {
        switch (current_state) {
        case State::VariableName: segment.result.name = templateString.substr(last_start_offset, current_offset); break;
        case State::Index: segment.result.index = templateString.substr(last_start_offset, current_offset); break;
        case State::Function: segment.result.function = templateString.substr(last_start_offset, current_offset); break;
        }
        MediaElch_Debug_Unreachable();
    };

    const auto readUntil = [&](auto callback) {
        for (elch_ssize_t i = current_offset; i < templateString.size(); ++i) {
            if (callback(templateString[i])) {
                return;
            }
        }
    };

    const auto isEol =
        [&]() {
            MediaElch_Debug_Assert(current_offset <= templateString.size());
            return current_offset == templateString.size();
        }

    readUntil([](char c) { return c == '>' || c == '|' || c == '['; });
    if (isEol()) {
        insert();
    }

    readUntil([](char c) { return c == '>' || c == '|' });
    if (error) {}

    readUntil([](char c) { return c == '>' });
    if (error) {}

    MediaElch_Assert(current_index > offset);
    segment.length = current_index - offset;


    MediaElch_Debug_Ensures(templateString[offset + length] == '>');
}

*/


} // namespace impl
} // namespace renamer
} // namespace mediaelch
