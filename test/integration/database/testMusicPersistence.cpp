#include "test/integration/database/DatabaseTestHelpers.h"

#include "data/music/Album.h"
#include "data/music/Artist.h"
#include "database/Database.h"
#include "database/MusicPersistence.h"

using namespace mediaelch;

TEST_CASE("Music persistence restores artists, albums and their relationships", "[database][persistence][music]")
{
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    MusicPersistence persistence{database, &owner};
    const DirectoryPath root(QStringLiteral("/music/ä's"));
    const DirectoryPath artistPath = root.subDir("日本語");
    CHECK(persistence.artistsInDirectory(root).isEmpty());
    Artist artist{artistPath};
    Artist other{root.subDir("other")};
    artist.setNfoContent(QStringLiteral("<artist>initial ä</artist>"));
    persistence.add(&artist, root);
    persistence.add(&other, root);
    REQUIRE(artist.databaseId().isValid());
    REQUIRE(other.databaseId().toInt() != artist.databaseId().toInt());
    CHECK(persistence.albums(&artist).isEmpty());
    auto initialArtists = persistence.artistsInDirectory(root);
    REQUIRE(initialArtists.size() == 2);
    bool foundInitial = false;
    for (const auto* item : initialArtists) {
        if (item->databaseId().toInt() == artist.databaseId().toInt()) {
            CHECK(item->nfoContent() == artist.nfoContent());
            foundInitial = true;
        }
    }
    REQUIRE(foundInitial);
    artist.setNfoContent(QStringLiteral("<artist>日本語</artist>"));
    persistence.update(&artist);
    Album album{artistPath.subDir("album's")};
    album.setArtistObj(&artist);
    album.setNfoContent(QStringLiteral("<album>initial ä</album>"));
    persistence.add(&album, root);
    REQUIRE(album.databaseId().isValid());
    auto initialAlbums = persistence.albums(&artist);
    REQUIRE(initialAlbums.size() == 1);
    CHECK(initialAlbums.first()->nfoContent() == album.nfoContent());
    album.setNfoContent(QStringLiteral("<album>ä's 日本語</album>"));
    persistence.update(&album);
    auto artists = persistence.artistsInDirectory(root);
    REQUIRE(artists.size() == 2);
    Artist* loaded = nullptr;
    for (auto* item : artists) {
        CHECK(item->parent() == &owner);
        if (item->databaseId().toInt() == artist.databaseId().toInt()) {
            loaded = item;
        } else {
            CHECK(item->nfoContent().isEmpty());
        }
    }
    REQUIRE(loaded != nullptr);
    CHECK(loaded->path() == artistPath);
    CHECK(loaded->nfoContent() == artist.nfoContent());
    auto albums = persistence.albums(loaded);
    REQUIRE(albums.size() == 1);
    CHECK(albums.first()->parent() == &owner);
    CHECK(albums.first()->path() == album.path());
    CHECK(albums.first()->databaseId().toInt() == album.databaseId().toInt());
    CHECK(albums.first()->nfoContent() == album.nfoContent());
    CHECK(albums.first()->artistObj() == loaded);
    CHECK(loaded->albums() == albums);
    CHECK(persistence.albums(&other).isEmpty());
    CHECK(persistence.artistsInDirectory(root.subDir("absent")).isEmpty());
    artist.setNfoContent(QString());
    album.setNfoContent(QString());
    persistence.update(&artist);
    persistence.update(&album);
    auto updated = persistence.artistsInDirectory(root);
    for (auto* item : updated) {
        CHECK(item->nfoContent().isEmpty());
    }
    auto emptyAlbums = persistence.albums(&artist);
    REQUIRE(emptyAlbums.size() == 1);
    CHECK(emptyAlbums.first()->nfoContent().isEmpty());
    CHECK(test::rowCount(persistence.db(), "artists") == 2);
    CHECK(test::rowCount(persistence.db(), "albums") == 1);
}

TEST_CASE("Music cleanup respects roots and cascades artist removal to albums", "[database][persistence][music]")
{
    // All four public cleanup operations, including their different effects on artists.
    const int mode = GENERATE(0, 1, 2, 3);
    CAPTURE(mode);
    Database database{QStringLiteral(":memory:")};
    QObject owner;
    MusicPersistence persistence{database, &owner};
    const DirectoryPath firstRoot(QStringLiteral("/music/first"));
    const DirectoryPath secondRoot(QStringLiteral("/music/second"));
    Artist first{firstRoot.subDir("artist")};
    Artist second{secondRoot.subDir("artist")};
    persistence.add(&first, firstRoot);
    persistence.add(&second, secondRoot);
    Album firstAlbum{first.path().subDir("album")};
    Album secondAlbum{second.path().subDir("album")};
    firstAlbum.setArtistObj(&first);
    secondAlbum.setArtistObj(&second);
    persistence.add(&firstAlbum, firstRoot);
    persistence.add(&secondAlbum, secondRoot);
    if (mode == 0) {
        persistence.clearAlbumsInDirectory(firstRoot);
        persistence.clearAlbumsInDirectory(firstRoot);
    } else if (mode == 1) {
        persistence.clearAllAlbums();
    } else if (mode == 2) {
        persistence.clearArtistsInDirectory(firstRoot);
        persistence.clearArtistsInDirectory(firstRoot);
    } else {
        persistence.clearAllArtists();
    }
    CHECK(test::rowCount(database.db(), "artists") == (mode < 2 ? 2 : mode == 2 ? 1 : 0));
    CHECK(test::rowCount(database.db(), "albums") == (mode == 0 || mode == 2 ? 1 : 0));
    CHECK(persistence.albums(&first).isEmpty());
    CHECK(persistence.albums(&second).size() == (mode == 0 || mode == 2 ? 1 : 0));
    CHECK(persistence.artistsInDirectory(firstRoot).size() == (mode < 2 ? 1 : 0));
    CHECK(persistence.artistsInDirectory(secondRoot).size() == (mode == 3 ? 0 : 1));
    if (mode == 3) {
        persistence.add(&first, firstRoot);
        CHECK(first.databaseId().toInt() == 1);
    }
    if (mode == 1 || mode == 3) {
        persistence.add(&firstAlbum, firstRoot);
        CHECK(firstAlbum.databaseId().toInt() == 1);
    }
}
