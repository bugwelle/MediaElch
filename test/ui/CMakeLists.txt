add_executable(
  mediaelch_test_ui main.cpp testLibraryWorkflows.cpp testRatingsWidget.cpp
)

target_link_libraries(
  mediaelch_test_ui PRIVATE libmediaelch Qt${QT_VERSION_MAJOR}::Test
)
mediaelch_post_target_defaults(mediaelch_test_ui)

add_custom_target(
  ui_test
  COMMAND $<TARGET_FILE:mediaelch_test_ui> --warn NoTests
  DEPENDS mediaelch_test_ui
  USES_TERMINAL
)
