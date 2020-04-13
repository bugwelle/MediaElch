#include "TmdbTvApi.h"

#include "Version.h"
#include "data/ImdbId.h"
#include "globals/JsonRequest.h"
#include "network/NetworkRequest.h"
#include "tv_shows/TvDbId.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace mediaelch {
namespace scraper {

TmdbTvApi::TmdbTvApi(QObject* parent) : QObject(parent)
{
}

void TmdbTvApi::initialize()
{
    QUrl url(TmdbTvApi::makeApiUrl("/configuration", Locale::English, {}));
    QNetworkRequest request = network::jsonRequestWithDefaults(url);

    QNetworkReply* const reply = m_qnam.get(request);
    new NetworkReplyWatcher(this, reply);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        QString data{"{}"};
        if (reply->error() == QNetworkReply::NoError) {
            data = QString::fromUtf8(reply->readAll());
            m_isInitialized = true;
            m_config = TmdbTvApiConfiguration::from(QJsonDocument::fromJson(data.toUtf8()));

        } else {
            qWarning() << "[TmdbTvApi] Network Error:" << reply->errorString() << "for URL" << reply->url();
            m_isInitialized = false;
        }

        reply->deleteLater();

        emit initialized(m_isInitialized);
    });
}

bool TmdbTvApi::isInitialized() const
{
    return m_isInitialized;
}

const TmdbTvApiConfiguration& TmdbTvApi::config() const
{
    return m_config;
}

void TmdbTvApi::sendGetRequest(const Locale& locale, const QUrl& url, TmdbTvApi::ApiCallback callback)
{
    if (m_cache.hasValidElement(url, locale)) {
        // Do not immediately run the callback because classes higher up may
        // set up a Qt connection while the network request is running.
        QTimer::singleShot(0, [cb = std::move(callback), element = m_cache.getElement(url, locale)]() {
            cb(QJsonDocument::fromJson(element.toUtf8()));
        });
        return;
    }

    QNetworkRequest request = mediaelch::network::requestWithDefaults(url);
    QNetworkReply* reply(m_qnam.get(request));
    new NetworkReplyWatcher(this, reply);

    connect(reply, &QNetworkReply::finished, [reply, callback, locale, this]() {
        QString data;
        if (reply->error() == QNetworkReply::NoError) {
            data = QString::fromUtf8(reply->readAll());

            if (!data.isEmpty()) {
                m_cache.addElement(reply->url(), locale, data);
            }
        } else {
            qWarning() << "[TmdbTv][Api] Network Error:" << reply->errorString() << "for URL" << reply->url();
        }
        callback(QJsonDocument::fromJson(data.toUtf8()));
        reply->deleteLater();
    });
}

void TmdbTvApi::searchForShow(const Locale& locale,
    const QString& query,
    bool includeAdult,
    TmdbTvApi::ApiCallback callback)
{
    sendGetRequest(locale, getShowSearchUrl(query, locale, includeAdult), std::move(callback));
}

void TmdbTvApi::loadShowInfos(const Locale& locale, const TmdbId& id, TmdbTvApi::ApiCallback callback)
{
    sendGetRequest(locale, getShowUrl(id, locale), callback);
}

void TmdbTvApi::loadEpisode(const Locale& locale,
    const TmdbId& showId,
    SeasonNumber season,
    EpisodeNumber episode,
    ApiCallback callback)
{
    sendGetRequest(locale, getEpisodeUrl(showId, season, episode, locale), callback);
}

QUrl TmdbTvApi::makeApiUrl(const QString& suffix, const Locale& locale, QUrlQuery query) const
{
    query.addQueryItem("api_key", apiKey());
    query.addQueryItem("language", locale.toString('-'));

    return QStringLiteral("https://api.themoviedb.org/3%1?%2").arg(suffix, query.toString());
}

QUrl TmdbTvApi::makeImageUrl(const QString& suffix) const
{
    // TODO: Use image sizes
    return QUrl(config().imageSecureBaseUrl + "original" + suffix);
}

QUrl TmdbTvApi::getShowSearchUrl(const QString& searchStr, const Locale& locale, bool includeAdult) const
{
    QUrlQuery queries;
    // Special handling of certain ID types. TheMovieDb supports other IDs and not only
    // their TMDb IDs.
    if (TmdbId::isValidFormat(searchStr)) {
        return makeApiUrl(QStringLiteral("/tv/") + searchStr, locale, queries);
    }
    if (ImdbId::isValidFormat(searchStr)) {
        queries.addQueryItem("external_source", "imdb_id");
        return makeApiUrl(QStringLiteral("/find/") + searchStr, locale, queries);
    }
    if (TvDbId::isValidFormat(searchStr)) {
        queries.addQueryItem("external_source", "tvdb_id");
        return makeApiUrl(QStringLiteral("/find/") + searchStr, locale, queries);
    }

    queries.addQueryItem("page", "1"); // Only query first page as of now.
    queries.addQueryItem("query", searchStr);
    queries.addQueryItem("include_adult", includeAdult ? "true" : "false");
    return makeApiUrl("/search/tv", locale, queries);
}

QUrl TmdbTvApi::getShowUrl(const TmdbId& id, const Locale& locale) const
{
    QUrlQuery queries;
    // Instead of multiple HTTP requests, use just one for everything.
    queries.addQueryItem("append_to_response", "content_ratings,keywords,external_ids,images,credits");
    return makeApiUrl(QStringLiteral("/tv/") + id.toString(), locale, queries);
}

QUrl TmdbTvApi::getEpisodeUrl(const TmdbId& showId,
    SeasonNumber season,
    EpisodeNumber episode,
    const Locale& locale) const
{
    QString url = QStringLiteral("/tv/%1/season/{season_number}/episode/{episode_number}")
                      .arg(showId.toString(), season.toString(), episode.toString());
    return makeApiUrl(url, locale, {});
}

QString TmdbTvApi::apiKey() const
{
    // TheMovieTv API v3 key for MediaElch
    return QStringLiteral("5d832bdf69dcb884922381ab01548d5b");
}

TmdbTvApiConfiguration TmdbTvApiConfiguration::from(QJsonDocument doc)
{
    QJsonObject obj = doc.object();
    QJsonObject images = obj["images"].toObject();

    const auto assignStringUrl = [](QString& target, const QString& source) {
        if (source.isEmpty()) {
            return;
        }
        target = source;
        if (target.at(target.length() - 1) != '/') {
            // Ensure that it ends with a slash
            target += '/';
        }
    };

    TmdbTvApiConfiguration config;
    assignStringUrl(config.imageBaseUrl, images["base_url"].toString());
    assignStringUrl(config.imageSecureBaseUrl, images["secure_base_url"].toString());

    const auto assignStringArray = [](QStringList& target, const QJsonArray& array) {
        for (const QJsonValue& val : array) {
            target << val.toString();
        }
    };

    assignStringArray(config.backdropSizes, images["backdrop_sizes"].toArray());
    assignStringArray(config.logoSizes, images["logo_sizes"].toArray());
    assignStringArray(config.posterSizes, images["poster_sizes"].toArray());
    assignStringArray(config.profileSizes, images["profile_sizes"].toArray());
    assignStringArray(config.stillSizes, images["still_sizes"].toArray());

    return config;
}


} // namespace scraper
} // namespace mediaelch