#include "scrapers/tv_show/TvScraper.h"

#include "tv_shows/TvShow.h"

#include <QRegularExpression>

namespace mediaelch {
namespace scraper {

QPair<QString, QString> ShowSearchJob::extractTitleAndYear(const QString& query)
{
    QVector<QRegularExpression> yearRegEx;
    yearRegEx << QRegularExpression(R"(^(.*) \((\d{4})\)$)") //
              << QRegularExpression(R"(^(.*) (\d{4})$)")     //
              << QRegularExpression(R"(^(.*) - (\d{4})$)");

    for (auto& rxYear : yearRegEx) {
        // minimal matching
        rxYear.setPatternOptions(QRegularExpression::InvertedGreedinessOption);
        auto match = rxYear.match(query);
        if (match.hasMatch()) {
            QString searchTitle = match.captured(0);
            QString searchYear = match.captured(1);
            return {searchTitle, searchTitle};
        }
    }
    return {};
}

ShowSearchJob::ShowSearchJob(ShowSearchJob::Config config, QObject* parent) :
    QObject(parent), m_config{std::move(config)}
{
}

const ShowSearchJob::Config& ShowSearchJob::config() const
{
    return m_config;
}

bool ShowSearchJob::hasError() const
{
    return m_error.hasError();
}

const ScraperSearchError& ShowSearchJob::error() const
{
    return m_error;
}

const QVector<ShowSearchJob::Result>& ShowSearchJob::results() const
{
    return m_results;
}

ShowScrapeJob::ShowScrapeJob(ShowScrapeJob::Config config, QObject* parent) :
    QObject(parent), m_tvShow{new TvShow({}, this)}, m_config{config}
{
}

bool ShowScrapeJob::hasError() const
{
    return m_error.hasError();
}

const ScraperLoadError& ShowScrapeJob::error() const
{
    return m_error;
}

EpisodeScrapeJob::EpisodeScrapeJob(EpisodeScrapeJob::Config config, QObject* parent) :
    QObject(parent), m_episode{new TvShowEpisode({}, this)}, m_config{config}
{
}

bool EpisodeScrapeJob::hasError() const
{
    return m_error.hasError();
}

const ScraperLoadError& EpisodeScrapeJob::error() const
{
    return m_error;
}

SeasonScrapeJob::SeasonScrapeJob(SeasonScrapeJob::Config config, QObject* parent) :
    QObject(parent), m_config{std::move(config)}
{
}

bool SeasonScrapeJob::hasError() const
{
    return m_error.hasError();
}

const ScraperLoadError& SeasonScrapeJob::error() const
{
    return m_error;
}

QDebug operator<<(QDebug debug, const ShowIdentifier& id)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "ShowIdentifier(" << id.str() << ')';

    return debug;
}

QDebug operator<<(QDebug debug, const EpisodeIdentifier& id)
{
    QDebugStateSaver saver(debug);
    if (id.hasEpisodeIdentifier()) {
        debug.nospace() << "EpisodeIdentifier(" << id.episodeIdentifier << ')';
    } else {
        debug.nospace() << "EpisodeIdentifier(Show(" << id.showIdentifier << "), " << id.seasonNumber << ", "
                        << id.episodeNumber << ')';
    }
    return debug;
}

QVector<ScraperSearchResult> toOldScraperSearchResult(const QVector<ShowSearchJob::Result>& searchResults)
{
    QVector<ScraperSearchResult> results;
    for (const auto& searchResult : searchResults) {
        ScraperSearchResult result;
        result.id = searchResult.identifier.str();
        result.name = searchResult.title;
        result.released = searchResult.released;
        results.push_back(result);
    }
    return results;
}

} // namespace scraper
} // namespace mediaelch
