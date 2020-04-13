#include "scrapers/tv_show/imdb/ImdbTvEpisodeScrapeJob.h"

#include "scrapers/tv_show/imdb/ImdbTvApi.h"

#include <QTimer>

namespace mediaelch {
namespace scraper {

ImdbTvEpisodeScrapeJob::ImdbTvEpisodeScrapeJob(ImdbTvApi& api, EpisodeScrapeJob::Config _config, QObject* parent) :
    EpisodeScrapeJob(_config, parent), m_api{api}
{
}

void ImdbTvEpisodeScrapeJob::execute()
{
    QTimer::singleShot(0, [this]() { emit sigFinished(this); });
}

} // namespace scraper
} // namespace mediaelch
