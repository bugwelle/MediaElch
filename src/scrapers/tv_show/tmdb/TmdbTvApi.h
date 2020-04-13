#pragma once

#include "data/Locale.h"
#include "data/TmdbId.h"
#include "globals/ScraperInfos.h"
#include "network/NetworkReplyWatcher.h"
#include "network/WebsiteCache.h"
#include "tv_shows/EpisodeNumber.h"
#include "tv_shows/SeasonNumber.h"
#include "tv_shows/SeasonOrder.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

namespace mediaelch {
namespace scraper {

struct TmdbTvApiConfiguration
{
    QString imageBaseUrl = "http://image.tmdb.org/t/p/";
    QString imageSecureBaseUrl = "https://image.tmdb.org/t/p/";
    QStringList backdropSizes;
    QStringList logoSizes;
    QStringList posterSizes;
    QStringList profileSizes;
    QStringList stillSizes;

    static TmdbTvApiConfiguration from(QJsonDocument doc);
};

/// \brief API interface for TheTvDb
class TmdbTvApi : public QObject
{
    Q_OBJECT

public:
    explicit TmdbTvApi(QObject* parent = nullptr);
    ~TmdbTvApi() override = default;

    void initialize();
    bool isInitialized() const;

public:
    const TmdbTvApiConfiguration& config() const;

public:
    using ApiCallback = std::function<void(QJsonDocument)>;

    void sendGetRequest(const Locale& locale, const QUrl& url, ApiCallback callback);

    void searchForShow(const Locale& locale, const QString& query, bool includeAdult, ApiCallback callback);
    void loadShowInfos(const Locale& locale, const TmdbId& id, ApiCallback callback);

    void loadEpisode(const Locale& locale,
        const TmdbId& showId,
        SeasonNumber season,
        EpisodeNumber episode,
        ApiCallback callback);

signals:
    void initialized(bool wasSuccessful);

public:
    QUrl makeImageUrl(const QString& suffix) const;
    QUrl makeApiUrl(const QString& suffix, const Locale& locale, QUrlQuery query) const;

private:
    QUrl getShowUrl(const TmdbId& id, const Locale& locale) const;
    QUrl getShowSearchUrl(const QString& searchStr, const Locale& locale, bool includeAdult) const;
    QUrl getEpisodeUrl(const TmdbId& showId, SeasonNumber season, EpisodeNumber episode, const Locale& locale) const;

    QString apiKey() const;

private:
    const QString m_language;
    QNetworkAccessManager m_qnam;
    WebsiteCache m_cache;
    TmdbTvApiConfiguration m_config;
    bool m_isInitialized;
};

} // namespace scraper
} // namespace mediaelch
