#include "SetsWidget.h"
#include "ui_SetsWidget.h"

#include "globals/DownloadManager.h"
#include "globals/Globals.h"
#include "globals/Helper.h"
#include "globals/ImageDialog.h"
#include "globals/ImagePreviewDialog.h"
#include "globals/Manager.h"
#include "log/Log.h"
#include "movies/Movie.h"
#include "movies/MovieSetModel.h"
#include "ui/movie_sets/MovieListDialog.h"
#include "ui/notifications/NotificationBox.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>

SetsWidget::SetsWidget(QWidget* parent) : QWidget(parent), ui(new Ui::SetsWidget)
{
    ui->setupUi(this);

    m_model = new mediaelch::MovieSetModel(this);

    ui->sets->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->buttonPreviewBackdrop->setEnabled(false);
    ui->buttonPreviewPoster->setEnabled(false);

    ui->sets->setModel(m_model);

#ifndef Q_OS_MAC
    QFont nameFont = ui->setName->font();
    nameFont.setPointSize(nameFont.pointSize() - 4);
    ui->setName->setFont(nameFont);
#endif

    helper::applyStyle(ui->movies);
    helper::applyStyle(ui->lblPoster);
    helper::applyStyle(ui->lblBackdrop);
    helper::applyStyle(ui->posterResolution);
    helper::applyStyle(ui->backdropResolution);
    helper::applyStyle(ui->groupBox_3);
    helper::applyEffect(ui->groupBox_3);

    m_loadingMovie = new QMovie(":/img/spinner.gif", QByteArray(), this);
    m_loadingMovie->start();
    m_downloadManager = new DownloadManager(this);

    // clang-format off
    connect(ui->sets->selectionModel(), &QItemSelectionModel::currentChanged,  this, &SetsWidget::onSetSelected);
    connect(m_model,                    &QAbstractItemModel::dataChanged,      this, &SetsWidget::onSetDataChanged);
    connect(ui->movies,                 &QTableWidget::itemChanged,            this, &SetsWidget::onSortTitleChanged);
    connect(ui->movies,                 &QTableWidget::itemDoubleClicked,      this, &SetsWidget::onJumpToMovie);
    connect(ui->buttonAddMovie,         &QAbstractButton::clicked,             this, &SetsWidget::onAddMovie);
    connect(ui->buttonRemoveMovie,      &QAbstractButton::clicked,             this, &SetsWidget::onRemoveMovie);
    connect(ui->poster,                 &MyLabel::clicked,                     this, &SetsWidget::chooseSetPoster);
    connect(ui->backdrop,               &MyLabel::clicked,                     this, &SetsWidget::chooseSetBackdrop);
    connect(ui->buttonPreviewPoster,    &QAbstractButton::clicked,             this, &SetsWidget::showPosterPreview);
    connect(ui->buttonPreviewBackdrop,  &QAbstractButton::clicked,             this, &SetsWidget::showBackdropPreview);
    connect(m_downloadManager,          &DownloadManager::sigDownloadFinished, this, &SetsWidget::onDownloadFinished, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    // clang-format on

    connect(&m_futureWatcher,
        &QFutureWatcher<QVector<mediaelch::MovieCollection*>>::finished,
        this,
        &SetsWidget::onSetsLoaded);

    ui->sets->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableContextMenu = new QMenu(ui->sets);
    auto* actionAddSet = new QAction(tr("Add Movie Set"), this);
    auto* actionDeleteSet = new QAction(tr("Delete Movie Set"), this);
    m_tableContextMenu->addAction(actionAddSet);
    m_tableContextMenu->addAction(actionDeleteSet);
    connect(actionAddSet, &QAction::triggered, this, &SetsWidget::onAddMovieSet);
    connect(actionDeleteSet, &QAction::triggered, this, &SetsWidget::onRemoveMovieSet);
    connect(ui->sets, &QWidget::customContextMenuRequested, this, [this](QPoint point) {
        m_tableContextMenu->exec(ui->sets->mapToGlobal(point));
    });

    clearMovieSetPanel();
    clearPosterImage();
    clearBackdropImage();
}

SetsWidget::~SetsWidget()
{
    delete ui;
}

QSplitter* SetsWidget::splitter()
{
    return ui->splitter;
}

void SetsWidget::loadSets()
{
    using namespace mediaelch;
    if (m_futureWatcher.isRunning()) {
        qInfo() << "[MovieSets] Sets are already loading. No reload is performed.";
        return;
    }

    qInfo() << "[MovieSets] Recreating list of movie sets from all movies.";
    m_reloadTimer.start();

    m_model->reset();
    resizeMoviesTable();

    emit setActionSaveEnabled(false, MainWidgets::MovieSets);
    clearMovieSetPanel();
    ui->buttonPreviewBackdrop->setEnabled(false);
    ui->buttonPreviewPoster->setEnabled(false);

    QFuture<QVector<MovieCollection*>> future = QtConcurrent::run([this]() {
        auto movieSets = MovieSetFactory::create(Manager::instance()->movieModel()->movies(), this);
        return movieSets;
    });
    m_futureWatcher.setFuture(future);
}

void SetsWidget::onSetsLoaded()
{
    int lastSelectedRow = ui->sets->currentIndex().row();

    QVector<mediaelch::MovieCollection*> sets = m_futureWatcher.result();
    m_model->addMovieSets(sets);

    if (ui->sets->model()->rowCount() > 0 && lastSelectedRow > 0 && lastSelectedRow < ui->sets->model()->rowCount()) {
        const QModelIndex lastIndex = ui->sets->model()->index(lastSelectedRow, 0, {});
        ui->sets->setCurrentIndex(lastIndex);

    } else if (ui->sets->model()->rowCount() > 0) {
        const QModelIndex index = ui->sets->model()->index(0, 0, {});
        ui->sets->setCurrentIndex(index);
    }

    qInfo() << "[MovieSets] Reloading movie sets took" << m_reloadTimer.elapsed() << "ms";
    m_reloadTimer.invalidate();

    emit setActionSaveEnabled(true, MainWidgets::MovieSets);
}

void SetsWidget::onSetSelected(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous)

    if (current.isValid()) {
        loadSet(current);
    }
    // Index may be invalid if movie set model was resetted.
}

void SetsWidget::clearMovieSetPanel()
{
    ui->setName->clear();
    ui->movies->clearContents();
    ui->movies->setRowCount(0);
    ui->backdropResolution->clear();
    ui->posterResolution->clear();
    m_currentBackdrop = QImage();
    m_currentPoster = QImage();
}

/**
 * \brief Fills the widget with set data
 * \param set Name of the set
 */
void SetsWidget::loadSet(const QModelIndex& index)
{
    using namespace mediaelch;

    clearMovieSetPanel();

    ui->setName->setText(index.data(Qt::DisplayRole).toString());
    ui->buttonPreviewBackdrop->setEnabled(false);
    ui->buttonPreviewPoster->setEnabled(false);
    ui->movies->blockSignals(true);

    const QString set = index.data(Qt::DisplayRole).toString();

    auto* movieSet = index.data(MovieSetModel::CollectionPointerRole).value<MovieCollection*>();
    Q_ASSERT(movieSet != nullptr);

    qDebug() << "[MovieSets] Loading set:" << set;

    const QVector<Movie*> movies = index.data(mediaelch::MovieSetModel::MovieListRole).value<QVector<Movie*>>();
    for (Movie* movie : movies) {
        int row = ui->movies->rowCount();
        ui->movies->insertRow(row);
        ui->movies->setItem(row, 0, new QTableWidgetItem(movie->name()));
        ui->movies->item(row, 0)->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->movies->item(row, 0)->setData(Qt::UserRole, QVariant::fromValue(movie));
        ui->movies->setItem(row, 1, new QTableWidgetItem(movie->sortTitle()));
    }

    ui->movies->sortByColumn(1, Qt::AscendingOrder);

    MediaCenterInterface& mediaCenter = *Manager::instance()->mediaCenterInterface();

    const QImage& poster = movieSet->hasPoster() ? movieSet->poster() : movieSet->posterFromMediaCenter(mediaCenter);
    if (!poster.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(poster).scaled(
            QSize(200, 300) * helper::devicePixelRatio(this), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        helper::setDevicePixelRatio(pixmap, helper::devicePixelRatio(this));
        ui->poster->setPixmap(pixmap);
        ui->posterResolution->setText(QString("%1x%2").arg(poster.width()).arg(poster.height()));
        ui->buttonPreviewPoster->setEnabled(true);
        m_currentPoster = poster;

    } else {
        clearPosterImage();
        ui->buttonPreviewPoster->setEnabled(false);
    }

    const QImage& backdrop =
        movieSet->hasBackdrop() ? movieSet->backdrop() : movieSet->backdropFromMediaCenter(mediaCenter);
    if (!backdrop.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(backdrop).scaled(
            QSize(200, 112) * helper::devicePixelRatio(this), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        helper::setDevicePixelRatio(pixmap, helper::devicePixelRatio(this));
        ui->backdrop->setPixmap(pixmap);
        ui->backdropResolution->setText(QStringLiteral("%1x%2").arg(backdrop.width()).arg(backdrop.height()));
        ui->buttonPreviewBackdrop->setEnabled(true);
        m_currentBackdrop = backdrop;

    } else {
        clearBackdropImage();
        ui->buttonPreviewBackdrop->setEnabled(false);
    }

    ui->movies->blockSignals(false);
}

void SetsWidget::resizeMoviesTable()
{
    // We need to update the size here, as the widget isn't fully constructed
    // in the constructor (window size may have changed).
    ui->movies->horizontalHeader()->resizeSections(QHeaderView::QHeaderView::Stretch);
}

void SetsWidget::clearPosterImage()
{
    QPixmap pixmap =
        QPixmap(":/img/placeholders/poster.png")
            .scaled(QSize(160, 260) * helper::devicePixelRatio(this), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    helper::setDevicePixelRatio(pixmap, helper::devicePixelRatio(this));
    ui->poster->setPixmap(pixmap);
}

void SetsWidget::clearBackdropImage()
{
    QPixmap pixmap =
        QPixmap(":/img/placeholders/fanart.png")
            .scaled(QSize(160, 72) * helper::devicePixelRatio(this), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    helper::setDevicePixelRatio(pixmap, helper::devicePixelRatio(this));
    ui->backdrop->setPixmap(pixmap);
}

void SetsWidget::onSortTitleChanged(QTableWidgetItem* item)
{
    qCDebug(generic) << "Entered, item->row=" << item->row() << "rowCount=" << ui->movies->rowCount();
    if (item->row() < 0 || item->row() >= ui->movies->rowCount() || item->column() != 1) {
        qCDebug(generic) << "Invalid row";
        return;
    }
    auto* movie = ui->movies->item(item->row(), 0)->data(Qt::UserRole).value<Movie*>();
    movie->setSortTitle(item->text());
    ui->movies->sortByColumn(1, Qt::AscendingOrder);

    auto* set = m_model->indexForSet(movie->set().name)
                    .data(mediaelch::MovieSetModel::CollectionPointerRole)
                    .value<mediaelch::MovieCollection*>();
    if (set != nullptr) {
        set->setChanged();
    }
}

void SetsWidget::onAddMovie()
{
    const int currentRow = ui->sets->currentIndex().row();

    if (currentRow < 0 || currentRow > m_model->rowCount()) {
        return;
    }

    auto* listDialog = new MovieListDialog(this);
    const int exitCode = listDialog->exec();
    const QVector<Movie*> movies = listDialog->selectedMovies();
    listDialog->deleteLater();

    if (exitCode != QDialog::Accepted || movies.isEmpty()) {
        return;
    }

    QString setName = ui->sets->currentIndex().data(Qt::DisplayRole).toString();
    for (Movie* movie : asConst(movies)) {
        if (movie->set().name == setName) {
            continue;
        }

        MovieSetDetails set = movie->set();
        set.name = setName;
        movie->setSet(set);

        m_model->addMovie(*movie);
    }

    loadSet(m_model->indexForSet(setName));
}

void SetsWidget::onRemoveMovie()
{
    const int setRow = ui->sets->currentIndex().row();
    if (setRow < 0 || setRow >= m_model->rowCount()) {
        qCWarning(generic) << "[SetsWidget] Invalid current row in sets: Can't remove movie from set";
        return;
    }

    if (ui->movies->currentRow() < 0 || ui->movies->currentRow() >= ui->movies->rowCount()) {
        qCWarning(generic) << "[SetsWidget] Invalid current row in movies: Can't remove movie from set";
        return;
    }

    auto* movie = ui->movies->item(ui->movies->currentRow(), 0)->data(Qt::UserRole).value<Movie*>();

    m_model->removeMovieFromSet(*movie);
    ui->movies->removeRow(ui->movies->currentRow());
}

void SetsWidget::chooseSetPoster()
{
    const int row = ui->sets->currentIndex().row();
    if (row < 0 || row >= m_model->rowCount()) {
        qCWarning(generic) << "[SetsWidget] Invalid current row in sets";
        return;
    }

    QString setName = ui->sets->currentIndex().data(Qt::DisplayRole).toString();
    auto* movie = new Movie(QStringList());
    movie->setName(setName);

    auto* imageDialog = new ImageDialog(this);
    imageDialog->setImageType(ImageType::MovieSetPoster);
    imageDialog->setMovie(movie);
    imageDialog->execWithType(ImageType::MoviePoster);
    const int exitCode = imageDialog->result();
    const QUrl imageUrl = imageDialog->imageUrl();
    imageDialog->deleteLater();

    if (exitCode == QDialog::Accepted) {
        DownloadManagerElement d;
        d.movie = movie;
        d.imageType = ImageType::MovieSetPoster;
        d.url = imageUrl;
        m_downloadManager->addDownload(d);
        ui->poster->setPixmap(QPixmap());
        ui->poster->setMovie(m_loadingMovie);
        ui->buttonPreviewPoster->setEnabled(false);
    }
}

void SetsWidget::chooseSetBackdrop()
{
    const int row = ui->sets->currentIndex().row();
    if (row < 0 || row >= m_model->rowCount()) {
        qCWarning(generic) << "[SetsWidget] Invalid current row in sets";
        return;
    }

    QString setName = ui->sets->currentIndex().data(Qt::DisplayRole).toString();
    auto* movie = new Movie(QStringList());
    movie->setName(setName);

    auto* imageDialog = new ImageDialog(this);
    imageDialog->setImageType(ImageType::MovieSetBackdrop);
    imageDialog->setMovie(movie);
    imageDialog->execWithType(ImageType::MovieBackdrop);
    const int exitCode = imageDialog->result();
    const QUrl imageUrl = imageDialog->imageUrl();
    imageDialog->deleteLater();

    if (exitCode == QDialog::Accepted) {
        DownloadManagerElement d;
        d.movie = movie;
        d.imageType = ImageType::MovieSetBackdrop;
        d.url = imageUrl;
        m_downloadManager->addDownload(d);
        ui->backdrop->setPixmap(QPixmap());
        ui->backdrop->setMovie(m_loadingMovie);
        ui->buttonPreviewBackdrop->setEnabled(false);
    }
}

void SetsWidget::saveSet()
{
    const int row = ui->sets->currentIndex().row();
    if (row < 0 || row >= m_model->rowCount() || !ui->sets->currentIndex().isValid()) {
        qCWarning(generic) << "[SetsWidget] Invalid current row in sets";
        return;
    }

    bool success = m_model->saveSet(ui->sets->currentIndex(), *Manager::instance()->mediaCenterInterface());

    const QString setName = ui->sets->currentIndex().data(Qt::DisplayRole).toString();
    if (success) {
        NotificationBox::instance()->showSuccess(tr("<b>\"%1\"</b> saved").arg(setName));
    } else {
        NotificationBox::instance()->showError(tr("<b>\"%1\"</b> not saved").arg(setName));
    }
}

void SetsWidget::showBackdropPreview()
{
    auto* dialog = new ImagePreviewDialog(this);
    dialog->setImage(QPixmap::fromImage(m_currentBackdrop));
    dialog->exec();
    dialog->deleteLater();
}

void SetsWidget::showPosterPreview()
{
    auto* dialog = new ImagePreviewDialog(this);
    dialog->setImage(QPixmap::fromImage(m_currentPoster));
    dialog->exec();
    dialog->deleteLater();
}

void SetsWidget::onAddMovieSet()
{
    m_tableContextMenu->close();
    m_model->addEmptyMovieSet();
}

void SetsWidget::onRemoveMovieSet()
{
    const int row = ui->sets->currentIndex().row();
    if (row < 0 || row >= m_model->rowCount()) {
        qCWarning(generic) << "[SetsWidget] Invalid current row in sets";
        return;
    }

    QString setName = ui->sets->currentIndex().data(Qt::DisplayRole).toString();
    qCInfo(generic) << "[SetsWidget] Removing set:" << setName;
    m_model->removeRows(row, 1);
}

void SetsWidget::onSetDataChanged(const QModelIndex& item)
{
    if (item == ui->sets->selectionModel()->currentIndex()) {
        QString newSetName = item.data(Qt::DisplayRole).toString();
        if (newSetName != ui->setName->text()) {
            ui->setName->setText(newSetName);
        }
    }
}

void SetsWidget::onDownloadFinished(DownloadManagerElement elem)
{
    using namespace mediaelch;
    QString setName = elem.movie->name();
    QModelIndex index = m_model->indexForSet(setName);

    // Only update the current view, if this download elements belongs
    // to the currently shown movie set.
    const auto updateIfCurrentSet = [this, &setName]() {
        if (ui->sets->currentIndex().data(Qt::DisplayRole).toString() == setName) {
            loadSet(ui->sets->currentIndex());
        }
    };

    if (index.isValid()) {
        auto* set = index.data(MovieSetModel::CollectionPointerRole).value<MovieCollection*>();
        Q_ASSERT(set != nullptr);

        if (elem.imageType == ImageType::MovieSetPoster) {
            set->setPoster(QImage::fromData(elem.data));
            // TODO: Maybe through signal in movie set?
            m_model->dataChanged(index, index, {});
            updateIfCurrentSet();

        } else if (elem.imageType == ImageType::MovieSetBackdrop) {
            set->setBackdrop(QImage::fromData(elem.data));
            // TODO: Maybe through signal in movie set?
            m_model->dataChanged(index, index, {});
            updateIfCurrentSet();
        }
    }

    delete elem.movie;
}

void SetsWidget::onJumpToMovie(QTableWidgetItem* item)
{
    if (item->column() != 0) {
        return;
    }

    auto* movie = item->data(Qt::UserRole).value<Movie*>();
    emit sigJumpToMovie(movie);
}
