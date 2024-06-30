add_library(
  mediaelch_ui_scrapers OBJECT
  ScraperManager.cpp movie/AebnConfigurationView.cpp
  movie/TmdbMovieConfigurationView.cpp movie/ImdbMovieConfigurationView.cpp
)

target_link_libraries(
  mediaelch_ui_scrapers
  PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network
          Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::Xml
)
mediaelch_post_target_defaults(mediaelch_ui_scrapers)
