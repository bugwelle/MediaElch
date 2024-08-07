#pragma once

#include "scrapers/movie/MovieScraper.h"
#include "scrapers/tmdb/TmdbApi.h"

#include <QMap>
#include <QObject>
#include <QPointer>

namespace mediaelch {
namespace scraper {

class TmdbMovieConfiguration;

class TmdbMovie final : public MovieScraper
{
    Q_OBJECT
public:
    /// \brief Identifier of this scraper; used in user settings as key
    /// Note that this identifier must not be changed, or it may break user's settings.
    static constexpr const char* ID = "TMDb";

public:
    explicit TmdbMovie(TmdbMovieConfiguration& settings, QObject* parent = nullptr);
    ~TmdbMovie() override;

    const ScraperMeta& meta() const override;

    void initialize() override;
    bool isInitialized() const override;

    ELCH_NODISCARD MovieSearchJob* search(MovieSearchJob::Config config) override;
    ELCH_NODISCARD MovieScrapeJob* loadMovie(MovieScrapeJob::Config config) override;

public:
    QSet<MovieScraperInfo> scraperNativelySupports() override;
    void changeLanguage(mediaelch::Locale locale) override;

private:
    TmdbMovieConfiguration& m_settings;
    TmdbApi m_api;
    ScraperMeta m_meta;

    QSet<MovieScraperInfo> m_scraperNativelySupports;
};

} // namespace scraper
} // namespace mediaelch
