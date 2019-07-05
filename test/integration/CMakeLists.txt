add_executable(
  mediaelch_test_integration
  main.cpp
  resource_dir.cpp
  media_centers/testKodi_v16_movie.cpp
  media_centers/testKodi_v16_show.cpp
  media_centers/testKodi_v18_movie.cpp
  media_centers/testKodi_v18_show.cpp
)

target_link_libraries(
  mediaelch_test_integration
  PRIVATE libmediaelch libmediaelch_testhelpers
)

set_source_files_properties(main.cpp PROPERTIES COTIRE_EXCLUDED ON)
generate_coverage_report(mediaelch_test_integration)
catch_discover_tests(mediaelch_test_integration)
mediaelch_post_target_defaults(mediaelch_test_integration)

add_custom_target(
  integration_test
  COMMAND
    $<TARGET_FILE:mediaelch_test_integration>
    --use-colour yes
    --work-dir ${CMAKE_SOURCE_DIR}/test/resources
)