#pragma once

#include "scrapers/tv_show/TvScraper.h"

#include "scrapers/tv_show/imdb/ImdbTvApi.h"

namespace mediaelch {
namespace scraper {

class ImdbTvSeasonScrapeJob : public SeasonScrapeJob
{
    Q_OBJECT

public:
    ImdbTvSeasonScrapeJob(ImdbTvApi& api, Config _config, QObject* parent = nullptr);
    ~ImdbTvSeasonScrapeJob() = default;
    void execute() override;

private:
    ImdbTvApi& m_api;
};

} // namespace scraper
} // namespace mediaelch
