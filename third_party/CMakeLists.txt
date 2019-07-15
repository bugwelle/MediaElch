# -----------------------------------------------------------------------------
# zip

if(WIN32)
  include(ExternalProject)

  set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)
  find_program(MAKE_EXE NAMES gmake nmake make)
  ExternalProject_Add(
    zlib_dep
    URL http://www.winimage.com/zLibDll/zlib123.zip
    URL_MD5 abbd0f2b456206da5e3ffd179324413a
    TIMEOUT
      15 # max 15 seconds
    SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/zlib"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND "${MAKE_EXE}"
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    BUILD_IN_SOURCE ON
  )
endif()

# -----------------------------------------------------------------------------
# QuaZip

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

if(NOT WIN32)
  find_package(ZLIB REQUIRED)
  target_link_libraries(QuaZip PUBLIC Qt5::Core ZLIB::ZLIB)

else()
  add_dependencies(QuaZip zlib_dep)
  target_link_libraries(QuaZip PUBLIC Qt5::Core z)
endif()

# We don't want e.g. clang-tidy warnings for QuaZip
set_target_properties(QuaZip PROPERTIES CXX_CLANG_TIDY "" CXX_CPPCHECK "")

add_library(QuaZip::QuaZip ALIAS QuaZip)
