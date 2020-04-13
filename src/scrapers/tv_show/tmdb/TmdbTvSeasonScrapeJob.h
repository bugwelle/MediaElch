#pragma once

#include "scrapers/tv_show/TvScraper.h"

namespace mediaelch {
namespace scraper {

class TmdbTvApi;

class TmdbTvSeasonScrapeJob : public SeasonScrapeJob
{
    Q_OBJECT

public:
    TmdbTvSeasonScrapeJob(TmdbTvApi& api, Config _config, QObject* parent = nullptr);
    ~TmdbTvSeasonScrapeJob() = default;
    void execute() override;

private:
    TmdbTvApi& m_api;
};

} // namespace scraper
} // namespace mediaelch
