#include "movies/MovieSetModel.h"

#include "data/Actor.h"
#include "log/Log.h"
#include "media_centers/MediaCenterInterface.h"
#include "movies/Movie.h"

#include <QFont>

namespace mediaelch {

bool MovieCollection::save(MediaCenterInterface& mediaCenter)
{
    for (Movie* movie : asConst(m_movies)) {
        movie->controller()->saveData(&mediaCenter);
    }

    // First rename...
    if (m_name != m_initialName) {
        mediaCenter.renameMovieSet(m_initialName, m_name);
        m_initialName = m_name;
    }

    // ... then safe the new images / overwrite the old ones.
    if (!m_poster.isNull()) {
        mediaCenter.saveMovieSetPoster(m_name, m_poster);
        // Clear to save memory
        m_poster = {};
    }
    if (!m_backdrop.isNull()) {
        mediaCenter.saveMovieSetBackdrop(m_name, m_backdrop);
        // Clear to save memory
        m_backdrop = {};
    }

    m_isUnsaved = false;

    return true;
}

bool MovieCollection::addMovie(Movie& movie)
{
    // First, set the name accordingly.
    // The description is not changed.
    MovieSetDetails set = movie.set();
    if (set.name != m_name) {
        set.name = m_name;
        movie.setSet(set);
        // Only mark as unsaved, if movie didn't belong to this set before.
        m_isUnsaved = true;
    }

    m_movies.push_back(&movie);

    return true;
}

bool MovieCollection::removeMovie(Movie& movie)
{
    // Can't remove movie from collection if it isn't part of it.
    const int index = m_movies.indexOf(&movie);
    if (index < 0) {
        return false;
    }

    if (movie.set().name != m_name) {
        // If the name does not match, we must have messed up!
        qCCritical(generic) << "[MovieCollection] Inconsistency in set name! Movie" << movie.name() << ":"
                            << movie.set().name << "|" << m_name;
    }

    const bool success = m_movies.removeOne(&movie);
    return success;
}

void MovieCollection::setName(QString _name)
{
    if (_name == m_name) {
        return; // don't update if not needed
    }

    m_name = std::move(_name);

    // Change the name for all movies as well.
    for (Movie* movie : asConst(m_movies)) {
        MovieSetDetails set = movie->set();
        set.name = m_name;
        movie->setSet(set);
    }

    m_isUnsaved = true;
}

void MovieCollection::setPoster(QImage img)
{
    m_poster = std::move(img);
    m_isUnsaved = true;
}

void MovieCollection::setBackdrop(QImage img)
{
    m_backdrop = std::move(img);
    m_isUnsaved = true;
}

QImage MovieCollection::posterFromMediaCenter(MediaCenterInterface& mediaCenter)
{
    // Always use the initial name. There may already be images.
    // The initial name is updated in save()
    return mediaCenter.movieSetPoster(m_initialName);
}

QImage MovieCollection::backdropFromMediaCenter(MediaCenterInterface& mediaCenter)
{
    // Always use the initial name. There may already be images.
    // The initial name is updated in save()
    return mediaCenter.movieSetBackdrop(m_initialName);
}

MovieSetModel::~MovieSetModel()
{
    // Should already be handled by using "this" as the set's parent
    // but better be safe.
    qDeleteAll(m_sets);
}

bool MovieSetModel::addEmptyMovieSet()
{
    QString setName = tr("New Movie Set");
    QModelIndex index = indexForSet(setName);

    int adder = 0;
    while (index.isValid()) {
        ++adder;
        QString newName = QStringLiteral("%1 %2").arg(setName).arg(adder);
        index = indexForSet(newName);
    }

    if (adder > 0) {
        setName = QStringLiteral("%1 %2").arg(setName).arg(adder);
    }

    QModelIndex root{};
    beginInsertRows(root, rowCount(), rowCount());
    m_sets.push_back(new MovieCollection(setName, {}, this));
    endInsertRows();
    return true;
}

void MovieSetModel::addMovie(Movie& movie)
{
    const QString setName = movie.set().name;

    const auto set = std::find_if(m_sets.begin(), m_sets.end(), [&setName](const MovieCollection* set) { //
        return setName == set->name();
    });

    if (set != m_sets.end()) {
        const QModelIndex index = indexForSet(setName);
        (*set)->addMovie(movie);
        emit dataChanged(index, index);

    } else {
        QModelIndex root{};
        beginInsertRows(root, rowCount(), rowCount());
        m_sets.push_back(new MovieCollection(movie.set().name, {&movie}, this));
        endInsertRows();
    }
}

void MovieSetModel::addMovieSets(const QVector<MovieCollection*>& sets)
{
    if (sets.isEmpty()) {
        return;
    }

    QModelIndex root{};
    beginInsertRows(root, rowCount(), rowCount() + sets.size() - 1);

    for (MovieCollection* set : sets) {
        set->setParent(this);
        m_sets.push_back(set);
    }

    endInsertRows();
}

int MovieSetModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        // Root has an invalid model index.
        return 0;
    }
    return m_sets.size();
}

int MovieSetModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        // Root has an invalid model index.
        return 0;
    }
    return 1;
}

QVariant MovieSetModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    auto* movieSet = const_cast<MovieCollection*>(m_sets[index.row()]);

    switch (role) {
    case MovieSetRoles::MovieListRole: return QVariant::fromValue(movieSet->movies());
    case MovieSetRoles::CollectionPointerRole:
        return QVariant::fromValue(const_cast<MovieCollection*>(m_sets[index.row()]));

    case Qt::DisplayRole:
    case Qt::EditRole: {
        switch (index.column()) {
        case Columns::NameColumn: return movieSet->name();
        }
        break;
    }
    case Qt::FontRole: {
        if (movieSet->hasChanged()) {
            QFont font;
            font.setItalic(true);
            return font;
        }
    }
    }

    return {};
}

QVariant MovieSetModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Orientation::Vertical) {
        return {};
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return {};
    }

    switch (section) {
    case Columns::NameColumn: return tr("Movie set name");
    }

    return {};
}

void MovieSetModel::removeEmptySets()
{
    int i = 0;
    int n = m_sets.size();

    while (i < n) {
        if (m_sets[i]->isEmpty()) {
            removeRows(i, 1);
        } else {
            ++i;
        }
        n = m_sets.size();
    }
}

bool MovieSetModel::removeMovieFromSet(Movie& movie)
{
    int row = rowOfSet(movie.set().name);
    if (row != -1) {
        bool success = m_sets[row]->removeMovie(movie);
        if (!success) {
            return false;
        }
        movie.set() = MovieSetDetails{};
        m_moviesWithoutSet.push_back(&movie);

        QModelIndex index = createIndex(row, 0);
        emit dataChanged(index, index);
    }
    return true;
}

bool MovieSetModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid()) {
        // non-root element not possible in table view
        return false;
    }

    const int endRow = row + count - 1;

    beginRemoveRows(parent, row, endRow);

    // Clear the "set" entry for all movies inside the to-be-removed set.
    for (int i = row; i <= endRow; ++i) {
        auto& movies = m_sets[i]->movies();
        for (Movie* movie : movies) {
            movie->setSet(MovieSetDetails{});
            // We somehow need to ensure that movies without sets can
            // be saved.
            m_moviesWithoutSet << movie;
        }
    }

    m_sets.erase(m_sets.begin() + row, m_sets.begin() + row + count);
    endRemoveRows();

    return true;
}

void MovieSetModel::reset()
{
    if (rowCount() <= 0) {
        return;
    }
    beginRemoveRows({}, 0, rowCount() - 1);

    qDeleteAll(m_sets);
    m_sets.clear();

    m_moviesWithoutSet.clear();
    endRemoveRows();
}

bool MovieSetModel::saveSet(const QModelIndex& index, MediaCenterInterface& mediaCenter)
{
    if (!index.isValid() || index.row() >= rowCount() || index.row() < 0) {
        return false;
    }
    bool success = m_sets[index.row()]->save(mediaCenter);
    if (success) {
        emit dataChanged(index, index, {});
    }
    return success;
}

QModelIndex MovieSetModel::indexForSet(const QString& setName)
{
    const int row = rowOfSet(setName);
    if (row != -1) {
        return createIndex(row, 0);
    }
    return QModelIndex{};
}

bool MovieSetModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) {
        // root element can't be edited
        return false;
    }
    if (role != Qt::EditRole) {
        return false;
    }

    auto& movieSet = m_sets[index.row()];
    switch (index.column()) {
    case Columns::NameColumn: {
        movieSet->setName(value.toString());
        break;
    }
    }

    emit dataChanged(index, index, {role});

    return true;
}

Qt::ItemFlags MovieSetModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.isValid())
        f |= Qt::ItemIsEditable;
    return f;
}

int MovieSetModel::rowOfSet(const QString& setName)
{
    int row = 0;
    for (const auto& set : asConst(m_sets)) {
        if (set->name() == setName) {
            return row;
        }
        ++row;
    }
    return -1;
}

QVector<MovieCollection*> MovieSetFactory::create(const QVector<Movie*>& movies, QObject* parent)
{
    QMap<QString, mediaelch::MovieCollection*> sets;

    for (Movie* movie : movies) {
        if (!movie->set().name.isEmpty()) {
            if (sets.contains(movie->set().name)) {
                sets[movie->set().name]->addMovie(*movie);
                if (movie->hasChanged()) {
                    sets[movie->set().name]->setChanged();
                }
            } else {
                auto* collection = new mediaelch::MovieCollection(movie->set().name, {movie});
                if (movie->hasChanged()) {
                    collection->setChanged();
                }
                if (parent != nullptr) {
                    collection->moveToThread(parent->thread());
                    collection->setParent(parent);
                }
                sets.insert(movie->set().name, collection);
            }
        }
    }

    return QVector<MovieCollection*>(sets.cbegin(), sets.cend());
}

} // namespace mediaelch
