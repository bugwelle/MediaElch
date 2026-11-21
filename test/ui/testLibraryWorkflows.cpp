#include "third_party/catch2/catch.hpp"

#include "test/ui/FixedMovieScraper.h"

#include "globals/Manager.h"
#include "settings/Settings.h"
#include "ui/concerts/ConcertInfoWidget.h"
#include "ui/main/FileScannerDialog.h"
#include "ui/main/MainWindow.h"
#include "ui/main/Message.h"
#include "ui/movies/MovieFilesWidget.h"
#include "ui/movies/MovieSearch.h"
#include "ui/music/MusicWidgetAlbum.h"
#include "ui/renamer/MovieRenamerDialog.h"

#include <QAbstractButton>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkProxy>
#include <QProcess>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QTreeView>
#include <QXmlStreamReader>
#include <algorithm>

namespace {

constexpr auto originalA = "<movie><title>Alpha Feature</title></movie>";
constexpr auto originalB = "<movie><title>Bravo Feature</title></movie>";
constexpr auto savedTitle = "Updated Feature";
constexpr auto worker = "Library window in an isolated profile";

QByteArray readFile(const QString& path)
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::ReadOnly));
    return file.readAll();
}

void writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly));
    REQUIRE(file.write(contents) == contents.size());
}

QString nfoTitle(const QByteArray& contents, const char* root = "movie")
{
    QXmlStreamReader xml(contents);
    QString title;
    if (xml.readNextStartElement() && xml.name() == QLatin1String(root)) {
        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("title")) {
                title = xml.readElementText();
            } else {
                xml.skipCurrentElement();
            }
        }
    }
    REQUIRE_FALSE(xml.hasError());
    return title;
}

void checkMovieRename(MainWindow& window, QVector<Movie*> movies, const QDir& library)
{
    auto* settings = Settings::instance();
    settings->setRenamePatterns(RenameType::Movies, "<title>.<extension>", {}, {}, {}, {});
    settings->setRenamings(RenameType::Movies, true, false, false, false);
    MovieRenamerDialog dialog(&window);
    auto* preview = dialog.findChild<QAbstractButton*>("btnDryRun");
    auto* rename = dialog.findChild<QAbstractButton*>("btnRename");
    auto* close = dialog.findChild<QAbstractButton*>("btnClose");
    auto* results = dialog.findChild<QTableWidget*>("resultsTable");
    REQUIRE(preview);
    REQUIRE(rename);
    REQUIRE(close);
    REQUIRE(results);
    REQUIRE(movies.size() == 1);

    const QDir folder(library.filePath("Bravo"));
    const auto originalFiles = folder.entryList(QDir::Files | QDir::NoDotAndDotDot);
    QStringList destinations;
    const auto run = [&](QAbstractButton* action) {
        destinations.clear();
        dialog.setMovies(movies);
        QTimer::singleShot(0, &dialog, [&]() {
            QTest::mouseClick(action, Qt::LeftButton);
            for (int row = 0; row < results->rowCount(); ++row) {
                destinations.append(QFileInfo(results->item(row, 2)->text()).fileName());
            }
            QTest::mouseClick(close, Qt::LeftButton);
        });
        dialog.exec();
        REQUIRE_FALSE(dialog.renameErrorOccurred());
        destinations.sort();
    };

    run(preview);
    const QStringList expected{"Bravo Feature.mkv", "Bravo Feature.nfo"};
    REQUIRE(destinations == expected);
    CHECK(folder.entryList(QDir::Files | QDir::NoDotAndDotDot) == originalFiles);
    CHECK(readFile(folder.filePath("Bravo.nfo")) == originalB);

    run(rename);
    CHECK(destinations == expected);
    CHECK(folder.entryList(QDir::Files | QDir::NoDotAndDotDot) == expected);
    CHECK(readFile(folder.filePath("Bravo Feature.nfo")) == originalB);
    CHECK(readFile(library.filePath("Alpha/Alpha.nfo")) == originalA);
}

void checkMovieScrape(MainWindow& window, QLineEdit& title)
{
    auto& scrapers = Manager::instance()->scrapers();
    auto fixture = std::make_unique<test::FixedMovieScraper>();
    auto* scraper = fixture.get();
    const auto registered = scrapers.movieScrapers();
    auto* settings = Settings::instance();
    const auto scraperIndex = std::count_if(registered.cbegin(), registered.cend(), [settings](const auto* entry) {
        return settings->showAdultScrapers() || !entry->meta().isAdult;
    });
    scrapers.addMovieScraper(std::move(fixture),
        std::make_unique<mediaelch::ScraperConfigurationStub>(scraper->meta().identifier, *settings));
    settings->setCurrentMovieScraper(qsizetype_to_int(scraperIndex));
    auto* search = window.findChild<QAbstractButton*>("btnSearch");
    REQUIRE(search);
    REQUIRE(search->isEnabled());

    bool clicked = false;
    QTest::mouseClick(search, Qt::LeftButton);
    QTimer::singleShot(0, &window, [&]() {
        auto* dialog = qobject_cast<MovieSearch*>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto* cancel = dialog->findChild<QAbstractButton*>("buttonClose");
        if (cancel && scraper->searchJob) {
            QTest::mouseClick(cancel, Qt::LeftButton);
            clicked = true;
        } else {
            dialog->reject();
        }
    });
    QApplication::processEvents();
    REQUIRE(clicked);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    REQUIRE(scraper->searchJob);
    CHECK(scraper->searchJob->cancellationRequested);
    scraper->searchJob->complete();
    CHECK(title.text() == "Bravo Feature");

    clicked = false;
    QSignalSpy titleChanged(&title, &QLineEdit::textChanged);
    QTest::mouseClick(search, Qt::LeftButton);
    QTimer::singleShot(0, &window, [&]() {
        auto* dialog = qobject_cast<MovieSearch*>(QApplication::activeModalWidget());
        if (!dialog) {
            return;
        }
        auto* results = dialog->findChild<QTableWidget*>("results");
        auto* scrape = dialog->findChild<QAbstractButton*>("buttonScrape");
        if (results && scrape && scraper->searchJob) {
            scraper->searchJob->complete();
            if (results->item(1, 0)) {
                const auto cell = results->visualItemRect(results->item(1, 0));
                QTest::mouseClick(results->viewport(), Qt::LeftButton, Qt::NoModifier, cell.center());
                if (scrape->isEnabled() && results->currentRow() == 1) {
                    QTest::mouseClick(scrape, Qt::LeftButton);
                    clicked = true;
                    return;
                }
            }
        }
        dialog->reject();
    });
    QApplication::processEvents();
    REQUIRE(clicked);
    if (titleChanged.isEmpty()) {
        REQUIRE(titleChanged.wait(3000));
    }
    REQUIRE(title.text() == "Second Result");
}

void checkEpisodeSave(MainWindow& window)
{
    auto* files = window.findChild<QWidget*>("tvShowFilesWidget");
    auto* episode = window.findChild<QWidget*>("episodeWidget");
    auto* season = window.findChild<QWidget*>("seasonWidget");
    REQUIRE(files);
    REQUIRE(episode);
    REQUIRE(season);
    auto* tree = files->findChild<QTreeView*>("files");
    auto* title = episode->findChild<QLineEdit*>("name");
    auto* save = window.findChild<QAbstractButton*>("btnSave");
    REQUIRE(tree);
    REQUIRE(title);
    REQUIRE(save);
    REQUIRE(tree->model()->rowCount() == 1);
    const auto showIndex = tree->model()->index(0, 0);
    REQUIRE(tree->model()->rowCount(showIndex) == 1);
    const auto seasonIndex = tree->model()->index(0, 0, showIndex);
    REQUIRE(tree->model()->rowCount(seasonIndex) == 2);
    tree->setAnimated(false);
    tree->expand(showIndex);
    tree->expand(seasonIndex);

    auto* pages = qobject_cast<QStackedWidget*>(episode->parentWidget());
    REQUIRE(pages);
    const auto select = [&](const QModelIndex& index, QWidget* page) {
        tree->scrollTo(index);
        const auto cell = tree->visualRect(index);
        REQUIRE(cell.isValid());
        const auto visibleCell = cell.intersected(tree->viewport()->rect());
        REQUIRE(visibleCell.isValid());
        const auto position = visibleCell.center();
        REQUIRE(tree->indexAt(position) == index);
        QSignalSpy pageChanged(pages, &QStackedWidget::currentChanged);
        QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, position);
        REQUIRE(tree->currentIndex() == index);
        if (pages->currentWidget() != page && pageChanged.isEmpty()) {
            pageChanged.wait(3000);
        }
        REQUIRE(pages->currentWidget() == page);
    };

    select(tree->model()->index(0, 0, seasonIndex), episode);
    REQUIRE(episode->isVisible());
    REQUIRE(title->text() == "First Episode");
    select(seasonIndex, season);
    REQUIRE(season->isVisible());
    select(tree->model()->index(1, 0, seasonIndex), episode);
    REQUIRE(episode->isVisible());
    REQUIRE(title->text() == "Second Episode");

    QTest::mouseClick(title, Qt::LeftButton);
    QTest::keyClick(title, Qt::Key_A, Qt::ControlModifier);
    QTest::keyClicks(title, "Updated Episode");
    REQUIRE(save->isEnabled());
    QTest::mouseClick(save, Qt::LeftButton);
}

void checkLibraryTitle(MainWindow& window, bool album, bool reopen)
{
    auto* files = window.findChild<QWidget*>(album ? "musicFilesWidget" : "concertFilesWidget");
    QWidget* editor = album ? static_cast<QWidget*>(window.findChild<MusicWidgetAlbum*>())
                            : static_cast<QWidget*>(window.findChild<ConcertInfoWidget*>());
    auto* save = window.findChild<QAbstractButton*>("btnSave");
    REQUIRE(files);
    REQUIRE(editor);
    REQUIRE(save);
    auto* title = editor->findChild<QLineEdit*>("title");
    REQUIRE(title);

    QAbstractItemView* view = album ? static_cast<QAbstractItemView*>(files->findChild<QTreeView*>("music"))
                                    : static_cast<QAbstractItemView*>(files->findChild<QTableView*>("files"));
    REQUIRE(view);
    const auto* model = view->model();
    REQUIRE(model->rowCount() == 1);
    auto index = model->index(0, 0);
    if (album) {
        REQUIRE(index.data().toString() == "Example Artist");
        REQUIRE(model->rowCount(index) == 1);
        auto* tree = static_cast<QTreeView*>(view);
        tree->setAnimated(false);
        tree->expand(index);
        index = model->index(0, 0, index);
    }
    view->scrollTo(index);
    const auto cell = view->visualRect(index).intersected(view->viewport()->rect());
    REQUIRE(cell.isValid());
    QSignalSpy titleChanged(title, &QLineEdit::textChanged);
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, cell.center());
    const QString expected = reopen ? "Updated Title" : "Original Title";
    if (title->text() != expected && titleChanged.isEmpty()) {
        titleChanged.wait(3000);
    }
    REQUIRE(title->text() == expected);
    if (album) {
        auto* artist = editor->findChild<QLineEdit*>("artist");
        REQUIRE(artist);
        CHECK(artist->text() == "Example Artist");
    }
    if (reopen) {
        return;
    }

    QTest::mouseClick(title, Qt::LeftButton);
    QTest::keyClick(title, Qt::Key_A, Qt::ControlModifier);
    QTest::keyClicks(title, "Updated Title");
    REQUIRE(save->isEnabled());
    QTest::mouseClick(save, Qt::LeftButton);
}

void runLibraryWindow(const QDir& profile, const QDir& library, const QString& mode)
{
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("XDG_CONFIG_HOME", profile.filePath("config"));
    env.insert("XDG_CONFIG_DIRS", profile.filePath("system-config"));
    env.insert("XDG_DATA_HOME", profile.filePath("data"));
    env.insert("XDG_CACHE_HOME", profile.filePath("cache"));
    env.insert("MEDIAELCH_UI_PROFILE", profile.absolutePath());
    env.insert("MEDIAELCH_UI_LIBRARY", library.absolutePath());
    env.insert("MEDIAELCH_UI_MODE", mode);

    QProcess process;
    process.setProcessEnvironment(env);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(QCoreApplication::applicationFilePath(), {worker, "--warn", "NoTests"});
    REQUIRE(process.waitForStarted(5000));
    const bool finished = process.waitForFinished(30000);
    if (!finished) {
        process.kill();
        process.waitForFinished();
    }
    INFO(process.readAll().toStdString());
    REQUIRE(finished);
    REQUIRE(process.exitStatus() == QProcess::NormalExit);
    REQUIRE(process.exitCode() == 0);
}

} // namespace

TEST_CASE("Movie editing, renaming and scraping workflows", "[ui][movies]")
{
    const auto mode = GENERATE(QString("edit"), QString("rename"), QString("scrape"));
    INFO("Workflow: " << mode.toStdString());
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    const QDir library(temp.filePath("library"));
    REQUIRE(library.mkpath("Alpha"));
    REQUIRE(library.mkpath("Bravo"));
    writeFile(library.filePath("Alpha/Alpha.mkv"), {});
    writeFile(library.filePath("Bravo/Bravo.mkv"), {});
    writeFile(library.filePath("Alpha/Alpha.nfo"), originalA);
    writeFile(library.filePath("Bravo/Bravo.nfo"), originalB);

    INFO("Library: " << library.absolutePath().toStdString());
    // Keep fixtures when an assertion fails so the resulting sidecars can be inspected.
    temp.setAutoRemove(false);
    if (mode != "edit") {
        runLibraryWindow(QDir(temp.filePath("profile")), library, mode);
    } else {
        const QDir profile(temp.filePath("profile"));
        runLibraryWindow(profile, library, "edit");
        REQUIRE(readFile(library.filePath("Alpha/Alpha.nfo")) == originalA);
        REQUIRE(nfoTitle(readFile(library.filePath("Bravo/Bravo.nfo"))) == savedTitle);
        // Keep the saved settings and force the new process to rebuild its cache from disk.
        const QDir data(profile.filePath("data"));
        REQUIRE(data.exists());
        REQUIRE(QDir(data).removeRecursively());
        runLibraryWindow(profile, library, "reopen");
    }
    temp.setAutoRemove(true);
}

TEST_CASE("Saving a selected episode preserves its sibling and show", "[ui][tv]")
{
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    const QDir library(temp.filePath("library"));
    REQUIRE(library.mkpath("Example/Season 1"));
    const auto show = library.filePath("Example/tvshow.nfo");
    const auto first = library.filePath("Example/Season 1/Example.S01E01.nfo");
    const auto second = library.filePath("Example/Season 1/Example.S01E02.nfo");
    constexpr auto showNfo = "<tvshow><title>Example Show</title></tvshow>";
    constexpr auto firstNfo =
        "<episodedetails><title>First Episode</title><season>1</season><episode>1</episode></episodedetails>";
    constexpr auto secondNfo =
        "<episodedetails><title>Second Episode</title><season>1</season><episode>2</episode></episodedetails>";
    writeFile(show, showNfo);
    writeFile(first, firstNfo);
    writeFile(second, secondNfo);
    writeFile(library.filePath("Example/Season 1/Example.S01E01.mkv"), {});
    writeFile(library.filePath("Example/Season 1/Example.S01E02.mkv"), {});

    INFO("Library: " << library.absolutePath().toStdString());
    temp.setAutoRemove(false);
    runLibraryWindow(QDir(temp.filePath("profile")), library, "tv");
    REQUIRE(readFile(show) == showNfo);
    REQUIRE(readFile(first) == firstNfo);
    REQUIRE(nfoTitle(readFile(second), "episodedetails") == "Updated Episode");
    temp.setAutoRemove(true);
}

TEST_CASE("Concert and album titles persist after reopening", "[ui][library]")
{
    const bool album = GENERATE(false, true);
    QTemporaryDir temp;
    REQUIRE(temp.isValid());
    const QDir library(temp.filePath("library"));
    QString sidecar;
    QString mode;
    if (album) {
        mode = "album";
        REQUIRE(library.mkpath("Example Artist/Example Album"));
        writeFile(library.filePath("Example Artist/artist.nfo"), "<artist><name>Example Artist</name></artist>");
        sidecar = library.filePath("Example Artist/Example Album/album.nfo");
        writeFile(sidecar, "<album><title>Original Title</title><artist>Example Artist</artist></album>");
    } else {
        mode = "concert";
        REQUIRE(library.mkpath("Example Concert"));
        writeFile(library.filePath("Example Concert/Example.mkv"), {});
        sidecar = library.filePath("Example Concert/Example.nfo");
        writeFile(sidecar, "<musicvideo><title>Original Title</title></musicvideo>");
    }

    INFO("Library: " << library.absolutePath().toStdString());
    temp.setAutoRemove(false);
    runLibraryWindow(QDir(temp.filePath("edit")), library, mode);
    REQUIRE(nfoTitle(readFile(sidecar), album ? "album" : "musicvideo") == "Updated Title");
    runLibraryWindow(QDir(temp.filePath("reopen")), library, mode + "-reopen");
    temp.setAutoRemove(true);
}

TEST_CASE("Library window in an isolated profile", "[.ui-worker]")
{
    const auto profilePath = qEnvironmentVariable("MEDIAELCH_UI_PROFILE");
    const auto libraryPath = qEnvironmentVariable("MEDIAELCH_UI_LIBRARY");
    const auto mode = qEnvironmentVariable("MEDIAELCH_UI_MODE");
    REQUIRE(!profilePath.isEmpty());
    REQUIRE(!libraryPath.isEmpty());
    const bool album = mode.startsWith("album");
    const bool concert = mode.startsWith("concert");
    REQUIRE((mode == "edit" || mode == "reopen" || mode == "rename" || mode == "tv" || mode == "scrape" || album
             || concert));
    const QDir profile(profilePath);
    const QDir library(libraryPath);
    INFO("Qt " << qVersion() << ", workflow " << mode.toStdString() << ", profile " << profilePath.toStdString());

    QCoreApplication::setOrganizationName("MediaElch");
    QCoreApplication::setApplicationName("MediaElch UI Test");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, profile.filePath("config"));
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, profile.filePath("system-config"));
    REQUIRE_FALSE(QFile::exists(Settings::applicationDir() + "/advancedsettings.xml"));
    auto* settings = Settings::instance();
    REQUIRE(settings->settings()->fileName().startsWith(profile.absolutePath() + '/'));
    REQUIRE(settings->databaseDir().toString().startsWith(profile.absolutePath() + '/'));
    REQUIRE(settings->imageCacheDir().toString().startsWith(profile.absolutePath() + '/'));
    settings->loadSettings();
    // Startup initializes remote scraper providers. Route those requests to loopback.
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, "127.0.0.1", 1));
    settings->setDownloadActorImages(false);
    settings->setAutoLoadStreamDetails(false);
    settings->setCheckForUpdates(false);
    const mediaelch::MediaDirectory directory{mediaelch::DirectoryPath(libraryPath), true, true, false};
    if (album) {
        settings->directorySettings().setMusicDirectories({directory});
        settings->setStartupSection("music");
    } else if (concert) {
        settings->directorySettings().setConcertDirectories({directory});
        settings->setStartupSection("concerts");
    } else if (mode == "tv") {
        settings->directorySettings().setTvShowDirectories({directory});
        settings->setStartupSection("tvshows");
    } else if (mode == "reopen") {
        const auto& dirs = settings->directorySettings().movieDirectories();
        REQUIRE(dirs.size() == 1);
        REQUIRE(dirs.first().path.toString() == libraryPath);
    } else {
        settings->directorySettings().setMovieDirectories({directory});
        settings->directorySettings().saveSettings();
        settings->settings()->sync();
        REQUIRE(settings->settings()->status() == QSettings::NoError);
    }

    MainWindowConfiguration config(*settings);
    config.init();
    MainWindow window(config);
    window.resize(1100, 750);
    auto* scanner = window.findChild<FileScannerDialog*>();
    REQUIRE(scanner);
    QSignalSpy scanFinished(scanner, &QDialog::finished);
    REQUIRE(scanFinished.wait(15000));
    REQUIRE(scanFinished.first().first().toInt() == QDialog::Accepted);
    // The scanner can close before the loader's deferred deletion lets its thread quit.
    const auto threads = Manager::instance()->movieFileSearcher()->findChildren<QThread*>();
    for (auto* thread : threads) {
        QSignalSpy stopped(thread, &QThread::finished);
        if (thread->isRunning() && stopped.isEmpty()) {
            REQUIRE(stopped.wait(3000));
        }
    }

    if (album || concert) {
        checkLibraryTitle(window, album, mode.endsWith("-reopen"));
        return;
    }
    if (mode == "tv") {
        checkEpisodeSave(window);
        return;
    }

    auto* list = window.findChild<MovieFilesWidget*>("movieFilesWidget");
    auto* editor = window.findChild<QWidget*>("movieWidget");
    REQUIRE(list);
    REQUIRE(editor);
    auto* table = list->findChild<QTableView*>("files");
    auto* title = editor->findChild<QLineEdit*>("name");
    auto* revert = editor->findChild<QAbstractButton*>("buttonRevert");
    auto* save = window.findChild<QAbstractButton*>("btnSave");
    REQUIRE(table);
    REQUIRE(title);
    REQUIRE(revert);
    REQUIRE(save);
    REQUIRE(table->model()->rowCount() == 2);

    const auto select = [&](const QString& expectedTitle) {
        const auto indexes =
            table->model()->match(table->model()->index(0, 0), Qt::DisplayRole, expectedTitle, 1, Qt::MatchExactly);
        INFO("Selecting " << expectedTitle.toStdString());
        REQUIRE(indexes.size() == 1);
        table->scrollTo(indexes.first());
        const auto cell = table->visualRect(indexes.first());
        REQUIRE(cell.isValid());
        QSignalSpy titleChanged(title, &QLineEdit::textChanged);
        QTest::mouseClick(table->viewport(), Qt::LeftButton, Qt::NoModifier, cell.center());
        if (title->text() != expectedTitle && titleChanged.isEmpty()) {
            titleChanged.wait(3000);
        }
        INFO("Editor displays " << title->text().toStdString());
        REQUIRE(title->text() == expectedTitle);
        CHECK_FALSE(revert->isVisible());
    };

    select("Alpha Feature");
    select(mode == "reopen" ? savedTitle : "Bravo Feature");
    if (mode == "rename") {
        checkMovieRename(window, list->selectedMovies(), library);
        return;
    }
    if (mode == "scrape") {
        checkMovieScrape(window, *title);
        return;
    }
    if (mode == "reopen") {
        return;
    }

    const auto typeTitle = [title](const char* text) {
        QTest::mouseClick(title, Qt::LeftButton);
        QTest::keyClick(title, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClicks(title, text);
        REQUIRE(title->text() == QLatin1String(text));
    };

    typeTitle("Discard This");
    REQUIRE(revert->isVisible());
    QTest::mouseClick(revert, Qt::LeftButton);
    CHECK(title->text() == "Bravo Feature");
    CHECK_FALSE(revert->isVisible());
    CHECK(readFile(library.filePath("Alpha/Alpha.nfo")) == originalA);
    CHECK(readFile(library.filePath("Bravo/Bravo.nfo")) == originalB);

    typeTitle(savedTitle);
    REQUIRE(save->isEnabled());
    const auto sidecar = library.filePath("Bravo/Bravo.nfo");
    const auto backup = library.filePath("Bravo/original.nfo.backup");
    REQUIRE(QFile::rename(sidecar, backup));
    REQUIRE(QDir().mkdir(sidecar));
    QTest::mouseClick(save, Qt::LeftButton);
    CHECK(title->text() == savedTitle);
    CHECK(revert->isVisible());
    CHECK(save->isEnabled());
    bool failureVisible = false;
    for (const auto* message : window.findChildren<Message*>()) {
        const auto* label = message->findChild<QLabel*>("label");
        failureVisible |= message->isVisible() && label && label->text().contains("Could not save movie metadata.");
    }
    CHECK(failureVisible);
    REQUIRE(QDir().rmdir(sidecar));
    REQUIRE(QFile::rename(backup, sidecar));
    QTest::mouseClick(save, Qt::LeftButton);
    CHECK_FALSE(revert->isVisible());
}
