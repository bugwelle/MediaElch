#include "scrapers/tv_show/imdb/ImdbTvSeasonScrapeJob.h"

#include <QTimer>

namespace mediaelch {
namespace scraper {

ImdbTvSeasonScrapeJob::ImdbTvSeasonScrapeJob(ImdbTvApi& api, SeasonScrapeJob::Config _config, QObject* parent) :
    SeasonScrapeJob(_config, parent), m_api{api}
{
}

void ImdbTvSeasonScrapeJob::execute()
{
    QTimer::singleShot(0, [this]() { emit sigFinished(this); });
}

} // namespace scraper
} // namespace mediaelch
