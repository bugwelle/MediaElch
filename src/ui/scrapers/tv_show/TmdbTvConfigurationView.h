#pragma once

#include "scrapers/tv_show/tmdb/TmdbTvConfiguration.h"
#include "ui/scrapers/ScraperConfigurationView.h"
#include "utils/Meta.h"

#include <QComboBox>
#include <QPointer>
#include <QWidget>

namespace mediaelch {
namespace scraper {

class TmdbTvConfigurationView : public QObject, public ScraperConfigurationView
{
    Q_OBJECT

public:
    explicit TmdbTvConfigurationView(TmdbTvConfiguration& settings);
    ~TmdbTvConfigurationView() override = default;

    void init() override;
    void load() override;
    void save() override;

    ELCH_NODISCARD QWidget* widget() override;

private:
    TmdbTvConfiguration& m_settings;
    QPointer<QWidget> m_widget;
    QComboBox* m_box;
};

} // namespace scraper
} // namespace mediaelch
