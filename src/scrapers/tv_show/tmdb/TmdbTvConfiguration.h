#pragma once

#include "data/Locale.h"
#include "scrapers/ScraperConfiguration.h"
#include "settings/Settings.h"
#include "utils/Meta.h"

namespace mediaelch {
namespace scraper {

class TmdbTvConfiguration : public ScraperConfiguration
{
public:
    explicit TmdbTvConfiguration(Settings& settings);
    ~TmdbTvConfiguration() override = default;

    void init() override;

    ELCH_NODISCARD static mediaelch::Locale defaultLocale();
    ELCH_NODISCARD static QVector<Locale> supportedLanguages();

public:
    ELCH_NODISCARD Locale language();
    void setLanguage(const Locale& value);
};

} // namespace scraper
} // namespace mediaelch
