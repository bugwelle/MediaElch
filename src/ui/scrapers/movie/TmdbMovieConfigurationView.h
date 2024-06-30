#pragma once

#include "scrapers/movie/tmdb/TmdbMovieConfiguration.h"
#include "ui/scrapers/ScraperConfigurationView.h"
#include "utils/Meta.h"

#include <QComboBox>
#include <QPointer>
#include <QWidget>

namespace mediaelch {
namespace scraper {

class TmdbMovieConfigurationView : public QObject, public ScraperConfigurationView
{
    Q_OBJECT

public:
    explicit TmdbMovieConfigurationView(TmdbMovieConfiguration& settings);
    ~TmdbMovieConfigurationView() override = default;

    void init() override;
    void load() override;
    void save() override;

    ELCH_NODISCARD QWidget* widget() override;

private:
    TmdbMovieConfiguration& m_settings;
    QPointer<QWidget> m_widget;
    QComboBox* m_box;
};

} // namespace scraper
} // namespace mediaelch
