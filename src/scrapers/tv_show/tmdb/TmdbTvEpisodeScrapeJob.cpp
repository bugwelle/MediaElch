#include "scrapers/tv_show/tmdb/TmdbTvEpisodeScrapeJob.h"

#include "scrapers/tv_show/tmdb/TmdbTvApi.h"

#include <QTimer>

namespace mediaelch {
namespace scraper {

TmdbTvEpisodeScrapeJob::TmdbTvEpisodeScrapeJob(TmdbTvApi& api, EpisodeScrapeJob::Config _config, QObject* parent) :
    EpisodeScrapeJob(_config, parent), m_api{api}
{
}

void TmdbTvEpisodeScrapeJob::execute()
{
    QTimer::singleShot(0, [this]() { emit sigFinished(this); });
}

} // namespace scraper
} // namespace mediaelch
