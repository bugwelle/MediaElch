#pragma once

#include "data/Locale.h"
#include "scrapers/ScraperInfos.h"
#include "ui/scrapers/ScraperManager.h"

#include <QListWidgetItem>
#include <QString>
#include <QWidget>

namespace Ui {
class TvScraperSettingsWidget;
}

class TvScraperSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TvScraperSettingsWidget(QWidget* parent = nullptr);
    ~TvScraperSettingsWidget() override;

    void setScrapers(const std::vector<mediaelch::ManagedTvScraper>& scrapers);
    void loadSettings();

private:
    Ui::TvScraperSettingsWidget* ui = nullptr;
};
