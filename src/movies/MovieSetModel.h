#pragma once

#include "movies/Movie.h"
#include "movies/MovieSet.h"

#include <QAbstractTableModel>
#include <memory>
#include <vector>

namespace mediaelch {

class MovieCollection : public QObject
{
    Q_OBJECT

public:
    MovieCollection() = default;
    explicit MovieCollection(const QString& _name, QVector<Movie*> _movies = {}, QObject* parent = nullptr) :
        QObject(parent), m_name{_name}, m_initialName{_name}, m_movies{std::move(_movies)}
    {
    }
    ~MovieCollection() override = default;

    bool save(MediaCenterInterface& mediaCenter);

    void setChanged() { m_isUnsaved = true; }
    bool hasChanged() const { return m_isUnsaved; }
    const QVector<Movie*>& movies() { return m_movies; }

    bool addMovie(Movie& movie);
    bool removeMovie(Movie& movie);

    void setName(QString _name);
    const QString& name() const { return m_name; }

    void setPoster(QImage img);
    const QImage& poster() const { return m_poster; }
    bool hasPoster() const { return !m_poster.isNull(); }


    void setBackdrop(QImage img);
    const QImage& backdrop() const { return m_backdrop; }
    bool hasBackdrop() const { return !m_backdrop.isNull(); }

    QImage posterFromMediaCenter(MediaCenterInterface& mediaCenter);
    QImage backdropFromMediaCenter(MediaCenterInterface& mediaCenter);

    bool isEmpty() const { return m_movies.isEmpty(); }

private:
    QString m_name;
    /// \brief The initial name of the collection. Used for renaming.
    QString m_initialName;
    QVector<Movie*> m_movies;
    QImage m_poster;
    QImage m_backdrop;

    bool m_isUnsaved = false;
};

/// \brief Factory for creating movie sets from movies.
class MovieSetFactory
{
public:
    MovieSetFactory() = delete;

    static QVector<MovieCollection*> create(const QVector<Movie*>& movies, QObject* parent = nullptr);
};


class MovieSetModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum MovieSetRoles
    {
        MovieListRole = Qt::UserRole + 1,
        CollectionPointerRole = Qt::UserRole + 2
    };

    enum Columns
    {
        NameColumn = 0
    };

public:
    using QAbstractTableModel::QAbstractTableModel;
    ~MovieSetModel() override;

    bool addEmptyMovieSet();
    void addMovie(Movie& movie);
    /// \brief Adds the provided movie sets to the model. Takes ownership of the movie sets.
    void addMovieSets(const QVector<MovieCollection*>& sets);

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    /// \brief   Removes all sets that do not have any movie
    /// \details This method uses removeRows(). Therefore, do not call it inside any slot
    ///          that is connected a signal that is emitted when rows are removed, e.g. do
    ///          not call this method in selection-change slots.
    void removeEmptySets();
    bool removeMovieFromSet(Movie& movie);
    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;
    /// \brief Clears and deletes all movie sets. Ignores unsaved changes.
    void reset();

    bool saveSet(const QModelIndex& index, MediaCenterInterface& mediaCenter);

    QModelIndex indexForSet(const QString& setName);

    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

private:
    int rowOfSet(const QString& setName);

private:
    QVector<MovieCollection*> m_sets;
    QVector<Movie*> m_moviesWithoutSet;
};


} // namespace mediaelch

Q_DECLARE_METATYPE(mediaelch::MovieCollection*)
