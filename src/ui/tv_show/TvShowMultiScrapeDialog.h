#pragma once

#include "globals/DownloadManager.h"
#include "scrapers/tv_show/TvScraper.h"
#include "tv_shows/TvShow.h"
#include "tv_shows/TvShowEpisode.h"

#include <QDialog>
#include <QPointer>
#include <QQueue>

namespace Ui {
class TvShowMultiScrapeDialog;
}

class TvShowMultiScrapeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TvShowMultiScrapeDialog(QWidget* parent = nullptr);
    ~TvShowMultiScrapeDialog() override;

    QVector<TvShow*> shows() const;
    void setShows(const QVector<TvShow*>& shows);

    QVector<TvShowEpisode*> episodes() const;
    void setEpisodes(const QVector<TvShowEpisode*>& episodes);

public slots:
    int exec() override;
    void reject() override;
    void accept() override;

private slots:
    void onShowInfoToggled();
    void onEpisodeInfoToggled();
    void onChkAllShowInfosToggled();
    void onChkAllEpisodeInfosToggled();
    void onStartScraping();
    void onScrapingFinished();
    void onSearchFinished(mediaelch::scraper::ShowSearchJob* searchJob);
    void scrapeNext();
    void onInfoLoadDone(TvShow* show, QSet<ShowScraperInfo> details);
    void onEpisodeLoadDone();
    void onLoadDone(TvShow* show, QMap<ImageType, QVector<Poster>> posters);
    void onDownloadFinished(DownloadManagerElement elem);
    void onDownloadsFinished();

    void onScraperChanged(int index);
    void onLanguageChanged(int index);
    void onSeasonOrderChanged(int index);

private:
    Ui::TvShowMultiScrapeDialog* ui;
    QVector<TvShow*> m_shows;
    QVector<TvShowEpisode*> m_episodes;
    bool m_executed;
    SeasonOrder m_seasonOrder = SeasonOrder::Aired;
    QSet<ShowScraperInfo> m_showDetailsToLoad;
    QSet<EpisodeScraperInfo> m_episodeDetailsToLoad;
    QQueue<TvShow*> m_showQueue;
    QQueue<TvShowEpisode*> m_episodeQueue;
    QPointer<TvShow> m_currentShow;
    QPointer<TvShowEpisode> m_currentEpisode;
    mediaelch::scraper::TvScraper* m_currentScraper;
    mediaelch::Locale m_locale = mediaelch::Locale::English;
    DownloadManager* m_downloadManager;
    QMap<QString, TvDbId> m_showIds;

private:
    void setCheckBoxesEnabled();
    void setupLanguageDropdown();
    void setupScraperDropdown();
    void setupSeasonOrderComboBox();

    void addDownload(ImageType imageType, QUrl url, TvShow* show, SeasonNumber season = SeasonNumber::NoSeason);
    void addDownload(ImageType imageType, QUrl url, TvShow* show, Actor* actor);
    void addDownload(ImageType imageType, QUrl url, TvShowEpisode* episode);

    void showError(const QString& message);
};
