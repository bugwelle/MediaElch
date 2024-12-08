
#include "RenamerPlaceholders.h"

namespace mediaelch {

RenamerPlaceholders::~RenamerPlaceholders() = default;

RenamerPlaceholders::ValidationResult RenamerPlaceholders::validate(const QString& text)
{
    return ValidationResult::valid();
}

QString RenamerPlaceholders::replace(QString text, RenamerData& data)
{
    for (const auto& placeholder : placeholders()) {
        if (placeholder.isCondition) {
            QRegularExpression rx(QStringLiteral("{%1}(.*){/%1}").arg(placeholder.name),
                QRegularExpression::DotMatchesEverythingOption | QRegularExpression::InvertedGreedinessOption);
            QRegularExpressionMatch match = rx.match(text);
            if (match.hasMatch()) {
                const QString search = QStringLiteral("{%1}%2{/%1}").arg(placeholder.name, match.captured(1));
                text.replace(search, data.passesCondition(placeholder.name) ? match.captured(1) : "");
            }
        }

        if (placeholder.isValue) {
            text.replace("<" + placeholder.name + ">", data.value(placeholder.name).trimmed());
        }
    }
    return text;
}

} // namespace mediaelch
