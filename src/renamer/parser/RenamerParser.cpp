#include "renamer/parser/RenamerParser.h"


namespace {

bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v';
}

} // namespace

namespace mediaelch {
namespace renamer {
namespace impl {

TemplateLexer TemplateLexer::create(const std::string& str)
{
    TemplateLexer lexer{str};
    Q_UNUSED(lexer.consume()); // initialize lexer with first value
    return lexer;
}

bool TemplateLexer::hasNext()
{
#ifdef QT_DEBUG
    // Sanity check: If this condition ever fails, we are in an endless loop.
    ++m_debug_counter;
    MediaElch_Debug_Assert(m_debug_counter < 5000);
#endif
    return m_nextToken.kind != TokenKind::EOL;
}

TemplateLexer::Token TemplateLexer::consume()
{
#ifdef QT_DEBUG
    // Sanity check: If this condition ever fails, we are in an endless loop.
    ++m_debug_counter;
    MediaElch_Debug_Assert(m_debug_counter < 5000);
#endif
    m_currentToken = m_nextToken;
    m_nextToken = lexNextToken();
    m_startIndex = m_nextToken.offset + m_nextToken.length;
    return m_currentToken;
}

TemplateLexer::Token TemplateLexer::lexNextToken()
{
    if (static_cast<std::size_t>(m_startIndex) >= m_data.size()) {
        return Token{TokenKind::EOL, m_startIndex, 0};
    }

    switch (m_data[m_startIndex]) {
    case '<': return Token{TokenKind::OpenAngleBracket, m_startIndex, 1};
    case '>': return Token{TokenKind::CloseAngleBracket, m_startIndex, 1};
    case '{': return Token{TokenKind::OpenBrace, m_startIndex, 1};
    case '}': return Token{TokenKind::CloseBrace, m_startIndex, 1};
    case '[': return Token{TokenKind::OpenBracket, m_startIndex, 1};
    case ']': return Token{TokenKind::CloseBracket, m_startIndex, 1};
    case '|': return Token{TokenKind::Pipe, m_startIndex, 1};
    case '/': return Token{TokenKind::Slash, m_startIndex, 1};
    }

    if (isWhitespace(m_data[m_startIndex])) { // whitespace
        return parseWhitespace();

    } else if (QChar(m_data[m_startIndex]).isDigit()) {
        return parseNumberToken();

    } else if (QChar(m_data[m_startIndex]).isLetter()) {
        return parseIdentifierToken();

    } else {
        return Token{TokenKind::Unknown, m_startIndex, 1};
    }
}

TemplateLexer::Token TemplateLexer::parseWhitespace()
{
    auto i = static_cast<std::size_t>(m_startIndex);
    while (i < m_data.size() && isWhitespace(m_data[i])) {
        ++i;
    }
    return Token{TokenKind::Whitespace, m_startIndex, static_cast<elch_ssize_t>(i) - m_startIndex};
}

TemplateLexer::Token TemplateLexer::parseNumberToken()
{
    auto i = static_cast<std::size_t>(m_startIndex);
    while (i < m_data.size() && QChar(m_data[i]).isDigit()) {
        ++i;
    }
    return Token{TokenKind::Number, m_startIndex, static_cast<elch_ssize_t>(i) - m_startIndex};
}

TemplateLexer::Token TemplateLexer::parseIdentifierToken()
{
    auto i = static_cast<std::size_t>(m_startIndex);
    while (i < m_data.size() && QChar(m_data[i]).isLetter()) {
        ++i;
    }
    return Token{TokenKind::Identifier, m_startIndex, static_cast<elch_ssize_t>(i) - m_startIndex};
}

void Placeholder::render(const PlaceholderDataProvider& data, QString& output) const
{
    for (const auto& child : m_children) {
        child->render(data, output);
    }
}

void Placeholder::accept(PlaceholderVisitor& visitor) const
{
    visitor.visitBegin(*this);
    for (const auto& child : m_children) {
        child->accept(visitor);
    }
    visitor.visitEnd(*this);
}

PlaceholderSegment::~PlaceholderSegment() = default;

void PlaceholderVariableSegment::render(const PlaceholderDataProvider& data, QString& output) const
{
    Field field = m_fields.isEmpty() ? Field{} : m_fields.first();
    QString result = data.data(field.name);
    if (!m_function.isEmpty()) {
        result = data.transformed(m_function, result);
    }
    output += result;
}

void PlaceholderVariableSegment::accept(PlaceholderVisitor& visitor) const
{
    visitor.visitBegin(*this);
    visitor.visitEnd(*this);
}

void PlaceholderSourceSegment::render(const PlaceholderDataProvider& data, QString& output) const
{
    Q_UNUSED(data)
    output += m_content;
}

void PlaceholderSourceSegment::accept(PlaceholderVisitor& visitor) const
{
    visitor.visitBegin(*this);
    visitor.visitEnd(*this);
}

void PlaceholderConditionSegment::render(const PlaceholderDataProvider& data, QString& output) const
{
    QString conditionResult;
    m_condition.render(data, conditionResult);

    if (!conditionResult.isEmpty()) {
        m_body.render(data, output);
    }
}

void PlaceholderConditionSegment::accept(PlaceholderVisitor& visitor) const
{
    visitor.visitBegin(*this);
    m_body.accept(visitor);
    visitor.visitEnd(*this);
}

TemplateParser::TemplateParser(const std::string& str) : m_lexer{TemplateLexer::create(str)}
{
}

TemplateParser::Result TemplateParser::parse(const std::string& str)
{
    TemplateParser::Result result;

    TemplateParser parser(str);
    result.placeholder = parser.parseRule();

    if (result.placeholder == nullptr) {
        result.error = "Error";
    }

    return result;
}

std::unique_ptr<Placeholder> TemplateParser::parseRule()
{
    auto result = std::make_unique<Placeholder>();

    std::unique_ptr<PlaceholderSegment> segment{nullptr};
    while (m_lexer.hasNext()) {
        TemplateLexer::Token token = m_lexer.consume();

        switch (token.kind) {
        case TemplateLexer::TokenKind::OpenAngleBracket: segment = parseVariable(); break;
        case TemplateLexer::TokenKind::OpenBrace: segment = parseCondition(); break;
        default: segment = parseSource(); break;
        }

        if (segment == nullptr) {
            return nullptr; // abort
        }

        result->m_children.push_back(std::move(segment));
        segment = nullptr;
    }

    return result;
}

std::unique_ptr<PlaceholderVariableSegment> TemplateParser::parseVariable()
{
    MediaElch_Ensures(m_lexer.current().kind == TemplateLexer::TokenKind::OpenAngleBracket);
    return parseVariableContent(TemplateLexer::TokenKind::CloseAngleBracket);
}

std::unique_ptr<PlaceholderVariableSegment> TemplateParser::parseVariableContent(Kind closingToken)
{
    auto variable = std::make_unique<PlaceholderVariableSegment>();
    QVector<PlaceholderVariableSegment::Field> m_fields;

    while (m_lexer.hasNext() && m_lexer.current().kind != closingToken) {
        TemplateLexer::Token token = m_lexer.consume();
        switch (token.kind) {
        case TemplateLexer::TokenKind::Identifier: {
            PlaceholderVariableSegment::Field field{m_lexer.tokenString(token), -1};
            m_fields.push_back(field);
            break;
        }
        default:
            // TODO: Error
            break;
        }
    }

    return variable;
}

std::unique_ptr<PlaceholderConditionSegment> TemplateParser::parseCondition()
{
    auto condition = std::make_unique<PlaceholderConditionSegment>();
    MediaElch_Ensures(m_lexer.current().kind == Kind::OpenBrace);

    auto variable = parseVariableContent(Kind::CloseBrace);
    if (variable == nullptr) {
        return nullptr;
    }
    condition->m_condition = *variable;

    if (m_lexer.lookahead().kind != Kind::CloseBrace) {
        return nullptr;
    }
    m_lexer.consume();
    auto body = parseRule();
    if (body == nullptr) {
        return nullptr;
    }
    condition->m_body = std::move(*body);
    body = nullptr;

    if (m_lexer.lookahead().kind != Kind::OpenBrace) {
        return nullptr;
    }
    m_lexer.consume();

    if (m_lexer.lookahead().kind != Kind::Slash) {
        return nullptr;
    }
    m_lexer.consume();

    // TODO: Assert equivalence

    if (m_lexer.lookahead().kind != Kind::CloseBrace) {
        return nullptr;
    }
    m_lexer.consume();

    return condition;
}

std::unique_ptr<PlaceholderSourceSegment> TemplateParser::parseSource()
{
    elch_ssize_t start = m_lexer.current().offset;

    while (m_lexer.hasNext() && m_lexer.lookahead().kind != TemplateLexer::TokenKind::OpenAngleBracket
           && m_lexer.lookahead().kind != TemplateLexer::TokenKind::OpenBrace) {
        Q_UNUSED(m_lexer.consume())
    }

    elch_ssize_t end = m_lexer.current().offset + m_lexer.current().length;
    return std::make_unique<PlaceholderSourceSegment>(m_lexer.substr(start, end));
}

} // namespace impl
} // namespace renamer
} // namespace mediaelch
