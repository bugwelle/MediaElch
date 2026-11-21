#define CATCH_CONFIG_RUNNER
#include "third_party/catch2/catch.hpp"

#include "utils/Meta.h"

#include <QApplication>
#include <QDir>
#include <QWidget>

namespace {

struct FailureScreenshot : Catch::TestEventListenerBase
{
    using TestEventListenerBase::TestEventListenerBase;

    bool assertionEnded(const Catch::AssertionStats& stats) override
    {
        if (!stats.assertionResult.isOk()
            && stats.assertionResult.getResultType() != Catch::ResultWas::FatalErrorCondition) {
            const auto profilePath = qEnvironmentVariable("MEDIAELCH_UI_PROFILE");
            auto* window = QApplication::activeModalWidget();
            if (!window) {
                window = QApplication::activeWindow();
            }
            if (!profilePath.isEmpty() && window && window->isVisible()) {
                const auto line = stats.assertionResult.getSourceInfo().line;
                window->grab().save(QDir(profilePath).filePath(QString("failure-%1.png").arg(line)));
            }
        }
        return false;
    }
};

} // namespace

CATCH_REGISTER_LISTENER(FailureScreenshot)

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    registerAllMetaTypes();
    return Catch::Session().run(argc, argv);
}
