#pragma once

#include "ui/scrapers/ScraperConfigurationView.h"
#include "utils/Meta.h"

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>
#include <vector>

class Settings;

namespace mediaelch {
namespace scraper {

class MusicScraper;
class ConcertScraper;
class TvScraper;
class MovieScraper;

} // namespace scraper

class ScraperConfiguration;

} // namespace mediaelch

namespace mediaelch {

template<class Scraper>
class ManagedScraper
{
public:
    Scraper* scraper() const { return m_scraper.get(); }
    ScraperConfiguration* config() const { return m_config.get(); }
    ScraperConfigurationView* view() const { return m_view.get(); }

private:
    std::unique_ptr<Scraper> m_scraper;
    std::unique_ptr<ScraperConfiguration> m_config;
    std::unique_ptr<ScraperConfigurationView> m_view;

    friend class ScraperManager;
};

using ManagedMovieScraper = ManagedScraper<mediaelch::scraper::MovieScraper>;
using ManagedTvScraper = ManagedScraper<mediaelch::scraper::TvScraper>;
using ManagedConcertScraper = ManagedScraper<mediaelch::scraper::ConcertScraper>;
using ManagedMusicScraper = ManagedScraper<mediaelch::scraper::MusicScraper>;

class ScraperManager : public QObject
{
    Q_OBJECT

public:
    explicit ScraperManager(Settings& settings, QObject* parent = nullptr);
    ~ScraperManager() override;

    ELCH_NODISCARD mediaelch::scraper::MovieScraper* movieScraper(const QString& identifier);
    ELCH_NODISCARD QVector<mediaelch::scraper::MovieScraper*> movieScrapers();
    ELCH_NODISCARD const std::vector<ManagedMovieScraper>& allMovieScrapers();

    ELCH_NODISCARD mediaelch::scraper::TvScraper* tvScraper(const QString& identifier);
    ELCH_NODISCARD QVector<mediaelch::scraper::TvScraper*> tvScrapers();
    ELCH_NODISCARD const std::vector<ManagedTvScraper>& allTvScrapers();

    ELCH_NODISCARD mediaelch::scraper::ConcertScraper* concertScraper(const QString& identifier);
    ELCH_NODISCARD QVector<mediaelch::scraper::ConcertScraper*> concertScrapers();
    ELCH_NODISCARD const std::vector<ManagedConcertScraper>& allConcertScrapers();

    ELCH_NODISCARD QVector<mediaelch::scraper::MusicScraper*> musicScrapers();
    ELCH_NODISCARD const std::vector<ManagedMusicScraper>& allMusicScrapers();


private:
    void initMovieScrapers();
    void initTvScrapers();
    void initConcertScrapers();
    void initMusicScrapers();

private:
    Settings& m_settings;

    std::vector<ManagedMovieScraper> m_scraperMovies;
    std::vector<ManagedTvScraper> m_scraperTv;
    std::vector<ManagedConcertScraper> m_scraperConcert;
    std::vector<ManagedMusicScraper> m_scraperMusic;
};

} // namespace mediaelch
