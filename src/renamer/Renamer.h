#pragma once

#include "media/FileFilter.h"
#include "utils/Meta.h"

#include <QDir>
#include <QString>
#include <QStringList>
#include <QVector>

class Movie;
class RenamerDialog;


enum class RenameType : int8_t
{
    Movies,
    TvShows,
    Concerts,
    All
};

QString renamerTypeToString(RenameType type);

struct RenamerConfig
{
    QString filePattern;
    QString filePatternMulti;
    QString directoryPattern;
    bool renameFiles = false;
    bool renameDirectories = false;
    bool dryRun = false;
};


/// \brief A placeholder is a template string that can be filled with data.
/// \details
///     The placeholder class expects as input a template string.  This template string
///     can be used to render strings that are filled with values from a data provider.
/// \example
///     ```cpp
///     Placeholder p { QStringLiteral("<title|lower>{imdbId} - imdb-<imdbId>{/imdbId} - <genre[0]|lower>") };
///     Placeholder::DataProvider data = …;
///     if (p.isValidWith(data)) {
///       QString result = p.replaceWith(data);
///       // e.g. "my movie - action"
///     }
///     ```
class Placeholder
{
public:
    class DataProvider
    {
    public:
        DataProvider() = default;
        virtual ~DataProvider() = default;

        virtual ELCH_NODISCARD bool hasField(const QString& name) = 0;
        virtual ELCH_NODISCARD QString stringValue(const QString& name) = 0;
        virtual ELCH_NODISCARD int integerValue(const QString& name) = 0;
        virtual ELCH_NODISCARD double floatValue(const QString& name) = 0;
    };

    Placeholder fromTemplate(const QString& str);

    bool hasParseError() const;
    bool isValidWith(DataProvider& data) const;
    QString replaceWith(DataProvider& data) const;

private:
    explicit Placeholder() = default;
};


class Renamer
{
public:
    enum class RenameResult : int8_t
    {
        Failed,
        Success
    };
    enum class RenameOperation : int8_t
    {
        CreateDir,
        Move,
        Rename
    };
    enum class RenameError : int8_t
    {
        None, // Todo: Be more specific about what error occurred
        Error
    };

    Renamer(RenamerConfig config, RenamerDialog* dialog);

    static QString replace(QString& text, const QString& search, QString replacement);
    static QString replaceCondition(QString& text, const QString& condition, const QString& replace);
    static QString replaceCondition(QString& text, const QString& condition, bool hasCondition);

    static bool rename(QDir& dir, QString newName);
    static bool rename(const QString& file, const QString& newName);

protected:
    RenamerConfig m_config;
    RenamerDialog* m_dialog;
    const mediaelch::FileFilter& m_extraFiles;
};
