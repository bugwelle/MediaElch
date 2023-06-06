#pragma once

#include "renamer/parser/RenamerParser.h"

#include <QHash>
#include <iostream>

namespace test {

struct PlaceholderTreeVisitor : public mediaelch::renamer::impl::PlaceholderVisitor
{
    PlaceholderTreeVisitor() : str{"\n"} {}
    ~PlaceholderTreeVisitor() override = default;

    void visitBegin(const mediaelch::renamer::impl::Placeholder& segment) override
    {
        Q_UNUSED(segment)
        str += QString(depth, ' ');
        str += "- Placeholder\n";
        depth += 2;
    }

    void visitEnd(const mediaelch::renamer::impl::Placeholder& segment) override
    {
        Q_UNUSED(segment)
        depth -= 2;
    }

    void visitBegin(const mediaelch::renamer::impl::PlaceholderSourceSegment& segment) override
    {
        Q_UNUSED(segment)
        str += QString(depth, ' ');
        str += "- PlaceholderSourceSegment\n";
        depth += 2;
    }

    void visitEnd(const mediaelch::renamer::impl::PlaceholderSourceSegment& segment) override
    {
        Q_UNUSED(segment)
        depth -= 2;
    }

    void visitBegin(const mediaelch::renamer::impl::PlaceholderVariableSegment& segment) override
    {
        Q_UNUSED(segment)
        str += QString(depth, ' ');
        str += "- PlaceholderVariableSegment\n";
        depth += 2;
    }

    void visitEnd(const mediaelch::renamer::impl::PlaceholderVariableSegment& segment) override
    {
        Q_UNUSED(segment)
        depth -= 2;
    }

    void visitBegin(const mediaelch::renamer::impl::PlaceholderConditionSegment& segment) override
    {
        Q_UNUSED(segment)
        str += QString(depth, ' ');
        str += "- PlaceholderConditionSegment\n";
        depth += 2;
    }

    void visitEnd(const mediaelch::renamer::impl::PlaceholderConditionSegment& segment) override
    {
        Q_UNUSED(segment)
        depth -= 2;
    }

    QString str;

private:
    int depth{0};
};

struct PlaceholderTestData : public mediaelch::renamer::impl::PlaceholderDataProvider
{
    explicit PlaceholderTestData(QHash<QString, QString> data) : m_dataMap(std::move(data)) {}
    ~PlaceholderTestData() override = default;

    ELCH_NODISCARD bool has(const QString& field) const override { return m_dataMap.contains(field); }
    ELCH_NODISCARD QString data(const QString& field) const override { return m_dataMap.value(field); }

private:
    QHash<QString, QString> m_dataMap;
};


} // namespace test

std::ostream& operator<<(std::ostream& os, const mediaelch::renamer::impl::TemplateLexer::TokenKind& value)
{
    using namespace mediaelch::renamer::impl;
    using Kind = TemplateLexer::TokenKind;
    switch (value) {
    case Kind::Whitespace: os << "Whitespace"; break;
    case Kind::Identifier: os << "Identifier"; break;
    case Kind::Number: os << "Number"; break;
    case Kind::Pipe: os << "Pipe"; break;
    case Kind::Slash: os << "Slash"; break;
    case Kind::OpenAngleBracket: os << "OpenAngleBracket"; break;
    case Kind::CloseAngleBracket: os << "CloseAngleBracket"; break;
    case Kind::OpenBrace: os << "OpenBrace"; break;
    case Kind::CloseBrace: os << "CloseBrace"; break;
    case Kind::OpenBracket: os << "OpenBracket"; break;
    case Kind::CloseBracket: os << "CloseBracket"; break;
    case Kind::Unknown: os << "Unknown"; break;
    case Kind::EOL: os << "EOL"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const mediaelch::renamer::impl::TemplateLexer::Token& value)
{
    os << "Token(kind:" << value.kind << ", offset:" << value.offset << ", length:" << value.length << ")";
    return os;
}
