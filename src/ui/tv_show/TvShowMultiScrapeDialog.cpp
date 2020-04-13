#include "TvShowMultiScrapeDialog.h"
#include "ui_TvShowMultiScrapeDialog.h"

#include <QDebug>
#include <utility>

#include "data/ImageCache.h"
#include "globals/Helper.h"
#include "globals/Manager.h"
#include "scrapers/tv_show/TvScraper.h"

using namespace mediaelch;

TvShowMultiScrapeDialog::TvShowMultiScrapeDialog(QWidget* parent) : QDialog(parent), ui(new Ui::TvShowMultiScrapeDialog)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    setWindowFlags((windowFlags() & ~Qt::WindowType_Mask) | Qt::Sheet);
#else
    setWindowFlags((windowFlags() & ~Qt::WindowType_Mask) | Qt::Dialog);
#endif

    QFont font = ui->itemCounter->font();
#ifdef Q_OS_WIN
    font.setPointSize(font.pointSize() - 1);
#else
    font.setPointSize(font.pointSize() - 2);
#endif
    ui->itemCounter->setFont(font);

    m_executed = false;
    m_currentShow = nullptr;
    m_currentEpisode = nullptr;

    ui->chkActors->setMyData(static_cast<int>(ShowScraperInfo::Actors));
    ui->chkBanner->setMyData(static_cast<int>(ShowScraperInfo::Banner));
    ui->chkCertification->setMyData(static_cast<int>(ShowScraperInfo::Certification));
    ui->chkFanart->setMyData(static_cast<int>(ShowScraperInfo::Fanart));
    ui->chkFirstAired->setMyData(static_cast<int>(ShowScraperInfo::FirstAired));
    ui->chkGenres->setMyData(static_cast<int>(ShowScraperInfo::Genres));
    ui->chkTags->setMyData(static_cast<int>(ShowScraperInfo::Tags));
    ui->chkNetwork->setMyData(static_cast<int>(ShowScraperInfo::Network));
    ui->chkOverview->setMyData(static_cast<int>(ShowScraperInfo::Overview));
    ui->chkPoster->setMyData(static_cast<int>(ShowScraperInfo::Poster));
    ui->chkRating->setMyData(static_cast<int>(ShowScraperInfo::Rating));
    ui->chkSeasonPoster->setMyData(static_cast<int>(ShowScraperInfo::SeasonPoster));
    ui->chkSeasonFanart->setMyData(static_cast<int>(ShowScraperInfo::SeasonBackdrop));
    ui->chkSeasonBanner->setMyData(static_cast<int>(ShowScraperInfo::SeasonBanner));
    ui->chkSeasonThumb->setMyData(static_cast<int>(ShowScraperInfo::SeasonThumb));
    ui->chkTitle->setMyData(static_cast<int>(ShowScraperInfo::Title));
    ui->chkExtraArts->setMyData(static_cast<int>(ShowScraperInfo::ExtraArts));
    ui->chkRuntime->setMyData(static_cast<int>(ShowScraperInfo::Runtime));
    ui->chkStatus->setMyData(static_cast<int>(ShowScraperInfo::Status));
    ui->chkThumb->setMyData(static_cast<int>(ShowScraperInfo::Thumb));

    ui->chkEpisodeActors->setMyData(static_cast<int>(EpisodeScraperInfo::Actors));
    ui->chkEpisodeCertification->setMyData(static_cast<int>(EpisodeScraperInfo::Certification));
    ui->chkEpisodeDirector->setMyData(static_cast<int>(EpisodeScraperInfo::Director));
    ui->chkEpisodeFirstAired->setMyData(static_cast<int>(EpisodeScraperInfo::FirstAired));
    ui->chkEpisodeNetwork->setMyData(static_cast<int>(EpisodeScraperInfo::Network));
    ui->chkEpisodeOverview->setMyData(static_cast<int>(EpisodeScraperInfo::Overview));
    ui->chkEpisodeRating->setMyData(static_cast<int>(EpisodeScraperInfo::Rating));
    ui->chkEpisodeThumbnail->setMyData(static_cast<int>(EpisodeScraperInfo::Thumbnail));
    ui->chkEpisodeTitle->setMyData(static_cast<int>(EpisodeScraperInfo::Title));
    ui->chkEpisodeWriter->setMyData(static_cast<int>(EpisodeScraperInfo::Writer));

    for (MyCheckBox* box : ui->showInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0) {
            connect(box, &QAbstractButton::clicked, this, &TvShowMultiScrapeDialog::onShowInfoToggled);
        }
    }

    for (MyCheckBox* box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0) {
            connect(box, &QAbstractButton::clicked, this, &TvShowMultiScrapeDialog::onEpisodeInfoToggled);
        }
    }

    connect(ui->chkUnCheckAll, &QAbstractButton::clicked, this, &TvShowMultiScrapeDialog::onChkAllShowInfosToggled);
    connect(ui->chkEpisodeUnCheckAll,
        &QAbstractButton::clicked,
        this,
        &TvShowMultiScrapeDialog::onChkAllEpisodeInfosToggled);

    connect(ui->btnStartScraping, &QAbstractButton::clicked, this, &TvShowMultiScrapeDialog::onStartScraping);

    auto indexChanged = elchOverload<int>(&QComboBox::currentIndexChanged);
    connect(ui->comboScraper, indexChanged, this, &TvShowMultiScrapeDialog::onScraperChanged);
    connect(ui->comboLanguage, indexChanged, this, &TvShowMultiScrapeDialog::onLanguageChanged);
    connect(ui->comboSeasonOrder, indexChanged, this, &TvShowMultiScrapeDialog::onSeasonOrderChanged);

    m_currentScraper = Manager::instance()->scrapers().tvScrapers().at(0);

    m_downloadManager = new DownloadManager(this);
    connect(m_downloadManager, &DownloadManager::sigElemDownloaded, this, &TvShowMultiScrapeDialog::onDownloadFinished);
    connect(
        m_downloadManager, &DownloadManager::allDownloadsFinished, this, &TvShowMultiScrapeDialog::onDownloadsFinished);
}

TvShowMultiScrapeDialog::~TvShowMultiScrapeDialog()
{
    delete ui;
}

QVector<TvShow*> TvShowMultiScrapeDialog::shows() const
{
    return m_shows;
}

void TvShowMultiScrapeDialog::setShows(const QVector<TvShow*>& shows)
{
    m_shows = shows;
}

QVector<TvShowEpisode*> TvShowMultiScrapeDialog::episodes() const
{
    return m_episodes;
}

void TvShowMultiScrapeDialog::setEpisodes(const QVector<TvShowEpisode*>& episodes)
{
    m_episodes = episodes;
}

int TvShowMultiScrapeDialog::exec()
{
    m_showQueue.clear();
    m_episodeQueue.clear();
    ui->itemCounter->setVisible(false);
    ui->btnCancel->setVisible(true);
    ui->btnClose->setVisible(false);
    ui->btnStartScraping->setVisible(true);
    ui->btnStartScraping->setEnabled(true);
    ui->chkAutoSave->setEnabled(true);
    ui->chkOnlyId->setEnabled(true);
    ui->comboSeasonOrder->setEnabled(true);
    ui->progressAll->setValue(0);
    ui->progressItem->setValue(0);
    ui->showInfosGroupBox->setEnabled(true);
    ui->episodeInfosGroupBox->setEnabled(true);
    ui->title->clear();
    m_currentEpisode = nullptr;
    m_currentShow = nullptr;
    m_showIds.clear();
    m_executed = true;
    setCheckBoxesEnabled();
    adjustSize();

    ui->chkAutoSave->setChecked(Settings::instance()->multiScrapeSaveEach());
    ui->chkOnlyId->setChecked(Settings::instance()->multiScrapeOnlyWithId());

    setupScraperDropdown();
    setupLanguageDropdown();
    setupSeasonOrderComboBox();

    return QDialog::exec();
}

void TvShowMultiScrapeDialog::accept()
{
    m_executed = false;
    Settings::instance()->setMultiScrapeOnlyWithId(ui->chkOnlyId->isChecked());
    Settings::instance()->setMultiScrapeSaveEach(ui->chkAutoSave->isChecked());
    Settings::instance()->saveSettings();
    QDialog::accept();
}

void TvShowMultiScrapeDialog::reject()
{
    m_downloadManager->abortDownloads();
    m_executed = false;

    m_showQueue.clear();
    m_episodeQueue.clear();

    Settings::instance()->setMultiScrapeOnlyWithId(ui->chkOnlyId->isChecked());
    Settings::instance()->setMultiScrapeSaveEach(ui->chkAutoSave->isChecked());
    Settings::instance()->saveSettings();
    QDialog::reject();
}

void TvShowMultiScrapeDialog::onShowInfoToggled()
{
    m_showDetailsToLoad.clear();
    bool allToggled = true;
    for (const auto box : ui->showInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->isEnabled() && box->isChecked() && box->myData().toInt() > 0) {
            m_showDetailsToLoad.insert(ShowScraperInfo(box->myData().toInt()));
        }
        if (box->isEnabled() && !box->isChecked() && box->myData().toInt() > 0) {
            allToggled = false;
        }
    }

    ui->chkUnCheckAll->setChecked(allToggled);
    ui->btnStartScraping->setEnabled(!m_episodeDetailsToLoad.isEmpty() || !m_showDetailsToLoad.isEmpty());
}

void TvShowMultiScrapeDialog::onEpisodeInfoToggled()
{
    m_episodeDetailsToLoad.clear();
    bool allToggled = true;
    for (const auto box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->isEnabled() && box->isChecked() && box->myData().toInt() > 0) {
            m_episodeDetailsToLoad.insert(EpisodeScraperInfo(box->myData().toInt()));
        }
        if (box->isEnabled() && !box->isChecked() && box->myData().toInt() > 0) {
            allToggled = false;
        }
    }

    ui->chkEpisodeUnCheckAll->setChecked(allToggled);
    ui->btnStartScraping->setEnabled(!m_episodeDetailsToLoad.isEmpty() || !m_showDetailsToLoad.isEmpty());
}

void TvShowMultiScrapeDialog::onChkAllShowInfosToggled()
{
    bool checked = ui->chkUnCheckAll->isChecked();
    for (MyCheckBox* box : ui->showInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0 && box->isEnabled()) {
            box->setChecked(checked);
        }
    }
    onShowInfoToggled();
}

void TvShowMultiScrapeDialog::onChkAllEpisodeInfosToggled()
{
    bool checked = ui->chkEpisodeUnCheckAll->isChecked();
    for (MyCheckBox* box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0 && box->isEnabled()) {
            box->setChecked(checked);
        }
    }
    onEpisodeInfoToggled();
}

void TvShowMultiScrapeDialog::setCheckBoxesEnabled()
{
    bool checked = ui->chkEpisodeUnCheckAll->isChecked();
    for (MyCheckBox* box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0 && box->isEnabled()) {
            box->setChecked(checked);
        }
    }
    onEpisodeInfoToggled();
}

void TvShowMultiScrapeDialog::onStartScraping()
{
    ui->showInfosGroupBox->setEnabled(false);
    ui->episodeInfosGroupBox->setEnabled(false);
    ui->btnStartScraping->setEnabled(false);
    ui->chkAutoSave->setEnabled(false);
    ui->chkOnlyId->setEnabled(false);
    ui->comboSeasonOrder->setEnabled(false);

    m_showQueue.append(m_shows.toList());
    m_episodeQueue.append(m_episodes.toList());

    ui->itemCounter->setText(QString("0/%1").arg(m_showQueue.count() + m_episodeQueue.count()));
    ui->itemCounter->setVisible(true);
    ui->progressAll->setMaximum(m_showQueue.count() + m_episodeQueue.count());
    scrapeNext();
}

void TvShowMultiScrapeDialog::scrapeNext()
{
    using namespace mediaelch;
    using namespace mediaelch::scraper;
    if (!m_executed) {
        return;
    }

    if ((m_currentShow != nullptr) && ui->chkAutoSave->isChecked()) {
        m_currentShow->saveData(Manager::instance()->mediaCenterInterfaceTvShow());
    }

    if ((m_currentEpisode != nullptr) && ui->chkAutoSave->isChecked()) {
        m_currentEpisode->saveData(Manager::instance()->mediaCenterInterfaceTvShow());
    }

    if (m_showQueue.isEmpty() && m_episodeQueue.isEmpty()) {
        onScrapingFinished();
        return;
    }

    m_currentShow = nullptr;
    m_currentEpisode = nullptr;

    if (!m_showQueue.isEmpty()) {
        m_currentShow = m_showQueue.dequeue();
        ui->title->setText(m_currentShow->title().trimmed());
    } else if (!m_episodeQueue.isEmpty()) {
        m_currentEpisode = m_episodeQueue.dequeue();
        ui->title->setText(m_currentEpisode->title().trimmed());
    }

    int sum = m_shows.count() + m_episodes.count();
    ui->itemCounter->setText(QString("%1/%2").arg(sum - m_showQueue.count() - m_episodeQueue.count()).arg(sum));

    ui->progressAll->setValue(ui->progressAll->maximum() - m_showQueue.size() - m_episodeQueue.count() - 1);
    ui->progressItem->setValue(0);

    if (ui->chkOnlyId->isChecked()
        && (((m_currentShow != nullptr) && !m_currentShow->tvdbId().isValid())
            || ((m_currentEpisode != nullptr) && !m_currentEpisode->tvShow()->tvdbId().isValid()))) {
        scrapeNext();
        return;
    }

    if (m_currentShow != nullptr) {
        if (!m_currentShow->tvdbId().isValid()) {
            ShowSearchJob::Config config{m_currentShow->title(), m_locale, Settings::instance()->showAdultScrapers()};
            auto* searchJob = m_currentScraper->search(config);
            connect(searchJob, &ShowSearchJob::sigFinished, this, &TvShowMultiScrapeDialog::onSearchFinished);
            searchJob->execute();

        } else {
            connect(m_currentShow.data(),
                &TvShow::sigLoaded,
                this,
                &TvShowMultiScrapeDialog::onInfoLoadDone,
                Qt::UniqueConnection);
            m_currentShow->scrapeData(m_currentScraper,
                m_currentShow->tvdbId().toString(),
                m_locale,
                m_seasonOrder,
                TvShowUpdateType::Show,
                m_showDetailsToLoad,
                m_episodeDetailsToLoad);
        }
    } else if (m_currentEpisode != nullptr) {
        connect(m_currentEpisode.data(),
            &TvShowEpisode::sigLoaded,
            this,
            &TvShowMultiScrapeDialog::onEpisodeLoadDone,
            Qt::UniqueConnection);

        const QString title = m_currentEpisode->tvShow()->title();

        if (m_currentEpisode->tvShow()->tvdbId().isValid()) {
            m_currentEpisode->scrapeData(m_currentScraper,
                m_locale,
                m_currentEpisode->tvShow()->tvdbId().toString(),
                m_seasonOrder,
                m_episodeDetailsToLoad);

        } else if (m_showIds.contains(title)) {
            m_currentEpisode->scrapeData(
                m_currentScraper, m_locale, m_showIds.value(title).toString(), m_seasonOrder, m_episodeDetailsToLoad);

        } else {
            ShowSearchJob::Config config{
                m_currentEpisode->tvShow()->title(), m_locale, Settings::instance()->showAdultScrapers()};
            auto* searchJob = m_currentScraper->search(config);
            connect(searchJob, &ShowSearchJob::sigFinished, this, &TvShowMultiScrapeDialog::onSearchFinished);
            searchJob->execute();
        }
    }
}

void TvShowMultiScrapeDialog::onSearchFinished(scraper::ShowSearchJob* searchJob)
{
    if (!m_executed) {
        searchJob->deleteLater();
        return;
    }
    if (searchJob->hasError()) {
        showError(searchJob->error().message);
        searchJob->deleteLater();
        return;
    }
    if (searchJob->results().isEmpty()) {
        searchJob->deleteLater();
        scrapeNext();
        return;
    }

    if (m_currentShow != nullptr) {
        m_showIds.insert(m_currentShow->title(), TvDbId(searchJob->results().first().identifier.str()));
        m_currentShow->scrapeData(m_currentScraper,
            searchJob->results().first().identifier.str(),
            m_locale,
            m_seasonOrder,
            TvShowUpdateType::Show,
            m_showDetailsToLoad,
            m_episodeDetailsToLoad);

    } else if (m_currentEpisode != nullptr) {
        m_showIds.insert(m_currentEpisode->tvShow()->title(), TvDbId(searchJob->results().first().identifier.str()));
        m_currentEpisode->scrapeData(m_currentScraper,
            m_locale,
            searchJob->results().first().identifier.str(),
            m_seasonOrder,
            m_episodeDetailsToLoad);
    }
    searchJob->deleteLater();
}

void TvShowMultiScrapeDialog::onScrapingFinished()
{
    ui->itemCounter->setVisible(false);
    int numberOfShows = m_shows.count();
    int numberOfEpisodes = m_episodes.count();
    if (ui->chkOnlyId->isChecked()) {
        numberOfShows = 0;
        numberOfEpisodes = 0;
        for (TvShow* show : m_shows) {
            if (show->tvdbId().isValid()) {
                numberOfShows++;
            }
        }
        for (TvShowEpisode* episode : m_episodes) {
            if (episode->tvShow()->tvdbId().isValid()) {
                numberOfEpisodes++;
            }
        }
    }

    QString shows = tr("%n TV shows", "", numberOfShows);
    QString episodes = tr("%n episodes", "", numberOfEpisodes);
    if (numberOfShows > 0 && numberOfEpisodes > 0) {
        ui->title->setText(tr("Scraping of %1 and %2 has finished.").arg(shows).arg(episodes));
    } else if (numberOfShows > 0) {
        ui->title->setText(tr("Scraping of %1 has finished.").arg(shows));
    } else if (numberOfEpisodes > 0) {
        ui->title->setText(tr("Scraping of %1 has finished.").arg(episodes));
    }

    ui->progressAll->setValue(ui->progressAll->maximum());
    ui->btnCancel->setVisible(false);
    ui->btnClose->setVisible(true);
    ui->btnStartScraping->setVisible(false);
}

void TvShowMultiScrapeDialog::onInfoLoadDone(TvShow* show, QSet<ShowScraperInfo> details)
{
    Q_UNUSED(details);

    if (!m_executed) {
        return;
    }

    if (show != m_currentShow) {
        return;
    }

    if (show->showMissingEpisodes()) {
        show->clearMissingEpisodes();
        show->fillMissingEpisodes();
    }

    QVector<ImageType> types = {ImageType::TvShowClearArt,
        ImageType::TvShowLogos,
        ImageType::TvShowCharacterArt,
        ImageType::TvShowThumb,
        ImageType::TvShowSeasonThumb};
    if (show->tvdbId().isValid() && details.contains(ShowScraperInfo::ExtraArts)) {
        Manager::instance()->fanartTv()->tvShowImages(show, show->tvdbId(), types, m_locale);
        connect(Manager::instance()->fanartTv(),
            &ImageProviderInterface::sigTvShowImagesLoaded,
            this,
            &TvShowMultiScrapeDialog::onLoadDone,
            Qt::UniqueConnection);
    } else {
        QMap<ImageType, QVector<Poster>> map;
        onLoadDone(show, map);
    }
}

void TvShowMultiScrapeDialog::onLoadDone(TvShow* show, QMap<ImageType, QVector<Poster>> posters)
{
    if (!m_executed) {
        return;
    }

    if (show != m_currentShow) {
        return;
    }

    int downloadsSize = 0;
    if (!show->posters().isEmpty() && m_showDetailsToLoad.contains(ShowScraperInfo::Poster)) {
        addDownload(ImageType::TvShowPoster, show->posters().at(0).originalUrl, show);
        downloadsSize++;
    }

    if (!show->backdrops().isEmpty() && m_showDetailsToLoad.contains(ShowScraperInfo::Fanart)) {
        addDownload(ImageType::TvShowBackdrop, show->backdrops().at(0).originalUrl, show);
        downloadsSize++;
    }

    if (!show->banners().isEmpty() && m_showDetailsToLoad.contains(ShowScraperInfo::Banner)) {
        addDownload(ImageType::TvShowBanner, show->banners().at(0).originalUrl, show);
        downloadsSize++;
    }

    QVector<SeasonNumber> thumbsForSeasons;
    QMapIterator<ImageType, QVector<Poster>> it(posters);
    while (it.hasNext()) {
        it.next();
        if (m_showDetailsToLoad.contains(ShowScraperInfo::ExtraArts) && it.key() == ImageType::TvShowClearArt
            && !it.value().isEmpty()) {
            addDownload(ImageType::TvShowClearArt, it.value().at(0).originalUrl, show);
            downloadsSize++;
        } else if (m_showDetailsToLoad.contains(ShowScraperInfo::ExtraArts) && it.key() == ImageType::TvShowCharacterArt
                   && !it.value().isEmpty()) {
            addDownload(ImageType::TvShowCharacterArt, it.value().at(0).originalUrl, show);
            downloadsSize++;
        } else if (m_showDetailsToLoad.contains(ShowScraperInfo::ExtraArts) && it.key() == ImageType::TvShowLogos
                   && !it.value().isEmpty()) {
            addDownload(ImageType::TvShowLogos, it.value().at(0).originalUrl, show);
            downloadsSize++;
        } else if (m_showDetailsToLoad.contains(ShowScraperInfo::ExtraArts) && it.key() == ImageType::TvShowThumb
                   && !it.value().isEmpty()) {
            addDownload(ImageType::TvShowThumb, it.value().at(0).originalUrl, show);
            downloadsSize++;
        } else if (m_showDetailsToLoad.contains(ShowScraperInfo::SeasonThumb)
                   && it.key() == ImageType::TvShowSeasonThumb && !it.value().isEmpty()) {
            for (const Poster& p : it.value()) {
                if (thumbsForSeasons.contains(p.season)) {
                    continue;
                }
                if (!show->seasons().contains(p.season)) {
                    continue;
                }

                addDownload(ImageType::TvShowSeasonThumb, p.originalUrl, show, p.season);
                downloadsSize++;
                thumbsForSeasons.append(p.season);
            }
        }
    }

    if (m_showDetailsToLoad.contains(ShowScraperInfo::Actors) && Settings::instance()->downloadActorImages()) {
        for (Actor* actor : show->actors()) {
            if (actor->thumb.isEmpty()) {
                continue;
            }
            addDownload(ImageType::Actor, QUrl(actor->thumb), show, actor);
            downloadsSize++;
        }
    }

    for (SeasonNumber season : show->seasons()) {
        if (!show->seasonPosters(season).isEmpty() && m_showDetailsToLoad.contains(ShowScraperInfo::SeasonPoster)) {
            addDownload(ImageType::TvShowSeasonPoster, show->seasonPosters(season).at(0).originalUrl, show, season);
            downloadsSize++;
        }
        if (!show->seasonBackdrops(season).isEmpty() && m_showDetailsToLoad.contains(ShowScraperInfo::SeasonBackdrop)) {
            addDownload(ImageType::TvShowSeasonBackdrop, show->seasonBackdrops(season).at(0).originalUrl, show, season);
            downloadsSize++;
        }
        if (!show->seasonBanners(season).isEmpty() && m_showDetailsToLoad.contains(ShowScraperInfo::SeasonBanner)) {
            addDownload(ImageType::TvShowSeasonBanner, show->seasonBanners(season).at(0).originalUrl, show, season);
            downloadsSize++;
        }
    }

    if (downloadsSize > 0) {
        ui->progressItem->setMaximum(downloadsSize);
    } else {
        scrapeNext();
    }
}

void TvShowMultiScrapeDialog::addDownload(ImageType imageType, QUrl url, TvShow* show, SeasonNumber season)
{
    DownloadManagerElement d;
    d.imageType = imageType;
    d.url = std::move(url);
    d.season = season;
    d.show = show;
    m_downloadManager->addDownload(d);
}

void TvShowMultiScrapeDialog::addDownload(ImageType imageType, QUrl url, TvShow* show, Actor* actor)
{
    DownloadManagerElement d;
    d.imageType = imageType;
    d.url = std::move(url);
    d.actor = actor;
    d.show = show;
    m_downloadManager->addDownload(d);
}

void TvShowMultiScrapeDialog::addDownload(ImageType imageType, QUrl url, TvShowEpisode* episode)
{
    DownloadManagerElement d;
    d.imageType = imageType;
    d.url = std::move(url);
    d.episode = episode;
    d.directDownload = true;
    m_downloadManager->addDownload(d);
}

void TvShowMultiScrapeDialog::onDownloadFinished(DownloadManagerElement elem)
{
    if (!m_executed) {
        return;
    }

    if (elem.show != nullptr) {
        int left = m_downloadManager->downloadsLeftForShow(m_currentShow);
        ui->progressItem->setValue(ui->progressItem->maximum() - left);
        qDebug() << "Download finished" << left << ui->progressItem->maximum();

        if (TvShow::seasonImageTypes().contains(elem.imageType)) {
            if (elem.imageType == ImageType::TvShowSeasonBackdrop) {
                helper::resizeBackdrop(elem.data);
            }
            ImageCache::instance()->invalidateImages(
                Manager::instance()->mediaCenterInterface()->imageFileName(elem.show, elem.imageType, elem.season));
            elem.show->setSeasonImage(elem.season, elem.imageType, elem.data);
        } else if (elem.imageType != ImageType::Actor) {
            if (elem.imageType == ImageType::TvShowBackdrop) {
                helper::resizeBackdrop(elem.data);
            }
            ImageCache::instance()->invalidateImages(
                Manager::instance()->mediaCenterInterface()->imageFileName(elem.show, elem.imageType));
            elem.show->setImage(elem.imageType, elem.data);
        }
    } else if ((elem.episode != nullptr) && elem.imageType == ImageType::TvShowEpisodeThumb) {
        elem.episode->setThumbnailImage(elem.data);
        scrapeNext();
    }
}

void TvShowMultiScrapeDialog::onDownloadsFinished()
{
    if (!m_executed) {
        return;
    }

    scrapeNext();
}

void TvShowMultiScrapeDialog::setupSeasonOrderComboBox()
{
    ui->comboSeasonOrder->addItem(tr("Aired order"), static_cast<int>(SeasonOrder::Aired));
    ui->comboSeasonOrder->addItem(tr("DVD order"), static_cast<int>(SeasonOrder::Dvd));

    const int index = 0;
    ui->comboSeasonOrder->setCurrentIndex(index);
}

void TvShowMultiScrapeDialog::onScraperChanged(int index)
{
    if (index < 0 || index >= Manager::instance()->scrapers().movieScrapers().size()) {
        qCritical() << "[Movie Search] Selected invalid scraper:" << index;
        showError(tr("Internal inconsistency: Selected an invalid scraper!"));
        return;
    }

    const QString scraperId = ui->comboScraper->itemData(index, Qt::UserRole).toString();
    m_currentScraper = Manager::instance()->scrapers().tvScraper(scraperId);

    setupLanguageDropdown();
}

void TvShowMultiScrapeDialog::onLanguageChanged(int index)
{
    const int size = static_cast<int>(m_currentScraper->meta().supportedLanguages.size());
    if (index < 0 || index >= size) {
        return;
    }
    m_locale = ui->comboLanguage->localeAt(index);
}

void TvShowMultiScrapeDialog::onSeasonOrderChanged(int index)
{
    bool ok = false;
    const int order = ui->comboSeasonOrder->itemData(index, Qt::UserRole).toInt(&ok);
    if (!ok) {
        qCritical() << "[TvShowMultiScrapeDialog] Invalid index for SeasonOrder";
        return;
    }
    m_seasonOrder = SeasonOrder(order);
    Settings::instance()->setSeasonOrder(m_seasonOrder);
}

void TvShowMultiScrapeDialog::showError(const QString& message)
{
    ui->lblError->setText(message);
    ui->lblError->show();
}

void TvShowMultiScrapeDialog::setupScraperDropdown()
{
    ui->comboScraper->blockSignals(true);
    ui->comboScraper->clear();

    for (const scraper::TvScraper* scraper : Manager::instance()->scrapers().tvScrapers()) {
        ui->comboScraper->addItem(scraper->meta().name, scraper->meta().identifier);
    }

    m_currentScraper = Manager::instance()->scrapers().tvScrapers().first();

    const int index = ui->comboScraper->findData(m_currentScraper->meta().identifier);
    ui->comboScraper->setCurrentIndex(index);
    ui->comboScraper->blockSignals(false);
}

void TvShowMultiScrapeDialog::setupLanguageDropdown()
{
    if (m_currentScraper == nullptr) {
        ui->comboLanguage->setInvalid();
        qCritical() << "[TvShowSearch] Cannot set language dropdown in TV show search widget";
        showError(tr("Internal inconsistency: Cannot set language dropdown in TV show search widget!"));
        return;
    }

    const auto& meta = m_currentScraper->meta();
    m_locale = Settings::instance()->scraperSettings(meta.identifier)->language(meta.defaultLocale);
    ui->comboLanguage->setupLanguages(meta.supportedLanguages, m_locale);
}

void TvShowMultiScrapeDialog::onEpisodeLoadDone()
{
    if (!m_executed) {
        return;
    }

    auto* episode = dynamic_cast<TvShowEpisode*>(QObject::sender());
    if (episode == nullptr) {
        return;
    }

    if (m_episodeDetailsToLoad.contains(EpisodeScraperInfo::Thumbnail) && !episode->thumbnail().isEmpty()) {
        addDownload(ImageType::TvShowEpisodeThumb, episode->thumbnail(), episode);
    } else {
        scrapeNext();
    }
}
