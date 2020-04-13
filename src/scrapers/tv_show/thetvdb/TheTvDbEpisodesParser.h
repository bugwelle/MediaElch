#pragma once

#include "scrapers/tv_show/thetvdb/TheTvDbApi.h"

#include <QString>
#include <memory>

class TvShowEpisode;

namespace mediaelch {
namespace scraper {

class TheTvDbEpisodesParser
{
public:
    TheTvDbEpisodesParser() {}

    /// \brief Parses episodes from the json and stores them in this object.
    /// \param json TheTvDb api JSON response (episode page)
    /// \param parentForEpisodes QObject parent parameter for new TvShowEpisodes.
    /// \param episodeCallback Called when an episode is parsed. Can be used to
    ///                        delete the generated episode and/or store the
    ///                        pointer.
    TheTvDbApi::Paginate
    parseEpisodes(const QString& json, QObject* parentForEpisodes, std::function<void(TvShowEpisode*)> episodeCallback);
};

} // namespace scraper
} // namespace mediaelch
