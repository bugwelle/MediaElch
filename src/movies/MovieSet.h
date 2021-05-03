#pragma once

#include "data/TmdbId.h"

#include <QString>

/// \brief   Represents a movie collection (aka. set).
/// \details This struct is used in "Movie" and only stores basic meta data.
struct MovieSetDetails
{
    /// A collection's TmdbId, e.g. 1241 for Harry Potter.
    /// Used for getting data from TMDb, e.g.
    /// themoviedb.org/movie/1241 which redirects to
    /// themoviedb.org/collection/1241-harry-potter-collection
    TmdbId tmdbId{TmdbId::NoId};
    QString name;
    QString overview;
};
