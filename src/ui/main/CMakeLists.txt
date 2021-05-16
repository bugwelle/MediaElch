add_library(
  mediaelch_ui_main OBJECT
  AboutDialog.cpp
  FileScannerDialog.cpp
  MainWindow.cpp
  Message.cpp
  MyIconFont.cpp
  Navbar.cpp
  Update.cpp
  QuickOpen.cpp
)

target_link_libraries(
  mediaelch_ui_main
  PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
          Qt${QT_VERSION_MAJOR}::MultimediaWidgets Qt${QT_VERSION_MAJOR}::Sql
)

if (ENABLE_SPELLCHECK)
  target_link_libraries(
    mediaelch_ui_main
          PRIVATE
            KF5::SonnetCore KF5::SonnetUi
  )
endif()

mediaelch_post_target_defaults(mediaelch_ui_main)
