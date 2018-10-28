# cmake build of MediaElch

# Uncomment his to see all commands cmake actually executes
# set(CMAKE_VERBOSE_MAKEFILE ON)

cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)

project(mediaelch VERSION 2.6.0)

# don't process generated files - new in 3.10
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.10.0")
  cmake_policy(SET CMP0071 OLD)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON) # Find includes in corresponding build
                                  # directories
set(CMAKE_AUTOMOC ON) # Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOUIC ON) # Create code from a list of Qt designer ui files
set(CMAKE_AUTORCC ON) # For .qrc files

add_executable(mediaelch "")
target_compile_features(mediaelch PUBLIC cxx_std_14 c_std_11)
target_include_directories(mediaelch PRIVATE src)

# You may need to set CMAKE_PREFIX_PATH e.g. to ~/Qt/5.11.2/gcc_64/
find_package(
  Qt5
  COMPONENTS
    Core
    Widgets
    Gui
    Network
    Sql
    Concurrent
    Multimedia
    MultimediaWidgets
    Qml
    Quick
    QuickWidgets
    OpenGL
    Xml
    REQUIRED
)
target_link_libraries(
  mediaelch
  PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Gui
    Qt5::Network
    Qt5::Sql
    Qt5::Concurrent
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::OpenGL
    Qt5::Xml
)

# libmediainfo requires dlopen/dlclose
target_link_libraries(mediaelch PRIVATE ${CMAKE_DL_LIBS})

add_subdirectory(quazip)
target_link_libraries(mediaelch PRIVATE quazip_static)

# Warnings
if(GNU)
  target_compile_options(
    mediaelch
    PRIVATE
      -Wall -Wextra -pedantic
      # See https://kristerw.blogspot.com/2017/09/useful-gcc-warning-options-
      # not-enabled.html
      -Wunknown-pragmas
      -Wundef
      -Wold-style-cast
      -Wuseless-cast
      -Wdisabled-optimization
      -Wstrict-overflow=4
      -Winit-self
      -Wpointer-arith
      -Wduplicated-cond
      -Wjump-misses-init
      -Wdouble-promotion
      -Wshadow
      -Wduplicated-branches
      -Wrestrict
      -Wnull-dereference
      -Wlogical-op
      -Wunsafe-loop-optimizations
      -Wno-error=unsafe-loop-optimizations
      -Wformat=2
  )

elseif(Clang)
  target_compile_options(mediaelch PRIVATE -Wall -Wextra -pedantic)

endif()

# Installation
install(TARGETS mediaelch RUNTIME DESTINATION bin)

target_sources(mediaelch PRIVATE MediaElch.qrc i18n.qrc ui.qrc)

target_sources(
  mediaelch
  PRIVATE
  src/main.cpp
  src/concerts/ConcertController.cpp
  src/concerts/ConcertFilesWidget.cpp
  src/concerts/ConcertSearch.cpp
  src/concerts/ConcertSearchWidget.cpp
  src/concerts/ConcertStreamDetailsWidget.cpp
  src/concerts/ConcertWidget.cpp
  src/concerts/ConcertInfoWidget.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/data/Concert.cpp
  src/data/ConcertFileSearcher.cpp
  src/data/ConcertModel.cpp
  src/data/ConcertProxyModel.cpp
  src/data/Database.cpp
  src/data/ImageCache.cpp
  src/data/Movie.cpp
  src/data/MovieFileSearcher.cpp
  src/data/MovieFilesOrganizer.cpp
  src/data/MovieImages.cpp
  src/data/MovieModel.cpp
  src/data/MovieProxyModel.cpp
  src/data/Rating.cpp
  src/data/Storage.cpp
  src/data/StreamDetails.cpp
  src/data/Subtitle.cpp
  src/data/TvShow.cpp
  src/data/TvShowEpisode.cpp
  src/data/TvShowFileSearcher.cpp
  src/data/TvShowModel.cpp
  src/data/TvShowModelItem.cpp
  src/data/TvShowProxyModel.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/downloads/DownloadsWidget.cpp
  src/downloads/Extractor.cpp
  src/downloads/FileWorker.cpp
  src/downloads/ImportActions.cpp
  src/downloads/ImportDialog.cpp
  src/downloads/MakeMkvCon.cpp
  src/downloads/MakeMkvDialog.cpp
  src/downloads/MyFile.cpp
  src/downloads/UnpackButtons.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/export/ExportDialog.cpp
  src/export/ExportTemplate.cpp
  src/export/ExportTemplateLoader.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/globals/ComboDelegate.cpp
  src/globals/DownloadManager.cpp
  src/globals/DownloadManagerElement.cpp
  src/globals/Filter.cpp
  src/globals/Helper.cpp
  src/globals/ImageDialog.cpp
  src/globals/ImagePreviewDialog.cpp
  src/globals/Manager.cpp
  src/globals/NameFormatter.cpp
  src/globals/NetworkReplyWatcher.cpp
  src/globals/TrailerDialog.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/image/Image.cpp
  src/image/ImageCapture.cpp
  src/image/ImageModel.cpp
  src/image/ImageProxyModel.cpp
  src/image/ImageWidget.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/imageProviders/FanartTv.cpp
  src/imageProviders/FanartTvMusic.cpp
  src/imageProviders/FanartTvMusicArtists.cpp
  src/imageProviders/TheTvDbImages.cpp
  src/imageProviders/TMDbImages.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/main/AboutDialog.cpp
  src/main/FileScannerDialog.cpp
  src/main/MainWindow.cpp
  src/main/Message.cpp
  src/main/MyIconFont.cpp
  src/main/Navbar.cpp
  src/main/Update.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/mediaCenterPlugins/XbmcXml.cpp
  src/mediaCenterPlugins/kodi/ArtistXmlReader.cpp
  src/mediaCenterPlugins/kodi/ArtistXmlWriter.cpp
  src/mediaCenterPlugins/kodi/ConcertXmlWriter.cpp
  src/mediaCenterPlugins/kodi/ConcertXmlReader.cpp
  src/mediaCenterPlugins/kodi/EpisodeXmlReader.cpp
  src/mediaCenterPlugins/kodi/MovieXmlWriter.cpp
  src/mediaCenterPlugins/kodi/MovieXmlReader.cpp
  src/mediaCenterPlugins/kodi/TvShowXmlWriter.cpp
  src/mediaCenterPlugins/kodi/TvShowXmlReader.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/movies/CertificationWidget.cpp
  src/movies/FilesWidget.cpp
  src/movies/GenreWidget.cpp
  src/movies/MovieController.cpp
  src/movies/MovieDuplicateItem.cpp
  src/movies/MovieDuplicates.cpp
  src/movies/MovieMultiScrapeDialog.cpp
  src/movies/MovieSearch.cpp
  src/movies/MovieSearchWidget.cpp
  src/movies/MovieWidget.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/music/Album.cpp
  src/music/AlbumController.cpp
  src/music/Artist.cpp
  src/music/ArtistController.cpp
  src/music/MusicFileSearcher.cpp
  src/music/MusicFilesWidget.cpp
  src/music/MusicModel.cpp
  src/music/MusicModelItem.cpp
  src/music/MusicMultiScrapeDialog.cpp
  src/music/MusicProxyModel.cpp
  src/music/MusicSearch.cpp
  src/music/MusicSearchWidget.cpp
  src/music/MusicWidget.cpp
  src/music/MusicWidgetAlbum.cpp
  src/music/MusicWidgetArtist.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/notifications/NotificationBox.cpp
  src/notifications/Notificator.cpp
  src/qml/AlbumImageProvider.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/renamer/ConcertRenamer.cpp
  src/renamer/EpisodeRenamer.cpp
  src/renamer/MovieRenamer.cpp
  src/renamer/Renamer.cpp
  src/renamer/RenamerDialog.cpp
  src/renamer/RenamerPlaceholders.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/scrapers/AdultDvdEmpire.cpp
  src/scrapers/AEBN.cpp
  src/scrapers/CustomMovieScraper.cpp
  src/scrapers/HotMovies.cpp
  src/scrapers/IMDB.cpp
  src/scrapers/KinoDe.cpp
  src/scrapers/OFDb.cpp
  src/scrapers/TheTvDb.cpp
  src/scrapers/TMDb.cpp
  src/scrapers/TMDbConcerts.cpp
  src/scrapers/TvTunes.cpp
  src/scrapers/UniversalMusicScraper.cpp
  src/scrapers/VideoBuster.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/sets/MovieListDialog.cpp
  src/sets/SetsWidget.cpp
  src/settings/AdvancedSettings.cpp
  src/settings/DataFile.cpp
  src/settings/ExportTemplateWidget.cpp
  src/settings/Settings.cpp
  src/settings/SettingsWindow.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/smallWidgets/AlphabeticalList.cpp
  src/smallWidgets/Badge.cpp
  src/smallWidgets/ClosableImage.cpp
  src/smallWidgets/FilterWidget.cpp
  src/smallWidgets/ImageGallery.cpp
  src/smallWidgets/ImageLabel.cpp
  src/smallWidgets/LoadingStreamDetails.cpp
  src/smallWidgets/MediaFlags.cpp
  src/smallWidgets/MessageLabel.cpp
  src/smallWidgets/MusicTreeView.cpp
  src/smallWidgets/MyCheckBox.cpp
  src/smallWidgets/MyLabel.cpp
  src/smallWidgets/MyLineEdit.cpp
  src/smallWidgets/MySpinBox.cpp
  src/smallWidgets/MySplitter.cpp
  src/smallWidgets/MySplitterHandle.cpp
  src/smallWidgets/MyTableView.cpp
  src/smallWidgets/MyTableWidget.cpp
  src/smallWidgets/MyTableWidgetItem.cpp
  src/smallWidgets/MyTreeView.cpp
  src/smallWidgets/MyWidget.cpp
  src/smallWidgets/SearchOverlay.cpp
  src/smallWidgets/SlidingStackedWidget.cpp
  src/smallWidgets/TagCloud.cpp
  src/smallWidgets/TvShowTreeView.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/support/SupportDialog.cpp
  src/trailerProviders/HdTrailers.cpp
  src/tvShows/TvShowFilesWidget.cpp
  src/tvShows/TvShowMultiScrapeDialog.cpp
  src/tvShows/TvShowSearch.cpp
  src/tvShows/TvShowSearchEpisode.cpp
  src/tvShows/TvShowUpdater.cpp
  src/tvShows/TvShowWidget.cpp
  src/tvShows/TvShowWidgetEpisode.cpp
  src/tvShows/TvShowWidgetSeason.cpp
  src/tvShows/TvShowWidgetTvShow.cpp
  src/tvShows/TvTunesDialog.cpp
)

target_sources(
  mediaelch
  PRIVATE
  src/xbmc/XbmcSync.cpp
  src/data/ImdbId.cpp
  src/data/TmdbId.cpp
  src/data/TvDbId.cpp
  src/data/EpisodeNumber.cpp
  src/data/SeasonNumber.cpp
  src/data/Certification.cpp
  src/data/MovieCrew.cpp
)
