#pragma once

#include "scrapers/movie/imdb/ImdbMovieConfiguration.h"
#include "ui/scrapers/ScraperConfigurationView.h"
#include "utils/Meta.h"

#include <QCheckBox>
#include <QPointer>
#include <QWidget>

namespace mediaelch {
namespace scraper {

class ImdbMovieConfigurationView : public QObject, public ScraperConfigurationView
{
    Q_OBJECT

public:
    explicit ImdbMovieConfigurationView(ImdbMovieConfiguration& settings);
    ~ImdbMovieConfigurationView() override = default;

    void init() override;
    void load() override;
    void save() override;

    ELCH_NODISCARD QWidget* widget() override;

private:
    ImdbMovieConfiguration& m_settings;
    QPointer<QWidget> m_settingsWidget;
    QCheckBox* m_loadAllTagsWidget;
};

} // namespace scraper
} // namespace mediaelch
