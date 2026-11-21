#pragma once

#include "data/movie/Movie.h"
#include "scrapers/movie/MovieScraper.h"

#include <QPointer>

namespace test {

class FixedMovieSearchJob final : public mediaelch::scraper::MovieSearchJob
{
public:
    using MovieSearchJob::MovieSearchJob;

    void complete()
    {
        using mediaelch::scraper::MovieIdentifier;
        m_results = {
            {"First Result", {}, MovieIdentifier("first")},
            {"Second Result", {}, MovieIdentifier("second")},
        };
        emitFinished();
    }

    bool cancellationRequested = false;

private:
    void doStart() override {}

    bool doKill() override
    {
        cancellationRequested = true;
        // Simulate a request that has already completed at the provider and still delivers its result.
        return false;
    }
};

class FixedMovieScrapeJob final : public mediaelch::scraper::MovieScrapeJob
{
public:
    using MovieScrapeJob::MovieScrapeJob;

private:
    void doStart() override
    {
        movie().setTitle(config().identifier.str() == "second" ? "Second Result" : "First Result");
        emitFinished();
    }
};

class FixedMovieScraper final : public mediaelch::scraper::MovieScraper
{
public:
    FixedMovieScraper()
    {
        m_meta.identifier = "ui.fixture";
        m_meta.name = "Fixture Movies";
        m_meta.supportedDetails = {MovieScraperInfo::Title};
    }

    const ScraperMeta& meta() const override { return m_meta; }
    void initialize() override {}
    bool isInitialized() const override { return true; }
    void changeLanguage(mediaelch::Locale) override {}
    QSet<MovieScraperInfo> scraperNativelySupports() override { return m_meta.supportedDetails; }

    FixedMovieSearchJob* search(mediaelch::scraper::MovieSearchJob::Config config) override
    {
        searchJob = new FixedMovieSearchJob(std::move(config), this);
        return searchJob.data();
    }

    FixedMovieScrapeJob* loadMovie(mediaelch::scraper::MovieScrapeJob::Config config) override
    { return new FixedMovieScrapeJob(std::move(config), this); }

    QPointer<FixedMovieSearchJob> searchJob;

private:
    ScraperMeta m_meta;
};

} // namespace test
