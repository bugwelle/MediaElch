#include "TvShowSearchWidget.h"
#include "ui_TvShowSearchWidget.h"

#include "globals/Manager.h"
#include "settings/Settings.h"
#include "ui/small_widgets/MyCheckBox.h"

using namespace mediaelch::scraper;

static constexpr unsigned ComboIndex_Show = 0;
static constexpr unsigned ComboIndex_ShowAndNewEpisodes = 1;
static constexpr unsigned ComboIndex_ShowAndAllEpisodes = 2;
static constexpr unsigned ComboIndex_NewEpisodes = 3;
static constexpr unsigned ComboIndex_AllEpisodes = 4;

static constexpr unsigned TabInfos_PageShow = 0;
static constexpr unsigned TabInfos_PageEpisode = 1;

TvShowSearchWidget::TvShowSearchWidget(QWidget* parent) : QWidget(parent), ui(new Ui::TvShowSearchWidget)
{
    ui->setupUi(this);

    ui->results->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->searchString->setType(MyLineEdit::TypeLoading);

    const auto indexChanged = elchOverload<int>(&QComboBox::currentIndexChanged);
    // clang-format off
    connect(ui->comboScraper,  indexChanged, this, &TvShowSearchWidget::onScraperChanged,  Qt::QueuedConnection);
    connect(ui->comboLanguage, indexChanged, this, &TvShowSearchWidget::onLanguageChanged, Qt::QueuedConnection);
    connect(ui->searchString, &QLineEdit::returnPressed,  this, &TvShowSearchWidget::initializeAndStartSearch);
    connect(ui->results,      &QTableWidget::itemClicked, this, &TvShowSearchWidget::onResultClicked);
    connect(ui->comboUpdate,  indexChanged,               this, &TvShowSearchWidget::onUpdateTypeChanged);
    connect(ui->comboSeasonOrder, indexChanged,           this, &TvShowSearchWidget::onSeasonOrderChanged);
    // clang-format on

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
            connect(box, &QAbstractButton::clicked, this, &TvShowSearchWidget::onShowInfoToggled);
        }
    }
    for (MyCheckBox* box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0) {
            connect(box, &QAbstractButton::clicked, this, &TvShowSearchWidget::onEpisodeInfoToggled);
        }
    }

    connect(ui->chkUnCheckAll, &QAbstractButton::clicked, this, &TvShowSearchWidget::onChkAllShowInfosToggled);
    connect(
        ui->chkEpisodeUnCheckAll, &QAbstractButton::clicked, this, &TvShowSearchWidget::onChkAllEpisodeInfosToggled);

    setupScraperDropdown();
    setupLanguageDropdown();
    setupSeasonOrderComboBox();
    onShowInfoToggled();
    onEpisodeInfoToggled();
}

TvShowSearchWidget::~TvShowSearchWidget()
{
    delete ui;
}

void TvShowSearchWidget::clearResultTable()
{
    ui->results->clearContents();
    ui->results->setRowCount(0);
}

void TvShowSearchWidget::search(QString searchString)
{
    ui->searchString->setText(searchString.replace(".", " ").trimmed());
    initializeAndStartSearch();
}

void TvShowSearchWidget::initializeAndStartSearch()
{
    using namespace mediaelch::scraper;

    clearResultTable();
    ui->searchString->setLoading(true);

    if (m_currentScraper->isInitialized()) {
        qInfo() << "[TvShowSearch] Start search for:" << ui->searchString->text();

        ShowSearchJob::Config config{
            ui->searchString->text().trimmed(), m_currentLanguage, Settings::instance()->showAdultScrapers()};
        auto* searchJob = m_currentScraper->search(config);
        connect(searchJob, &ShowSearchJob::sigFinished, this, &TvShowSearchWidget::onShowResults);
        searchJob->execute();

    } else {
        // TODO: connections, etc
        qInfo() << "[TvShowSearch] Initialize scraper:" << m_currentScraper->meta().identifier;
        m_currentScraper->initialize();
    }
}

void TvShowSearchWidget::onShowResults(ShowSearchJob* searchJob)
{
    ui->searchString->setLoading(false);
    ui->searchString->setFocus();

    if (searchJob->hasError()) {
        qDebug() << "[TvShowSearch] Got error while searching for show" << searchJob->error().message;
        showError(searchJob->error().message);
        searchJob->deleteLater();
        return;
    }

    qDebug() << "[TvShowSearch] Result count:" << searchJob->results().count();
    showSuccess(tr("Found %n results", "", searchJob->results().count()));

    for (const auto& result : searchJob->results()) {
        const QString title(QStringLiteral("%1 (%2)").arg(result.title).arg(result.released.toString("yyyy")));

        auto* item = new QTableWidgetItem(title);
        item->setData(Qt::UserRole, result.identifier.str());

        const int row = ui->results->rowCount();
        ui->results->insertRow(row);
        ui->results->setItem(row, 0, item);
    }

    searchJob->deleteLater();
}

void TvShowSearchWidget::onResultClicked(QTableWidgetItem* item)
{
    m_showIdentifier = item->data(Qt::UserRole).toString();
    emit sigResultClicked();
}

void TvShowSearchWidget::setSearchType(TvShowType type)
{
    m_searchType = type;
    if (type == TvShowType::TvShow) {
        ui->tabsInfos->setCurrentIndex(TabInfos_PageShow);
        ui->comboUpdate->setVisible(true);
        const int index = Settings::instance()->tvShowUpdateOption();
        ui->comboUpdate->setCurrentIndex(index);
        onUpdateTypeChanged(index);

    } else if (type == TvShowType::Episode) {
        ui->tabsInfos->setCurrentIndex(TabInfos_PageEpisode);
        ui->comboUpdate->setVisible(false);
        ui->comboUpdate->setCurrentIndex(ComboIndex_AllEpisodes);
        onUpdateTypeChanged(ComboIndex_AllEpisodes);
    }
}

QString TvShowSearchWidget::showIdentifier()
{
    return m_showIdentifier;
}

mediaelch::scraper::TvScraper* TvShowSearchWidget::scraper()
{
    return m_currentScraper;
}

SeasonOrder TvShowSearchWidget::seasonOrder() const
{
    return m_seasonOrder;
}

const QSet<ShowScraperInfo>& TvShowSearchWidget::showDetailsToLoad() const
{
    return m_showDetailsToLoad;
}

const QSet<EpisodeScraperInfo>& TvShowSearchWidget::episodeDetailsToLoad() const
{
    return m_episodeDetailsToLoad;
}

const mediaelch::Locale& TvShowSearchWidget::locale() const
{
    return m_currentLanguage;
}

void TvShowSearchWidget::onShowInfoToggled()
{
    m_showDetailsToLoad.clear();
    bool allToggled = true;
    for (MyCheckBox* box : ui->showInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->isEnabled() && box->isChecked() && box->myData().toInt() > 0 && box->isEnabled()) {
            m_showDetailsToLoad.insert(ShowScraperInfo(box->myData().toInt()));
        }
        if (!box->isChecked() && box->myData().toInt() > 0 && box->isEnabled()) {
            allToggled = false;
        }
    }

    ui->chkUnCheckAll->setChecked(allToggled);

    int scraperNo = ui->comboUpdate->currentIndex();
    if (m_searchType == TvShowType::Episode) {
        scraperNo = ComboIndex_AllEpisodes;
    }
    Settings::instance()->setScraperShowInfos(QString::number(scraperNo), m_showDetailsToLoad);
}

void TvShowSearchWidget::onEpisodeInfoToggled()
{
    m_episodeDetailsToLoad.clear();
    bool allToggled = true;
    for (MyCheckBox* box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->isEnabled() && box->isChecked() && box->myData().toInt() > 0) {
            m_episodeDetailsToLoad.insert(EpisodeScraperInfo(box->myData().toInt()));
        }
        if (box->isEnabled() && !box->isChecked() && box->myData().toInt() > 0) {
            allToggled = false;
        }
    }

    ui->chkEpisodeUnCheckAll->setChecked(allToggled);

    int scraperNo = ui->comboUpdate->currentIndex();
    if (m_searchType == TvShowType::Episode) {
        scraperNo = ComboIndex_AllEpisodes;
    }
    Settings::instance()->setScraperEpisodeInfos(QString::number(scraperNo), m_episodeDetailsToLoad);
}

void TvShowSearchWidget::onChkAllShowInfosToggled()
{
    bool checked = ui->chkUnCheckAll->isChecked();
    for (MyCheckBox* box : ui->showInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0 && box->isEnabled()) {
            box->setChecked(checked);
        }
    }
    onShowInfoToggled();
}

void TvShowSearchWidget::onChkAllEpisodeInfosToggled()
{
    bool checked = ui->chkEpisodeUnCheckAll->isChecked();
    for (MyCheckBox* box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        if (box->myData().toInt() > 0 && box->isEnabled()) {
            box->setChecked(checked);
        }
    }
    onEpisodeInfoToggled();
}

void TvShowSearchWidget::onScraperChanged(int index)
{
    if (index < 0 || index >= Manager::instance()->scrapers().movieScrapers().size()) {
        qCritical() << "[Movie Search] Selected invalid scraper:" << index;
        showError(tr("Internal inconsistency: Selected an invalid scraper!"));
        return;
    }

    const QString scraperId = ui->comboScraper->itemData(index, Qt::UserRole).toString();
    qDebug() << "[TvShowSearchWidget] Selected scraper:" << scraperId;
    m_currentScraper = Manager::instance()->scrapers().tvScraper(scraperId);

    if (m_currentScraper == nullptr) {
        qFatal("[TvShowSearchWidget] Couldn't get scraper from manager");
    }

    setupLanguageDropdown();
    updateCheckBoxes();
    initializeAndStartSearch();
}

void TvShowSearchWidget::onLanguageChanged(int index)
{
    const int size = static_cast<int>(m_currentScraper->meta().supportedLanguages.size());
    if (index < 0 || index >= size) {
        return;
    }
    m_currentLanguage = ui->comboLanguage->localeAt(index);
    initializeAndStartSearch();
}

TvShowUpdateType TvShowSearchWidget::updateType()
{
    return m_updateType;
}

void TvShowSearchWidget::onUpdateTypeChanged(int index)
{
    if (m_searchType != TvShowType::Episode) {
        Settings::instance()->setTvShowUpdateOption(ui->comboUpdate->currentIndex());
    }

    if (index == ComboIndex_Show) {
        m_updateType = TvShowUpdateType::Show;
    } else if (index == ComboIndex_ShowAndNewEpisodes) {
        m_updateType = TvShowUpdateType::ShowAndNewEpisodes;
    } else if (index == ComboIndex_ShowAndAllEpisodes) {
        m_updateType = TvShowUpdateType::ShowAndAllEpisodes;
    } else if (index == ComboIndex_NewEpisodes) {
        m_updateType = TvShowUpdateType::NewEpisodes;
    } else {
        m_updateType = TvShowUpdateType::AllEpisodes;
    }

    updateCheckBoxes();
}

void TvShowSearchWidget::updateCheckBoxes()
{
    TvShowUpdateType type = updateType();
    const bool enableShow = isShowUpdateType(type);
    const bool enableEpisode = isEpisodeUpdateType(type);

    // General setup, e.g. hide help texts, disable group box, etc.
    // More fine-grained box-setup below for episode- and show details.
    ui->lblShowOnlyType->setVisible(!enableEpisode);
    ui->lblEpisodeOnlyType->setVisible(!enableShow);
    ui->episodeInfosGroupBox->setEnabled(enableEpisode);
    ui->showInfosGroupBox->setEnabled(enableShow);
    ui->comboSeasonOrder->setEnabled(enableEpisode);

    const auto& meta = m_currentScraper->meta();
    const auto showInfos = Settings::instance()->scraperInfos<ShowScraperInfo>(meta.identifier);
    const auto episodeInfos = Settings::instance()->scraperInfos<EpisodeScraperInfo>(meta.identifier);

    for (auto* box : ui->showInfosGroupBox->findChildren<MyCheckBox*>()) {
        const auto detail = ShowScraperInfo(box->myData().toInt());
        const bool enabled = enableShow && meta.supportedShowDetails.contains(detail);
        const bool checked = enabled && (showInfos.isEmpty() || showInfos.contains(detail));
        box->setChecked(checked);
        box->setEnabled(enabled);
    }
    for (auto* box : ui->episodeInfosGroupBox->findChildren<MyCheckBox*>()) {
        const auto detail = EpisodeScraperInfo(box->myData().toInt());
        const bool enabled = enableEpisode && meta.supportedEpisodeDetails.contains(detail);
        const bool checked = enabled && (showInfos.isEmpty() || episodeInfos.contains(detail));
        box->setChecked(checked);
        box->setEnabled(enabled);
    }

    onShowInfoToggled();
    onEpisodeInfoToggled();
}

void TvShowSearchWidget::onSeasonOrderChanged(int index)
{
    bool ok = false;
    const int order = ui->comboSeasonOrder->itemData(index, Qt::UserRole).toInt(&ok);
    if (!ok) {
        qCritical() << "[TvShowSearch] Invalid index for SeasonOrder";
        return;
    }
    m_seasonOrder = SeasonOrder(order);
    Settings::instance()->setSeasonOrder(m_seasonOrder);
}

void TvShowSearchWidget::setupSeasonOrderComboBox()
{
    m_seasonOrder = Settings::instance()->seasonOrder();
    ui->comboSeasonOrder->addItem(tr("Aired order"), static_cast<int>(SeasonOrder::Aired));
    ui->comboSeasonOrder->addItem(tr("DVD order"), static_cast<int>(SeasonOrder::Dvd));

    const int index = ui->comboSeasonOrder->findData(static_cast<int>(m_seasonOrder));
    if (index > -1) {
        ui->comboSeasonOrder->setCurrentIndex(index);
    } else {
        m_seasonOrder = SeasonOrder::Aired;
        qCritical() << "[TvShowSearch] Couldn't find season order element in combo box";
    }
}

void TvShowSearchWidget::setupScraperDropdown()
{
    ui->comboScraper->blockSignals(true);
    ui->comboScraper->clear();

    for (const TvScraper* scraper : Manager::instance()->scrapers().tvScrapers()) {
        ui->comboScraper->addItem(scraper->meta().name, scraper->meta().identifier);
    }

    m_currentScraper = Manager::instance()->scrapers().tvScrapers().first();

    const int index = ui->comboScraper->findData(m_currentScraper->meta().identifier);
    ui->comboScraper->setCurrentIndex(index);
    ui->comboScraper->blockSignals(false);
}

void TvShowSearchWidget::setupLanguageDropdown()
{
    const auto& meta = m_currentScraper->meta();
    m_currentLanguage = Settings::instance()->scraperSettings(meta.identifier)->language(meta.defaultLocale);
    ui->comboLanguage->setupLanguages(meta.supportedLanguages, m_currentLanguage);
}

void TvShowSearchWidget::showError(const QString& message)
{
    ui->lblSuccessMessage->hide();
    ui->lblErrorMessage->setText(message);
    ui->lblErrorMessage->show();
}

void TvShowSearchWidget::showSuccess(const QString& message)
{
    ui->lblErrorMessage->hide();
    ui->lblSuccessMessage->setText(message);
    ui->lblSuccessMessage->show();
}
