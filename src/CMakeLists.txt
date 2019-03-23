add_library(libmediaelch)

add_subdirectory(concerts)
add_subdirectory(data)
add_subdirectory(downloads)
add_subdirectory(export)
add_subdirectory(globals)
add_subdirectory(image)
add_subdirectory(media_centers)
add_subdirectory(movies)
add_subdirectory(music)
add_subdirectory(notifications)
add_subdirectory(renamer)
add_subdirectory(scrapers)
add_subdirectory(settings)
add_subdirectory(tvShows)
add_subdirectory(ui)
add_subdirectory(xbmc)

target_link_libraries(
  libmediaelch
  PUBLIC
    Qt5::Concurrent
    Qt5::Core
    Qt5::Gui
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    Qt5::Network
    Qt5::OpenGL
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Sql
    Qt5::Widgets
    Qt5::Xml
  PRIVATE
    quazip
    # MediaElch object libraries
    mediaelch_concert
    mediaelch_data
    mediaelch_downloads
    mediaelch_export
    mediaelch_globals
    mediaelch_image
    mediaelch_image_providers
    mediaelch_mediacenter
    mediaelch_movies
    mediaelch_music
    mediaelch_notifications
    mediaelch_renamer
    mediaelch_scrapers
    mediaelch_settings
    mediaelch_tvShows
    mediaelch_ui
    mediaelch_xbmc
)

# DL open/close
if(UNIX)
  target_link_libraries(libmediaelch PRIVATE curl)
  if(NOT APPLE)
    # libmediainfo requires dlopen/dlclose
    target_link_libraries(libmediaelch PRIVATE ${CMAKE_DL_LIBS})
  endif()

elseif(APPLE)
  find_package(Foundation REQUIRED)
  if(NOT FOUNDATION)
    message(FATAL_ERROR "Foundation framework not found")
  endif()
  target_link_libraries(libmediaelch PRIVATE ${FOUNDATION})

endif()

# ------------------------------------------------------------------------------
# Sources
target_sources(
  libmediaelch
  PRIVATE
  ../data/MediaElch.qrc
  ../data/i18n.qrc
  ../ui.qrc
)

# Headers that don't have a corresponding source file
target_sources(libmediaelch PRIVATE trailerProviders/TrailerProvider.h)

target_sources(
  libmediaelch
  PRIVATE
  sets/MovieListDialog.cpp
  sets/SetsWidget.cpp
  support/SupportDialog.cpp
  trailerProviders/HdTrailers.cpp
)

target_sources(
  libmediaelch
  PRIVATE
  main/AboutDialog.cpp
  main/FileScannerDialog.cpp
  main/MainWindow.cpp
  main/Message.cpp
  main/MyIconFont.cpp
  main/Navbar.cpp
  main/Update.cpp
)

target_sources(libmediaelch PRIVATE qml/AlbumImageProvider.cpp)

mediaelch_post_target_defaults(libmediaelch)