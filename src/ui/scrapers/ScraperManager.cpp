#include "ui/scrapers/ScraperManager.h"

#include "log/Log.h"
#include "scrapers/ScraperConfiguration.h"
#include "scrapers/concert/ConcertScraper.h"
#include "scrapers/concert/tmdb/TmdbConcert.h"
#include "scrapers/movie/MovieScraper.h"
#include "scrapers/movie/adultdvdempire/AdultDvdEmpire.h"
#include "scrapers/movie/aebn/AEBN.h"
#include "scrapers/movie/aebn/AebnConfiguration.h"
#include "scrapers/movie/custom/CustomMovieScraper.h"
#include "scrapers/movie/hotmovies/HotMovies.h"
#include "scrapers/movie/imdb/ImdbMovie.h"
#include "scrapers/movie/imdb/ImdbMovieConfiguration.h"
#include "scrapers/movie/tmdb/TmdbMovie.h"
#include "scrapers/movie/tmdb/TmdbMovieConfiguration.h"
#include "scrapers/movie/videobuster/VideoBuster.h"
#include "scrapers/music/MusicScraper.h"
#include "scrapers/music/UniversalMusicScraper.h"
#include "scrapers/tv_show/TvScraper.h"
#include "scrapers/tv_show/custom/CustomTvScraper.h"
#include "scrapers/tv_show/fernsehserien_de/FernsehserienDe.h"
#include "scrapers/tv_show/imdb/ImdbTv.h"
#include "scrapers/tv_show/thetvdb/TheTvDb.h"
#include "scrapers/tv_show/tmdb/TmdbTv.h"
#include "scrapers/tv_show/tmdb/TmdbTvConfiguration.h"
#include "scrapers/tv_show/tvmaze/TvMaze.h"
#include "settings/Settings.h"
#include "ui/scrapers/movie/AebnConfigurationView.h"
#include "ui/scrapers/movie/ImdbMovieConfigurationView.h"
#include "ui/scrapers/movie/TmdbMovieConfigurationView.h"
#include "ui/scrapers/tv_show/TmdbTvConfigurationView.h"

namespace mediaelch {

ScraperManager::ScraperManager(Settings& settings, QObject* parent) : QObject(parent), m_settings{settings}
{
    initMovieScrapers();
    initTvScrapers();
    initConcertScrapers();
    initMusicScrapers();
}

ScraperManager::~ScraperManager() = default;

QVector<mediaelch::scraper::MovieScraper*> ScraperManager::movieScrapers()
{
    QVector<mediaelch::scraper::MovieScraper*> movieScrapers;
    for (auto& entry : m_scraperMovies) {
        if (entry.scraper() != nullptr) {
            movieScrapers << entry.scraper();
        }
    }
    return movieScrapers;
}

mediaelch::scraper::MovieScraper* ScraperManager::movieScraper(const QString& identifier)
{
    for (auto& scraper : asConst(m_scraperMovies)) {
        if (scraper.scraper()->meta().identifier == identifier) {
            return scraper.scraper();
        }
    }
    MediaElch_Debug_Assert(identifier != "");
    if (identifier != "images.fanarttv") { // TODO
        qCDebug(generic) << "[ScraperManager] No scraper with ID:" << identifier;
    }
    return nullptr;
}

scraper::ConcertScraper* ScraperManager::concertScraper(const QString& identifier)
{
    for (auto& scraper : asConst(m_scraperConcert)) {
        if (scraper.scraper()->meta().identifier == identifier) {
            return scraper.scraper();
        }
    }

    return nullptr;
}

QVector<mediaelch::scraper::TvScraper*> ScraperManager::tvScrapers()
{
    QVector<mediaelch::scraper::TvScraper*> tvScrapers;
    for (auto& entry : m_scraperTv) {
        if (entry.scraper() != nullptr) {
            tvScrapers << entry.scraper();
        }
    }
    return tvScrapers;
}

mediaelch::scraper::TvScraper* ScraperManager::tvScraper(const QString& identifier)
{
    for (auto& scraper : asConst(m_scraperTv)) {
        if (scraper.scraper()->meta().identifier == identifier) {
            return scraper.scraper();
        }
    }
    return nullptr;
}

/**
 * \brief Returns a list of all concert scrapers
 * \return List of pointers of concert scrapers
 */
QVector<mediaelch::scraper::ConcertScraper*> ScraperManager::concertScrapers()
{
    QVector<mediaelch::scraper::ConcertScraper*> concertScrapers;
    for (auto& entry : m_scraperConcert) {
        if (entry.scraper() != nullptr) {
            concertScrapers << entry.scraper();
        }
    }
    return concertScrapers;
}

QVector<mediaelch::scraper::MusicScraper*> ScraperManager::musicScrapers()
{
    QVector<mediaelch::scraper::MusicScraper*> musicScrapers;
    for (auto& entry : m_scraperMusic) {
        if (entry.scraper() != nullptr) {
            musicScrapers << entry.scraper();
        }
    }
    return musicScrapers;
}

void ScraperManager::initMovieScrapers()
{
    using namespace mediaelch::scraper;

    ManagedMovieScraper tmdb;
    auto tmdbConfig = std::make_unique<TmdbMovieConfiguration>(m_settings);
    tmdbConfig->init();
    tmdb.m_scraper = std::make_unique<TmdbMovie>(*tmdbConfig, nullptr);
    tmdb.m_view = std::make_unique<TmdbMovieConfigurationView>(*tmdbConfig);
    tmdb.m_view->init();
    tmdb.m_config = std::move(tmdbConfig);

    ManagedMovieScraper imdb;
    auto imdbConfig = std::make_unique<ImdbMovieConfiguration>(m_settings);
    imdbConfig->init();
    imdb.m_scraper = std::make_unique<ImdbMovie>(*imdbConfig, nullptr);
    imdb.m_view = std::make_unique<ImdbMovieConfigurationView>(*imdbConfig);
    imdb.m_view->init();
    imdb.m_config = std::move(imdbConfig);

    ManagedMovieScraper videoBuster;
    videoBuster.m_scraper = std::make_unique<VideoBuster>(nullptr);

    // Adult Movie Scrapers
    ManagedMovieScraper hotMovies;
    hotMovies.m_scraper = std::make_unique<HotMovies>(nullptr);

    ManagedMovieScraper ade;
    ade.m_scraper = std::make_unique<AdultDvdEmpire>(nullptr);

    ManagedMovieScraper aebn;
    auto aebnConfig = std::make_unique<AebnConfiguration>(m_settings);
    aebnConfig->init();
    aebn.m_scraper = std::make_unique<AEBN>(*aebnConfig, nullptr);
    aebn.m_view = std::make_unique<AebnConfigurationView>(*aebnConfig);
    aebn.m_view->init();
    aebn.m_config = std::move(aebnConfig);

    // Custom Movie Scraper
    ManagedMovieScraper custom;
    custom.m_scraper = std::make_unique<CustomMovieScraper>(nullptr);

    m_scraperMovies.push_back(std::move(tmdb));
    m_scraperMovies.push_back(std::move(imdb));
    m_scraperMovies.push_back(std::move(videoBuster));
    m_scraperMovies.push_back(std::move(hotMovies));
    m_scraperMovies.push_back(std::move(ade));
    m_scraperMovies.push_back(std::move(aebn));
    m_scraperMovies.push_back(std::move(custom));
}

void ScraperManager::initTvScrapers()
{
    using namespace mediaelch::scraper;

    ManagedTvScraper tmdb;
    auto tmdbConfig = std::make_unique<TmdbTvConfiguration>(m_settings);
    tmdbConfig->init();
    tmdb.m_scraper = std::make_unique<TmdbTv>(*tmdbConfig, nullptr);
    tmdb.m_view = std::make_unique<TmdbTvConfigurationView>(*tmdbConfig);
    tmdb.m_view->init();
    tmdb.m_config = std::move(tmdbConfig);

    ManagedTvScraper theTvDb;
    theTvDb.m_scraper = std::make_unique<TheTvDb>(nullptr);

    ManagedTvScraper imdbTv;
    imdbTv.m_scraper = std::make_unique<ImdbTv>(nullptr);

    ManagedTvScraper tvMaze;
    tvMaze.m_scraper = std::make_unique<TvMaze>(nullptr);

    ManagedTvScraper fernsehserienDe;
    fernsehserienDe.m_scraper = std::make_unique<FernsehserienDe>(nullptr);

    auto* tmdbPtr = dynamic_cast<TmdbTv*>(tmdb.scraper());
    MediaElch_Assert(tmdbPtr != nullptr);
    auto* imdbPtr = dynamic_cast<ImdbTv*>(imdbTv.scraper());
    MediaElch_Assert(imdbPtr != nullptr);

    m_scraperTv.push_back(std::move(tmdb));
    m_scraperTv.push_back(std::move(theTvDb));
    m_scraperTv.push_back(std::move(imdbTv));
    m_scraperTv.push_back(std::move(tvMaze));
    m_scraperTv.push_back(std::move(fernsehserienDe));


    for (auto& managed : asConst(m_scraperTv)) {
        auto* scraper = managed.scraper();

        qCInfo(generic) << "[TvScraper] Initializing" << scraper->meta().name;
        connect(scraper, &scraper::TvScraper::initialized, this, [](bool wasSuccessful, scraper::TvScraper* tv) {
            if (wasSuccessful) {
                qCInfo(generic) << "[TvScraper] Initialized:" << tv->meta().name;
            } else {
                qCWarning(generic) << "[TvScraper] Initialization failed:" << tv->meta().name;
            }
        });
        scraper->initialize();
    }


    // Only add the Custom TV scraper after the previous ones were added
    // since the constructor explicitly requires them.
    // TODO: Use detail->scraper maps
    ManagedTvScraper custom;
    auto customConfig = std::make_unique<CustomTvScraperConfiguration>(m_settings,
        *tmdbPtr,
        *imdbPtr,
        CustomTvScraperConfiguration::ScraperForShowDetails{},
        CustomTvScraperConfiguration::ScraperForEpisodeDetails{});
    customConfig->init();
    custom.m_scraper = std::make_unique<CustomTvScraper>(*customConfig, nullptr);
    custom.m_config = std::move(customConfig);

    m_scraperTv.push_back(std::move(custom));
}

void ScraperManager::initConcertScrapers()
{
    ManagedConcertScraper tmdbConcert;
    tmdbConcert.m_scraper = std::make_unique<mediaelch::scraper::TmdbConcert>(nullptr);

    m_scraperConcert.push_back(std::move(tmdbConcert));
}

void ScraperManager::initMusicScrapers()
{
    ManagedMusicScraper universal;
    universal.m_scraper = std::make_unique<mediaelch::scraper::UniversalMusicScraper>(nullptr);

    m_scraperMusic.push_back(std::move(universal));
}

const std::vector<ManagedMovieScraper>& ScraperManager::allMovieScrapers()
{
    return m_scraperMovies;
}

const std::vector<ManagedTvScraper>& ScraperManager::allTvScrapers()
{
    return m_scraperTv;
}

const std::vector<ManagedConcertScraper>& ScraperManager::allConcertScrapers()
{
    return m_scraperConcert;
}
const std::vector<ManagedMusicScraper>& ScraperManager::allMusicScrapers()
{
    return m_scraperMusic;
}

} // namespace mediaelch
