#include "ui/settings/TvScraperSettingsWidget.h"
#include "ui_TvScraperSettingsWidget.h"

#include "TvScraperInfoWidget.h"
#include "globals/Helper.h"
#include "globals/Manager.h"
#include "log/Log.h"
#include "scrapers/tv_show/TvScraper.h"
#include "settings/ScraperSettings.h"
#include "settings/Settings.h"

#include <QListWidgetItem>
#include <QScrollArea>

using namespace mediaelch;

TvScraperSettingsWidget::TvScraperSettingsWidget(QWidget* parent) : QWidget(parent), ui(new Ui::TvScraperSettingsWidget)
{
    ui->setupUi(this);

    connect(ui->tvScraperList, &QListWidget::currentRowChanged, ui->stackedWidget, &QStackedWidget::setCurrentIndex);
}

TvScraperSettingsWidget::~TvScraperSettingsWidget()
{
    delete ui;
}

void TvScraperSettingsWidget::loadSettings()
{
}

void TvScraperSettingsWidget::setScrapers(const std::vector<ManagedTvScraper>& scrapers)
{
    ui->tvScraperList->blockSignals(true);
    ui->tvScraperList->clear();
    // FIXME: clear ui->stackedWidget->();

    for (auto& managed : scrapers) {
        auto& scraper = *managed.scraper();
        const auto& id = scraper.meta().identifier;

        auto* item = new QListWidgetItem;
        item->setText(scraper.meta().name);
        item->setData(Qt::UserRole, id);
        ui->tvScraperList->addItem(item);

        auto* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setSizeAdjustPolicy(QScrollArea::AdjustToContents);

        auto* page = new QWidget(scrollArea);
        scrollArea->setWidget(page);

        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(new TvScraperInfoWidget(scraper, page));

        ScraperConfigurationView* view = managed.view();
        if (view != nullptr) {
            QWidget* settings = view->widget();
            if (settings != nullptr) {
                layout->addWidget(new QLabel(tr("Settings")));
                layout->addWidget(settings);
            }
        }

        layout->addStretch();

        ui->stackedWidget->addWidget(scrollArea);
    }

    ui->tvScraperList->blockSignals(false);

    ui->tvScraperList->item(0)->setSelected(true);
}
