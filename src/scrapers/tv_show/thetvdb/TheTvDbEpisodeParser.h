#pragma once

#include "tv_shows/TvShowEpisode.h"

#include <QJsonObject>
#include <QString>

namespace mediaelch {
namespace scraper {

class TheTvDbEpisodeParser
{
public:
    TheTvDbEpisodeParser(TvShowEpisode& episode) : m_episode{episode} {}

    void parseInfos(const QString& json);
    void parseInfos(const QJsonObject& episodeObj);
    void parseIdFromSeason(const QString& json);

private:
    TvShowEpisode& m_episode;
};

} // namespace scraper
} // namespace mediaelch
