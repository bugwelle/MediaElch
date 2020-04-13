#pragma once

#include "data/ImdbId.h"
#include "data/Locale.h"
#include "globals/ScraperInfos.h"
#include "network/NetworkReplyWatcher.h"
#include "network/WebsiteCache.h"
#include "tv_shows/SeasonNumber.h"
#include "tv_shows/SeasonOrder.h"

#include <QByteArray>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QUrl>

#include <functional>

namespace mediaelch {
namespace scraper {

/// \brief API interface for TheTvDb
class ImdbTvApi : public QObject
{
    Q_OBJECT

public:
    explicit ImdbTvApi(QObject* parent = nullptr);
    ~ImdbTvApi() override = default;

    void initialize();
    bool isInitialized() const;

public:
    using ApiCallback = std::function<void(QString)>;

    void sendGetRequest(const Locale& locale, const QUrl& url, ApiCallback callback);

    void searchForShow(const Locale& locale, const QString& query, ApiCallback callback);
    void loadShowInfos(const Locale& locale, const ImdbId& id, ApiCallback callback);

    void loadEpisode(const Locale& locale, const ImdbId& episodeId, ApiCallback callback);

signals:
    void initialized();

public:
    static QUrl makeFullUrl(const QString& suffix);
    static QUrl makeFullAssetUrl(const QString& suffix);

private:
    /// \brief Add neccassaray headers for TheTvDb to the request object.
    /// Token must exist.
    /// \see ImdbTvApi::obtainJsonWebToken
    void addHeadersToRequest(const Locale& locale, QNetworkRequest& request);

    QUrl getShowUrl(const ImdbId& id) const;
    QUrl getShowSearchUrl(const QString& searchStr) const;
    QUrl getEpisodeUrl(const ImdbId& episodeId) const;

private:
    const QString m_language;
    QNetworkAccessManager m_qnam;
    WebsiteCache m_cache;
};

} // namespace scraper
} // namespace mediaelch