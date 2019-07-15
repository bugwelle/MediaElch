find_package(ZLIB REQUIRED)
find_package(Qt5 COMPONENTS Core REQUIRED)

add_library(QuaZip STATIC)

target_include_directories(
  QuaZip
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/quazip/quazip>
)
target_compile_definitions(QuaZip PRIVATE QUAZIP_BUILD)
target_compile_options(QuaZip PRIVATE -w) # Disable warnings for third-party
                                          # software

target_include_directories(
  QuaZip
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/quazip>
)

target_sources(
  QuaZip
  PRIVATE
  quazip/quazip/JlCompress.cpp
  quazip/quazip/qioapi.cpp
  quazip/quazip/quaadler32.cpp
  quazip/quazip/quacrc32.cpp
  quazip/quazip/quagzipfile.cpp
  quazip/quazip/quaziodevice.cpp
  quazip/quazip/quazip.cpp
  quazip/quazip/quazipdir.cpp
  quazip/quazip/quazipfile.cpp
  quazip/quazip/quazipfileinfo.cpp
  quazip/quazip/quazipnewinfo.cpp
  quazip/quazip/run_moc.bat
  quazip/quazip/unzip.c
  quazip/quazip/zip.c
)

target_link_libraries(QuaZip PUBLIC Qt5::Core ZLIB::ZLIB)
# We don't want e.g. clang-tidy warnings for QuaZip
set_target_properties(QuaZip PROPERTIES CXX_CLANG_TIDY "" CXX_CPPCHECK "")

add_library(QuaZip::QuaZip ALIAS QuaZip)
