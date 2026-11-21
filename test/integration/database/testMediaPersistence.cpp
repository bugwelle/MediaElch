#include "test/integration/database/DatabaseTestHelpers.h"

#include "data/concert/Concert.h"
#include "data/tv_show/TvShow.h"
#include "database/ConcertPersistence.h"
#include "database/Database.h"
#include "database/MoviePersistence.h"
#include "database/TvShowPersistence.h"

using namespace mediaelch;

namespace {

const DirectoryPath firstDirectory(QStringLiteral("/media/ä's"));
const DirectoryPath secondDirectory(QStringLiteral("/media/other"));
const FileList firstFiles{FilePath(QStringLiteral("/media/ä's/part1.mkv")),
    FilePath(QStringLiteral("/media/ä's/part2.mkv"))};
const FileList replacementFiles{FilePath(QStringLiteral("/media/ä's/renamed.mkv"))};
const QString nfo = QStringLiteral("<title>ä's 日本語</title>");

Subtitle* addSubtitle(Movie& movie, const QStringList& files, const QString& language, bool forced)
{
    auto* subtitle = new Subtitle(&movie);
    subtitle->setFiles(files);
    subtitle->setLanguage(language);
    subtitle->setForced(forced);
    movie.addSubtitle(subtitle);
    return subtitle;
}

} // namespace

TEST_CASE("Movies round trip metadata, files, labels, artwork and subtitles", "[database][persistence]")
{
    const bool imagesPresent = GENERATE(false, true);
    const bool emptyUpdate = GENERATE(false, true);
    Database database{QStringLiteral(":memory:")};
    MoviePersistence persistence{database};
    QObject owner;
    CHECK(persistence.moviesInDirectory(firstDirectory, &owner).isEmpty());
    Movie movie{firstFiles.toStringList()};
    movie.setNfoContent(nfo);
    movie.setInSeparateFolder(imagesPresent);
    movie.setDiscType(DiscType::BluRay);
    const auto modified = QDateTime::fromString("2026-10-02T12:30:00", Qt::ISODate);
    movie.setFileLastModified(modified);
    movie.setLabel(ColorLabel::Red);
    for (const auto type : Movie::imageTypes()) {
        movie.images().setHasImage(type, imagesPresent);
    }
    movie.images().setHasExtraFanarts(imagesPresent);
    auto* subtitle = addSubtitle(movie, {"one.idx", "one.sub"}, "de", true);
    persistence.addMovie(&movie, firstDirectory);
    REQUIRE(movie.databaseId().isValid());
    CHECK(persistence.moviesInDirectory(secondDirectory, &owner).isEmpty());
    auto movies = persistence.moviesInDirectory(firstDirectory, &owner);
    REQUIRE(movies.size() == 1);
    auto* loaded = movies.first();
    CHECK(loaded->parent() == &owner);
    CHECK(loaded->databaseId().toInt() == movie.databaseId().toInt());
    CHECK(loaded->files().toStringList() == firstFiles.toStringList());
    CHECK(loaded->nfoContent() == nfo);
    CHECK(loaded->fileLastModified() == modified);
    CHECK(loaded->inSeparateFolder() == imagesPresent);
    CHECK(loaded->discType() == DiscType::BluRay);
    CHECK(loaded->label() == ColorLabel::Red);
    CHECK_FALSE(loaded->hasChanged());
    for (const auto type : Movie::imageTypes()) {
        CHECK(loaded->hasImage(type) == imagesPresent);
    }
    CHECK(loaded->images().hasExtraFanarts() == imagesPresent);
    REQUIRE(loaded->subtitles().size() == 1);
    CHECK((loaded->subtitles().first()->files() == QStringList{"one.idx", "one.sub"}));
    CHECK(loaded->subtitles().first()->forced());
    CHECK(loaded->subtitles().first()->language() == QStringLiteral("de"));
    CHECK_FALSE(loaded->subtitles().first()->changed());

    movie.setFiles(replacementFiles);
    for (const auto type : Movie::imageTypes()) {
        movie.images().setHasImage(type, !imagesPresent);
    }
    movie.images().setHasExtraFanarts(!imagesPresent);
    movie.setInSeparateFolder(!imagesPresent);
    movie.setDiscType(DiscType::Dvd);
    const auto updatedModified = modified.addDays(1);
    movie.setFileLastModified(updatedModified);
    const QString updatedNfo = emptyUpdate ? QString() : QStringLiteral("<updated>ä's 日本語</updated>");
    movie.setNfoContent(updatedNfo);
    subtitle->setFiles({"new.srt"});
    subtitle->setLanguage(QString());
    subtitle->setForced(false);
    persistence.update(&movie);
    movies = persistence.moviesInDirectory(firstDirectory, &owner);
    REQUIRE(movies.size() == 1);
    CHECK(movies.first()->nfoContent() == updatedNfo);
    CHECK(movies.first()->fileLastModified() == updatedModified);
    CHECK(movies.first()->discType() == DiscType::Dvd);
    CHECK(movies.first()->inSeparateFolder() == !imagesPresent);
    for (const auto type : Movie::imageTypes()) {
        CHECK(movies.first()->hasImage(type) == !imagesPresent);
    }
    CHECK(movies.first()->images().hasExtraFanarts() == !imagesPresent);
    CHECK(movies.first()->files().toStringList() == replacementFiles.toStringList());
    REQUIRE(movies.first()->subtitles().size() == 1);
    CHECK(movies.first()->subtitles().first()->files() == QStringList{"new.srt"});
    CHECK(movies.first()->subtitles().first()->language().isEmpty());
    CHECK_FALSE(movies.first()->subtitles().first()->forced());
    CHECK(test::rowCount(persistence.db(), "movieFiles") == 1);
    CHECK(test::rowCount(persistence.db(), "movieSubtitles") == 1);
}

TEST_CASE("Movie cleanup removes dependent rows and preserves other directories and labels", "[database][persistence]")
{
    const bool clearAll = GENERATE(false, true);
    Database database{QStringLiteral(":memory:")};
    MoviePersistence persistence{database};
    Movie first{firstFiles.toStringList()};
    Movie second{replacementFiles.toStringList()};
    addSubtitle(first, {"first.srt"}, "en", false);
    addSubtitle(second, {"second.srt"}, "de", true);
    persistence.addMovie(&first, firstDirectory);
    persistence.addMovie(&second, secondDirectory);
    if (clearAll) {
        persistence.clearAllMovies();
    } else {
        persistence.clearMoviesInDirectory(firstDirectory);
        persistence.clearMoviesInDirectory(firstDirectory);
    }
    QObject owner;
    CHECK(persistence.moviesInDirectory(firstDirectory, &owner).isEmpty());
    CHECK(persistence.moviesInDirectory(secondDirectory, &owner).size() == (clearAll ? 0 : 1));
    CHECK(test::rowCount(database.db(), "movieFiles") == (clearAll ? 0 : 1));
    CHECK(test::rowCount(database.db(), "movieSubtitles") == (clearAll ? 0 : 1));
    CHECK(test::rowCount(database.db(), "labels") == 3);
    if (clearAll) {
        persistence.addMovie(&first, firstDirectory);
        CHECK(first.databaseId().toInt() == 1);
    }
}

TEST_CASE("Concerts round trip and replace their cached file lists", "[database][persistence]")
{
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    ConcertPersistence persistence{database, &owner};
    CHECK(persistence.concertsInDirectory(firstDirectory).isEmpty());
    Concert concert{firstFiles};
    concert.setNfoContent(nfo);
    concert.setInSeparateFolder(true);
    persistence.add(&concert, firstDirectory);
    CHECK(persistence.concertsInDirectory(secondDirectory).isEmpty());
    auto concerts = persistence.concertsInDirectory(firstDirectory);
    REQUIRE(concerts.size() == 1);
    CHECK(concerts.first()->parent() == &owner);
    CHECK(concerts.first()->databaseId().toInt() == concert.databaseId().toInt());
    CHECK(concerts.first()->nfoContent() == nfo);
    CHECK(concerts.first()->inSeparateFolder());
    CHECK(concerts.first()->files().toStringList() == firstFiles.toStringList());
    const QString updatedNfo = GENERATE(QString(), QStringLiteral("<updated>ä's 日本語</updated>"));
    concert.setNfoContent(updatedNfo);
    concert.setFiles(replacementFiles);
    persistence.update(&concert);
    concerts = persistence.concertsInDirectory(firstDirectory);
    REQUIRE(concerts.size() == 1);
    CHECK(concerts.first()->nfoContent() == updatedNfo);
    CHECK(concerts.first()->files().toStringList() == replacementFiles.toStringList());
    CHECK(test::rowCount(persistence.db(), "concertFiles") == 1);
}

TEST_CASE("Concert cleanup preserves unrelated directories", "[database][persistence]")
{
    const bool clearAll = GENERATE(false, true);
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    ConcertPersistence persistence{database, &owner};
    Concert first{firstFiles};
    Concert second{replacementFiles};
    persistence.add(&first, firstDirectory);
    persistence.add(&second, secondDirectory);
    if (clearAll) {
        persistence.clearAllConcerts();
    } else {
        persistence.clearConcertsInDirectory(firstDirectory);
        persistence.clearConcertsInDirectory(firstDirectory);
    }
    CHECK(persistence.concertsInDirectory(firstDirectory).isEmpty());
    CHECK(persistence.concertsInDirectory(secondDirectory).size() == (clearAll ? 0 : 1));
    CHECK(test::rowCount(database.db(), "concertFiles") == (clearAll ? 0 : 1));
    if (clearAll) {
        persistence.add(&first, firstDirectory);
        CHECK(first.databaseId().toInt() == 1);
    }
}

TEST_CASE("TV shows and episodes round trip without crossing show boundaries", "[database][persistence]")
{
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    TvShowPersistence persistence{database, &owner};
    CHECK(persistence.showCount(firstDirectory) == 0);
    CHECK(persistence.episodeCount() == 0);
    CHECK(persistence.showsInDirectory(firstDirectory).isEmpty());
    CHECK(persistence.episodes(DatabaseId(123)).isEmpty());
    TvShow show{firstDirectory};
    TvShow other{secondDirectory};
    show.setNfoContent(nfo);
    persistence.add(&show, firstDirectory);
    persistence.add(&other, secondDirectory);
    show.setShowMissingEpisodes(true, false);
    show.setHideSpecialsInMissingEpisodes(true, false);
    show.setEpisodeGuideUrl("https://example.test/ä");
    show.setTmdbId(TmdbId(123));
    show.setNfoContent(QStringLiteral("<updated>日本語</updated>"));
    persistence.update(&show);
    auto shows = persistence.showsInDirectory(firstDirectory);
    REQUIRE(shows.size() == 1);
    CHECK(shows.first()->parent() == &owner);
    CHECK(shows.first()->dir() == firstDirectory);
    CHECK(shows.first()->nfoContent() == show.nfoContent());
    CHECK(shows.first()->databaseId().toInt() == show.databaseId().toInt());
    CHECK(shows.first()->showMissingEpisodes());
    CHECK(shows.first()->hideSpecialsInMissingEpisodes());
    CHECK(persistence.showCount(secondDirectory) == 1);
    QSqlQuery settings(database.db());
    REQUIRE(settings.exec("SELECT tmdbid, url FROM showsSettings WHERE tmdbid='123'"));
    REQUIRE(settings.next());
    CHECK(settings.value(1).toString() == show.episodeGuideUrl());
    settings.finish();
    for (const bool enabled : {false, true}) {
        persistence.setShowMissingEpisodes(&show, enabled);
        auto updatedShows = persistence.showsInDirectory(firstDirectory);
        REQUIRE(updatedShows.size() == 1);
        CHECK(updatedShows.first()->showMissingEpisodes() == enabled);
        CHECK(updatedShows.first()->hideSpecialsInMissingEpisodes());
    }

    // Re-adding cached shows must read existing settings without mutating another database.
    persistence.add(&show, firstDirectory);
    CHECK(show.showMissingEpisodes());
    CHECK(show.hideSpecialsInMissingEpisodes());

    TvShowEpisode episode{firstFiles};
    episode.setSeason(SeasonNumber(2));
    episode.setEpisode(EpisodeNumber(3));
    episode.setNfoContent(nfo);
    persistence.add(&episode, firstDirectory, show.databaseId());
    auto episodes = persistence.episodes(show.databaseId());
    REQUIRE(episodes.size() == 1);
    CHECK(episodes.first()->nfoContent() == nfo);
    CHECK(episodes.first()->seasonNumber() == SeasonNumber(2));
    CHECK(episodes.first()->episodeNumber() == EpisodeNumber(3));
    CHECK(episodes.first()->databaseId().toInt() == episode.databaseId().toInt());
    CHECK(episodes.first()->files().toStringList() == firstFiles.toStringList());
    qDeleteAll(episodes);
    CHECK(persistence.episodes(other.databaseId()).isEmpty());
    episode.setFiles(replacementFiles);
    const QString updatedEpisodeNfo = GENERATE(QString(), QStringLiteral("<updated>ä's 日本語</updated>"));
    episode.setNfoContent(updatedEpisodeNfo);
    persistence.update(&episode);
    episodes = persistence.episodes(show.databaseId());
    REQUIRE(episodes.size() == 1);
    CHECK(episodes.first()->nfoContent() == updatedEpisodeNfo);
    CHECK(episodes.first()->files().toStringList() == replacementFiles.toStringList());
    qDeleteAll(episodes);
    CHECK(persistence.episodeCount() == 1);
}

TEST_CASE("TV cleanup scopes episode deletion and retains show preferences", "[database][persistence]")
{
    const int mode = GENERATE(0, 1, 2);
    CAPTURE(mode);
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    TvShowPersistence persistence{database, &owner};
    TvShow first{firstDirectory};
    TvShow second{firstDirectory.subDir("second-show")};
    TvShow third{secondDirectory};
    persistence.add(&first, firstDirectory);
    persistence.add(&second, firstDirectory);
    persistence.add(&third, secondDirectory);
    for (auto* show : {&first, &second, &third}) {
        TvShowEpisode episode{firstFiles};
        episode.setSeason(SeasonNumber(1));
        episode.setEpisode(EpisodeNumber(1));
        persistence.add(&episode, show == &third ? secondDirectory : firstDirectory, show->databaseId());
        persistence.addEpisodeToShowList(&episode, persistence.showsSettingsId(show), TmdbId(123));
    }
    if (mode == 0) {
        persistence.clearTvShowInDirectory(first.dir());
        persistence.clearTvShowInDirectory(first.dir());
    } else if (mode == 1) {
        persistence.clearTvShowsInDirectory(firstDirectory);
    } else {
        persistence.clearAllTvShows();
    }
    CHECK(persistence.showCount(firstDirectory) == (mode == 0 ? 1 : 0));
    CHECK(persistence.showCount(secondDirectory) == (mode == 2 ? 0 : 1));
    CHECK(persistence.episodeCount() == (mode == 0 ? 2 : mode == 1 ? 1 : 0));
    CHECK(test::rowCount(persistence.db(), "episodeFiles") == (mode == 0 ? 4 : mode == 1 ? 2 : 0));
    CHECK(test::rowCount(persistence.db(), "showsSettings") == 3);
    CHECK(test::rowCount(persistence.db(), "showsEpisodes") == 3);
    if (mode == 2) {
        persistence.add(&first, firstDirectory);
        CHECK(first.databaseId().toInt() == 1);
    }
}

TEST_CASE("Missing episode lists expose XML and clean up only the refreshed show", "[database][persistence]")
{
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    TvShowPersistence persistence{database, &owner};
    TvShow first{firstDirectory};
    TvShow second{secondDirectory};
    const auto firstId = persistence.showsSettingsId(&first);
    const auto secondId = persistence.showsSettingsId(&second);
    TvShowEpisode episode;
    episode.setSeason(SeasonNumber(0));
    episode.setEpisode(EpisodeNumber(1));
    episode.setTitle("日本語");
    persistence.addEpisodeToShowList(&episode, firstId, TmdbId(123));
    persistence.addEpisodeToShowList(&episode, secondId, TmdbId(123));
    auto episodes = persistence.showsEpisodes(&first);
    REQUIRE(episodes.size() == 1);
    CHECK(episodes.first()->parent() == &first);
    CHECK(episodes.first()->seasonNumber() == SeasonNumber(0));
    CHECK(episodes.first()->episodeNumber() == EpisodeNumber(1));
    CHECK(episodes.first()->nfoContent().contains(QStringLiteral("日本語")));
    persistence.clearEpisodeList(firstId);
    persistence.cleanUpEpisodeList(firstId);
    CHECK(persistence.showsEpisodes(&first).isEmpty());
    CHECK(persistence.showsEpisodes(&second).size() == 1);
    episode.setSeason(SeasonNumber::NoSeason);
    persistence.addEpisodeToShowList(&episode, firstId, TmdbId::NoId);
    CHECK(persistence.showsEpisodes(&first).isEmpty());
}

TEST_CASE("A failed show-settings insert does not return a usable ID", "[database][persistence]")
{
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    TvShowPersistence persistence{database, &owner};
    test::executeSql(database.db(),
        "CREATE TEMP TRIGGER fail_settings BEFORE INSERT ON showsSettings "
        "BEGIN SELECT RAISE(ABORT, 'test failure'); END");
    TvShow show{firstDirectory};
    CHECK_FALSE(persistence.showsSettingsId(&show).isValid());
    CHECK(test::rowCount(database.db(), "showsSettings") == 0);
}

TEST_CASE("Failed cached-episode writes retain the previous episode", "[database][persistence]")
{
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    TvShowPersistence persistence{database, &owner};
    TvShow show{firstDirectory};
    const auto id = persistence.showsSettingsId(&show);
    TvShowEpisode episode;
    episode.setSeason(SeasonNumber(1));
    episode.setEpisode(EpisodeNumber(1));
    persistence.addEpisodeToShowList(&episode, id, TmdbId(123));
    test::executeSql(database.db(),
        "CREATE TEMP TRIGGER fail_cache BEFORE UPDATE ON showsEpisodes "
        "BEGIN SELECT RAISE(ABORT, 'test failure'); END");
    episode.setEpisode(EpisodeNumber(2));
    persistence.addEpisodeToShowList(&episode, id, TmdbId(123));
    auto episodes = persistence.showsEpisodes(&show);
    REQUIRE(episodes.size() == 1);
    CHECK(episodes.first()->episodeNumber() == EpisodeNumber(1));
    test::executeSql(database.db(),
        "CREATE TEMP TRIGGER fail_cache_insert BEFORE INSERT ON showsEpisodes "
        "BEGIN SELECT RAISE(ABORT, 'test failure'); END");
    persistence.addEpisodeToShowList(&episode, id, TmdbId(456));
    CHECK(persistence.showsEpisodes(&show).size() == 1);
}

TEST_CASE("Episode-list replacement rolls back every failed refresh stage", "[database][persistence]")
{
    const QString stage = GENERATE("mark", "update", "insert", "cleanup", "success", "empty");
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    TvShowPersistence persistence{database, &owner};
    TvShow show{firstDirectory};
    const auto id = persistence.showsSettingsId(&show);
    TvShowEpisode retained;
    retained.setSeason(SeasonNumber(1));
    retained.setEpisode(EpisodeNumber(1));
    retained.setTmdbId(TmdbId(123));
    retained.setTitle("old");
    TvShowEpisode removed;
    removed.setSeason(SeasonNumber(1));
    removed.setEpisode(EpisodeNumber(2));
    removed.setTmdbId(TmdbId(456));
    REQUIRE(persistence.replaceEpisodeList(id, {&retained, &removed}));
    retained.setTitle("new");
    TvShowEpisode added;
    added.setSeason(SeasonNumber(1));
    added.setEpisode(EpisodeNumber(3));
    added.setTmdbId(TmdbId(789));
    if (stage != "success" && stage != "empty") {
        const QString event = stage == "insert" ? "INSERT" : stage == "cleanup" ? "DELETE" : "UPDATE";
        const QString condition = stage == "mark"     ? " WHEN NEW.updated=0"
                                  : stage == "update" ? " WHEN NEW.updated=1"
                                                      : "";
        test::executeSql(database.db(),
            "CREATE TRIGGER fail_refresh BEFORE " + event + " ON showsEpisodes" + condition
                + " BEGIN SELECT RAISE(ABORT,'test failure'); END");
    }
    const bool success = persistence.replaceEpisodeList(
        id, stage == "empty" ? QVector<TvShowEpisode*>{} : QVector<TvShowEpisode*>{&retained, &added});
    CHECK(success == (stage == "success" || stage == "empty"));
    auto episodes = persistence.showsEpisodes(&show);
    if (stage == "empty") {
        CHECK(episodes.isEmpty());
    } else {
        REQUIRE(episodes.size() == 2);
        for (auto* episode : episodes) {
            if (episode->episodeNumber() == EpisodeNumber(1)) {
                CHECK(episode->nfoContent().contains(stage == "success" ? "new" : "old"));
            } else {
                CHECK(episode->episodeNumber() == EpisodeNumber(stage == "success" ? 3 : 2));
            }
        }
    }
}
