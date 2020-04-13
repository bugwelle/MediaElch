#include "scrapers/tv_show/tmdb/TmdbTvSeasonScrapeJob.h"

#include "scrapers/tv_show/tmdb/TmdbTvApi.h"

#include <QTimer>

namespace mediaelch {
namespace scraper {

TmdbTvSeasonScrapeJob::TmdbTvSeasonScrapeJob(TmdbTvApi& api, SeasonScrapeJob::Config _config, QObject* parent) :
    SeasonScrapeJob(_config, parent), m_api{api}
{
}

void TmdbTvSeasonScrapeJob::execute()
{
    QTimer::singleShot(0, [this]() { emit sigFinished(this); });
}

} // namespace scraper
} // namespace mediaelch
