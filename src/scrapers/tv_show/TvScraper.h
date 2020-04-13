#pragma once

#include "data/ImdbId.h"
#include "data/Locale.h"
#include "data/TmdbId.h"
#include "globals/Globals.h"
#include "globals/ScraperInfos.h"
#include "globals/ScraperResult.h"
#include "scrapers/ScraperInterface.h"
#include "tv_shows/EpisodeMap.h"
#include "tv_shows/EpisodeNumber.h"
#include "tv_shows/SeasonNumber.h"
#include "tv_shows/SeasonOrder.h"
#include "tv_shows/TvDbId.h"

#include <QDate>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>
#include <memory>

class TvShow;
class TvShowEpisode;

namespace mediaelch {
namespace scraper {

// This file contains all interfaces for a TV show scraper:
class ShowSearchJob;
class ShowScrapeJob;
class TvScraper;


/// \brief This class uniquely identifies a TV show for scrapers.
class ShowIdentifier
{
public:
    ShowIdentifier() = default;
    explicit ShowIdentifier(QString _showIdentifier) : id{std::move(_showIdentifier)} {}
    explicit ShowIdentifier(TmdbId _showIdentifier) : id{_showIdentifier.toString()} {}
    explicit ShowIdentifier(TvDbId _showIdentifier) : id{_showIdentifier.toString()} {}
    explicit ShowIdentifier(ImdbId _showIdentifier) : id{_showIdentifier.toString()} {}
    ~ShowIdentifier() = default;

    const QString& str() const { return id; }

private:
    QString id;
};

QDebug operator<<(QDebug debug, const ShowIdentifier& id);


/// \brief This class uniquely identifies an episode for scrapers.
/// \details An episode is either identified by an unique string, e.g. TheTvDb
///          ID or an URL or it is identified by a season number, episode
///          number and season order and of course its TvShow identifier.
class EpisodeIdentifier
{
public:
    explicit EpisodeIdentifier(QString _episodeIdentifier) : episodeIdentifier{std::move(_episodeIdentifier)} {}
    explicit EpisodeIdentifier(TvDbId _episodeIdentifier) : episodeIdentifier{_episodeIdentifier.toString()} {}
    explicit EpisodeIdentifier(ImdbId _episodeIdentifier) : episodeIdentifier{_episodeIdentifier.toString()} {}

    EpisodeIdentifier(QString _showIdentifier, SeasonNumber season, EpisodeNumber episode, SeasonOrder order) :
        showIdentifier{std::move(_showIdentifier)},
        seasonNumber{std::move(season)},
        episodeNumber{std::move(episode)},
        seasonOrder{order}
    {
    }
    ~EpisodeIdentifier() = default;

    /// \brief Whether the episode can be uniquely identified by a its
    ///        identifier string.
    bool hasEpisodeIdentifier() const { return !episodeIdentifier.isEmpty(); }

public:
    QString episodeIdentifier;

    QString showIdentifier;
    SeasonNumber seasonNumber;
    EpisodeNumber episodeNumber;
    SeasonOrder seasonOrder;
};

QDebug operator<<(QDebug debug, const EpisodeIdentifier& id);


/// \brief A TV show search request resolved by a scraper.
class ShowSearchJob : public QObject
{
    Q_OBJECT

public:
    /// \brief Configuration object for a TV show search.
    struct Config
    {
        /// \brief The search string
        QString query;
        /// \brief Language key for the scraper, e.g. "en-US", "de-DE", ...
        Locale locale = Locale::English;
        /// \brief Whether to include adult (NFSW) search results.
        /// \details The scraper may or may not support adult search results.
        bool includeAdult = false;
    };

    /// \brief Search result of a TV show search request.
    struct Result
    {
        /// \brief Title shown to the user.
        QString title;
        /// \brief Release date of the TV show.
        /// \details Date is used for showing the year behind the search result.
        QDate released;
        /// \brief Scraper specific identifier that may be used for scraping a TV show.
        /// \details The identifier can be passed to scrape().
        ShowIdentifier identifier;
    };

public:
    /// \brief Extract the title and year from a search query.
    /// \details This function checks for common patterns and extract the title
    ///          and year if a pattern matches.
    /// \returns Title/Year pair if a pattern matched, empty string pair otherwise.
    static QPair<QString, QString> extractTitleAndYear(const QString& query);

public:
    /// \brief Create a TV show search.
    explicit ShowSearchJob(Config config, QObject* parent = nullptr);
    virtual ~ShowSearchJob() = default;

    virtual void execute() = 0;

public:
    ELCH_NODISCARD const Config& config() const;
    ELCH_NODISCARD bool hasError() const;
    ELCH_NODISCARD const ScraperSearchError& error() const;
    ELCH_NODISCARD const QVector<ShowSearchJob::Result>& results() const;

signals:
    /// \brief Signal emitted when the search() request has finished.
    ///
    /// Use hasError() and results() to know whether the request was successful.
    void sigFinished(ShowSearchJob* searchJob);

protected:
    QVector<ShowSearchJob::Result> m_results;
    ScraperSearchError m_error;

private:
    const Config m_config;
};

QVector<ScraperSearchResult> toOldScraperSearchResult(const QVector<ShowSearchJob::Result>& searchResults);

class ShowScrapeJob : public QObject
{
    Q_OBJECT

public:
    /// \brief Configuration object for a TV show scrape job.
    struct Config
    {
        /// \brief A string that can be consumed by the TV show scraper.
        /// \details It is used to uniquely identify the TV show. May be an IMDb ID in
        ///          string representation or an URL.
        ShowIdentifier identifier;

        /// \brief Language key for the scraper, e.g. "en-US", "de-DE", ...
        Locale locale = Locale::English;

        /// \brief TV show details to be loaded using the scraper.
        QSet<ShowScraperInfo> details;
    };

public:
    ShowScrapeJob(Config config, QObject* parent = nullptr);
    virtual ~ShowScrapeJob() = default;

    virtual void execute() = 0;

public:
    ELCH_NODISCARD TvShow& tvShow() { return *m_tvShow; }
    ELCH_NODISCARD const TvShow& tvShow() const { return *m_tvShow; }

    ELCH_NODISCARD const Config& config() const { return m_config; }

    ELCH_NODISCARD bool hasError() const;
    ELCH_NODISCARD const ScraperLoadError& error() const;

signals:
    /// \brief Signal emitted when the scrape job has finished.
    ///
    /// Use hasError() and tvShow() to know whether the request was successful.
    void sigFinished(ShowScrapeJob* scrapeJob);

    /// \brief Signals a download progress. Useful if a scraper has to load
    ///        data from multiple sites or sends multiple requests.
    void sigProgress(int progress, int max);

protected:
    TvShow* m_tvShow = nullptr;
    const Config m_config;
    ScraperLoadError m_error;
};

class EpisodeScrapeJob : public QObject
{
    Q_OBJECT

public:
    /// \brief Configuration object for a TV show scrape job.
    struct Config
    {
        Config(EpisodeIdentifier _identifier, Locale _locale, QSet<EpisodeScraperInfo> _details) :
            identifier{std::move(_identifier)}, locale{std::move(_locale)}, details{std::move(_details)}
        {
        }

        /// \brief An identifier that can be consumed by the episode scraper.
        /// \details It is used to uniquely identify the episode. May be a TvDb ID in
        ///          string representation or an URL or a combination of season/episode
        ///          number and TvShow id.
        EpisodeIdentifier identifier;

        /// \brief Language key for the scraper, e.g. "en-US", "de-DE", ...
        Locale locale = Locale::English;

        /// \brief Details to be loaded using the scraper.
        QSet<EpisodeScraperInfo> details;
    };

public:
    EpisodeScrapeJob(Config config, QObject* parent = nullptr);
    virtual ~EpisodeScrapeJob() = default;

    virtual void execute() = 0;

    ELCH_NODISCARD TvShowEpisode& episode() { return *m_episode; }
    ELCH_NODISCARD const Config& config() { return m_config; }

    ELCH_NODISCARD bool hasError() const;
    ELCH_NODISCARD const ScraperLoadError& error() const;

signals:
    /// \brief Signal emitted when the scrape job has finished.
    ///
    /// Use hasError() and tvShow() to know whether the request was successful.
    void sigFinished(EpisodeScrapeJob* scrapeJob);

    /// \brief Signals a download progress. Useful if a scraper has to load
    ///        data from multiple sites or sends multiple requests.
    void sigProgress(int progress, int max);

protected:
    TvShowEpisode* m_episode = nullptr;
    const Config m_config;
    ScraperLoadError m_error;
};

/// \brief Load episodes of the given seasons.
class SeasonScrapeJob : public QObject
{
    Q_OBJECT

public:
    /// \brief Configuration object for a TV show scrape job.
    struct Config
    {
        Config(ShowIdentifier _identifier,
            Locale _locale,
            QSet<SeasonNumber> _seasons,
            SeasonOrder _seasonOrder,
            QSet<EpisodeScraperInfo> _details) :
            showIdentifier{std::move(_identifier)},
            locale{std::move(_locale)},
            seasons{std::move(_seasons)},
            seasonOrder{_seasonOrder},
            details{std::move(_details)}
        {
        }

        /// \brief A string that can be consumed by the TV show scraper.
        /// \details It is used to uniquely identify the TV show. May be an IMDb ID in
        ///          string representation or an URL.
        ShowIdentifier showIdentifier;

        /// \brief Language key for the scraper, e.g. "en-US", "de-DE", ...
        Locale locale = Locale::English;

        /// \brief Set of seasons that whose episodes shall be loaded.
        QSet<SeasonNumber> seasons;

        /// \brief Order of episodes when loading seasons.
        SeasonOrder seasonOrder;

        /// \brief Details to be loaded using the scraper.
        QSet<EpisodeScraperInfo> details;

        /// \brief Returns true if all seasons should be loaded.
        bool shouldLoadAllSeasons() const { return seasons.isEmpty(); }
    };

public:
    SeasonScrapeJob(Config config, QObject* parent = nullptr);
    virtual ~SeasonScrapeJob() = default;

    virtual void execute() = 0;

public:
    ELCH_NODISCARD const EpisodeMap& episodes() const { return m_episodes; }

    ELCH_NODISCARD const Config& config() const { return m_config; }

    ELCH_NODISCARD bool hasError() const;
    ELCH_NODISCARD const ScraperLoadError& error() const;

signals:
    /// \brief Signal emitted when the scrape job has finished.
    ///
    /// Use hasError() and tvShow() to know whether the request was successful.
    void sigFinished(SeasonScrapeJob* scrapeJob);

    /// \brief Signals a download progress. Useful if a scraper has to load
    ///        data from multiple sites or sends multiple requests.
    void sigProgress(int progress, int max);

protected:
    EpisodeMap m_episodes;
    const Config m_config;
    ScraperLoadError m_error;
};


/// \brief A scraper for tvShows that allows searching for and loading of
///        TV show details.
class TvScraper : public QObject
{
    Q_OBJECT

public:
    /// \brief   Information object about the scraper.
    /// \details This object can be used to display details about the scraper.
    ///          For example in the "About" dialog for each scraper or similar.
    struct ScraperMeta
    {
        /// \brief Unique identifier used to store settings and more.
        /// \details The identifier must not be changed once set and is often the
        /// lowercase name of the data provider without spaces or other special characters.
        QString identifier;

        /// \brief Human readable name of the scraper. Often its title.
        QString name;

        /// \brief Short description of the scraper, i.e. a one-liner.
        QString description;

        /// \brief The data provider's website, e.g. https://kodi.tv
        QUrl website;

        /// \brief An URL to the provider's terms of service.
        QUrl termsOfService;

        /// \brief An URL to the data provider's data policy.
        QUrl privacyPolicy;

        /// \brief An URL to the data provider's contact page or forum.
        QUrl help;

        /// \brief A set of show details that the scraper supports.
        QSet<ShowScraperInfo> supportedShowDetails;

        /// \brief A set of  episodedetails that the scraper supports.
        QSet<EpisodeScraperInfo> supportedEpisodeDetails;

        /// \brief A set of season orders that the scraper supports.
        QSet<SeasonOrder> supportedSeasonOrders = {SeasonOrder::Aired};

        /// \brief A list of languages that are supported by the scraper.
        /// \see Locale::Locale
        QVector<Locale> supportedLanguages = {Locale::English};

        /// \brief Default locale for this scraper.
        Locale defaultLocale = Locale::English;
    };

public:
    explicit TvScraper(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~TvScraper() = default;

    /// \brief Information about the scraper.
    virtual const ScraperMeta& meta() const = 0;

    virtual void initialize() = 0;
    virtual bool isInitialized() const = 0;

signals:
    void initialized(bool wasSuccessful, TvScraper* scraper);

public:
    /// \brief Search for the given \p query.
    ///
    /// \param config Configuration for the search, e.g. language and search query.
    ELCH_NODISCARD virtual ShowSearchJob* search(ShowSearchJob::Config config) = 0;

    /// \brief   Load a TV show using the given identifier.
    /// \details Only the given details are loaded which - if only the title
    ///          shall be loaded - results in fewer network requests and faster lookup.
    ///
    /// \param config Configuration for the scrape job, e.g. language and show ID.
    ELCH_NODISCARD virtual ShowScrapeJob* loadShow(ShowScrapeJob::Config config) = 0;

    /// \brief   Load episodes of the given seasons.
    /// \details Only episodes of the configured seasons are loaded.
    ///
    /// \param config Configuration for the scrape job, e.g. language and show ID.
    ELCH_NODISCARD virtual SeasonScrapeJob* loadSeasons(SeasonScrapeJob::Config config) = 0;

    /// \brief   Load a TV episode using the given identifier.
    /// \details Only the given details are loaded which - if only the title
    ///          shall be loaded - results in fewer network requests and faster lookup.
    ///
    /// \param config Configuration for the scrape job, e.g. language and episode ID.
    ELCH_NODISCARD virtual EpisodeScrapeJob* loadEpisode(EpisodeScrapeJob::Config config) = 0;
};

} // namespace scraper
} // namespace mediaelch
