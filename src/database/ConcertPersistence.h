#pragma once

#include "media/Path.h"

#include <QObject>
#include <QSqlDatabase>

class Concert;
class Database;

namespace mediaelch {

class ConcertPersistence
{
public:
    /// Optional parent for loaded objects; must outlive this persistence instance.
    explicit ConcertPersistence(Database& db, QObject* loadedObjectParent = nullptr);
    virtual ~ConcertPersistence() = default;
    QSqlDatabase db();

    virtual void clearAllConcerts();
    virtual void clearConcertsInDirectory(DirectoryPath path);
    virtual void add(Concert* concert, DirectoryPath path);
    virtual void update(Concert* concert);
    virtual QVector<Concert*> concertsInDirectory(DirectoryPath path);

private:
    Database& m_db;
    QObject* m_loadedObjectParent = nullptr;
};

} // namespace mediaelch
