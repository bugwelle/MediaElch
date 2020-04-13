#pragma once

#include "scrapers/tv_show/TvScraper.h"

#include "scrapers/tv_show/imdb/ImdbTvApi.h"

namespace mediaelch {
namespace scraper {

class ImdbTvEpisodeScrapeJob : public EpisodeScrapeJob
{
    Q_OBJECT

public:
    ImdbTvEpisodeScrapeJob(ImdbTvApi& api, Config _config, QObject* parent = nullptr);
    ~ImdbTvEpisodeScrapeJob() = default;
    void execute() override;

private:
    ImdbTvApi& m_api;
};

} // namespace scraper
} // namespace mediaelch
