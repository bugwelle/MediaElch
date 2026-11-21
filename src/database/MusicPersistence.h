#pragma once

#include "media/Path.h"

#include <QObject>
#include <QSqlDatabase>

class Artist;
class Album;
class Database;

namespace mediaelch {

class MusicPersistence
{
public:
    /// Optional parent for loaded objects; must outlive this persistence instance.
    explicit MusicPersistence(Database& db, QObject* loadedObjectParent = nullptr);
    virtual ~MusicPersistence() = default;
    QSqlDatabase db();

    virtual void clearAllArtists();
    virtual void clearArtistsInDirectory(DirectoryPath path);
    virtual void add(Artist* artist, DirectoryPath path);
    virtual void update(Artist* artist);
    virtual QVector<Artist*> artistsInDirectory(DirectoryPath path);

    virtual void clearAllAlbums();
    virtual void clearAlbumsInDirectory(DirectoryPath path);
    virtual void add(Album* album, DirectoryPath path);
    virtual void update(Album* album);
    virtual QVector<Album*> albums(Artist* artist);

private:
    Database& m_db;
    QObject* m_loadedObjectParent = nullptr;
};

} // namespace mediaelch
