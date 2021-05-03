#pragma once

#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QImage>
#include <QMap>
#include <QMovie>
#include <QSplitter>
#include <QStringList>
#include <QTableWidgetItem>
#include <QWidget>

#include "globals/DownloadManagerElement.h"

class DownloadManager;
class Movie;

namespace Ui {
class SetsWidget;
}

namespace mediaelch {
class MovieSetModel;
class MovieCollection;
} // namespace mediaelch

class SetsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SetsWidget(QWidget* parent = nullptr);
    ~SetsWidget() override;

public slots:
    /// \brief Parses list of movie and constructs sets map
    void loadSets();
    void saveSet();
    QSplitter* splitter();

signals:
    void setActionSaveEnabled(bool, MainWidgets);
    void sigJumpToMovie(Movie* movie);

private slots:
    void onSetsLoaded();

    /// \see SetsWidget::loadSets
    void onSetSelected(const QModelIndex& current, const QModelIndex& previous);
    /// \brief Updates the setName label in the detail view if current set changed.
    void onSetDataChanged(const QModelIndex& index);

    void clearMovieSetPanel();
    /// \brief   Called when an item in the movies table was changed.
    /// \details Updates movies sorttitle and reorders the table
    /// \param item changed item
    void onSortTitleChanged(QTableWidgetItem* item);
    void onAddMovieSet();
    void onRemoveMovieSet();
    /// \brief Execs the MovieListDialog and (if accepted) adds a movie to the movies table.
    /// \details Adds the movie to the set model.
    void onAddMovie();
    /// \brief   Removes a movie from the movies table
    /// \details Empties the movie set for the movie and removes it from the set model.
    void onRemoveMovie();
    /// \brief Shows QFileDialog to choose an image, if successful sets the poster image.
    void chooseSetPoster();
    /// \brief Shows QFileDialog to choose an image, if successful sets the backdrop image.
    void chooseSetBackdrop();
    void showBackdropPreview();
    void showPosterPreview();

    void onDownloadFinished(DownloadManagerElement elem);
    void onJumpToMovie(QTableWidgetItem* item);
    void loadSet(const QModelIndex& index);

private:
    void resizeMoviesTable();
    void clearPosterImage();
    void clearBackdropImage();

private:
    Ui::SetsWidget* ui;

    QImage m_currentPoster;
    QImage m_currentBackdrop;
    QMenu* m_tableContextMenu = nullptr;
    DownloadManager* m_downloadManager;
    QMovie* m_loadingMovie = nullptr;
    QElapsedTimer m_reloadTimer;
    QFutureWatcher<QVector<mediaelch::MovieCollection*>> m_futureWatcher;

    mediaelch::MovieSetModel* m_model = nullptr;
};
