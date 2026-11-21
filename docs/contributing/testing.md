# MediaElch Testing

__State__: last updated 2023-05-24

Table of contents:

 - [Test types and folder structure](#test-types-and-folder-structure)
 - [Compile and run Tests](#compile-and-run-tests)
 - [UI test plan](#ui-test-plan)
 - [Code Coverage](#code-coverage)
 - [Other checks](#other-checks)


## Test types and folder structure

MediaElch distinguishes between following test types, each of which has its
own subdirectory:

 - `unit`: Unit tests. Very fast to execute (<1s). No dependencies like reference files.
 - `scrapers`: Online scraper tests.  Requires an internet connection and
   can take two minutes or more to complete. 
 - `integration`: Integration tests which test all of MediaElch as one unit.
    Also contains unit-test-like tests for media_centers (Kodi NFO Tests).

`mocks` and `helpers` contain further C++ files that are helpful when writing tests.

The folder `resources` contains reference files.  These are files that we
load and compare a function's output against.  They are used for integration and
scraper tests.


## Compile and run Tests

As our test framework we use [Catch2](https://github.com/catchorg/Catch2).
With Catch2 you can create easy to write and easy to read tests.

In combination with CMake it's easy to test MediaElch:

```sh
# Run CMake with debug info enabled.
# We use Ninja as our build system instead of make.
cmake -S . -B build -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -GNinja
# Build all of MediaElch including test executables:
cd build && ninja

# You can also use the CMake preset `debug` from MediaElch's root,
# however, that also uses other tools such as USAN, Mold linker, etc.
#
#   cmake --preset debug && cmake --build --preset debug
#
# I recommend to use the AddressSanitizer preset:
#
#   cmake --preset asan && cmake --build --preset asan

# Execute all unit tests
ninja unit_test

# Execute all integration tests (resource paths are set via CMake)
ninja integration_test
# …or via direct executable (resource paths are set via macros through CMake at build time)
./test/integration/mediaelch_test_integration

# Execute all scraper tests (resource paths are set via CMake)
ninja scraper_test
# …or via direct executable (resource paths are set via macros through CMake at build time)
./test/scrapers/mediaelch_test_scrapers 

# The CMake targets are convenience for the long variants.
# Via compile definitions, the source directory path is part of the test executable,
# so explicit options are not necessary.
./test/scrapers/mediaelch_test_scrapers \
  --use-colour yes \
  --resource-dir ../test/resources \
  --temp-dir test/resources '[filter]'
```

There are three test executables in the build folder after compiling everything:

 - `build/test/unit/mediaelch_unit_test`
 - `build/test/scrapers/mediaelch_test_scrapers`
 - `build/test/scrapers/mediaelch_test_integration`

All use Catch2 and therefore have the same command line interface, e.g.

```sh
# Test options
./mediaelch_unit_test -h            # List Catch2 help
./mediaelch_unit_test -t            # List all tags
./mediaelch_unit_test -d yes        # Run *all* tests and print duration
./mediaelch_unit_test "[load_data]" # Run scraping tests (online test)
```

Since integration tests and scraper tests check their output against reference
files (found in `test/resources`), you may need to update them if you change
MediaElch's code.  
To ease that, you can set the environment variable `MEDIAELCH_UPDATE_REF_FILES`.

```sh
export MEDIAELCH_UPDATE_REF_FILES=1
ninja integration_test
ninja scraper_test
```


## UI test plan

The first release gate should answer a question the Kodi integration tests cannot answer on their own.
When a user selects one of two movies, edits its title and presses Save, does the edit reach the right NFO?
Build toward that check in three increments. Each increment should be independently runnable.

### 1. Establish the widget test harness

Add a proposed `test/ui/mediaelch_test_ui` executable under `ENABLE_TESTS`, with a `ui_test` convenience
target. Follow the integration runner's Catch2 and `QApplication` setup and the unit target's linkage to
`Qt${QT_VERSION_MAJOR}::Test`. Use Catch2 for assertions, QTest for keyboard and mouse input, and
QSignalSpy for observable notifications. Keep this target offline.

Start with `RatingsWidget`. Bind a local `Ratings` collection with one identifiable entry and show the widget.

- Click Add. The new row is selected, the backing collection grows, and the widget emits `ratingsChanged`.
- Open the new row's rating editor, type a value and commit with Return. Check the displayed value and
  the value in the corresponding backing entry. Remove the selected row and verify the original entry survives.
- Clear the selection, observe the signal count, then click Remove. The collection and signal count stay the same.

Locate controls by widget type and existing Designer object names such as `btnAddRating`. Locate table cells
through the view's model and visual rectangle. Send input through the control that handles it. For example,
typing in the movie title field exercises its `textEdited` connection, which calling `setText()` would bypass.
The first increment passes when the same cases run individually and together on Qt5 and Qt6 under Xvfb.

### 2. Protect the movie edit and save workflow

Prepare a fresh temporary library with two movie folders, distinct local NFO titles and matching media filenames.
Copy source fixtures before editing. Use recognizable values for both movies so a stale selection is detectable.
Use the production movie list, editor, Save action and Kodi writer for this workflow.

| Case | User action | Evidence of success |
| --- | --- | --- |
| Select | Scan the temporary library, wait for both entries, select A and then B. | The editor follows each selection. Loading either movie leaves it clean. |
| Discard an edit | Type a new title for B and click the visible Revert button. | The editor restores B's saved title, the dirty indication clears, and both NFO files retain their original contents. |
| Save and reopen | Edit B again, invoke Save, then reopen the library with a fresh cache. | B's NFO contains the new title, the reopened editor displays it, and A's NFO remains byte-for-byte identical. |

Assert the changed XML field by parsing it. The existing Kodi integration suite owns full serialization
comparisons. Include one full-window test for list selection and action routing. Keep additional title,
rating and validation cases in focused widgets so their failures identify the broken interaction.

Before constructing `Settings::instance()`, `Manager::instance()` or the main window, direct settings,
database, cache and library paths into a temporary profile. Verify the resolved paths, including the
portable INI path if portable mode is active. Disable startup update checks and remote artwork requests
in the fixture. Give each full-window scenario a fresh process if singleton state survives window teardown.
Finish or cancel outstanding workers before deleting the library. The exit criterion is a successful
save/reopen and revert run from an empty profile with external network access unavailable.

### 3. Expand by risk

| Priority | Workflow | Regression to exercise |
| --- | --- | --- |
| Next | Renamer preview and execution | Preview a rename in a disposable library and cancel. Verify the file tree is identical. Reopen and apply, then check the actual media and sidecar paths against the preview. |
| Next | TV show, season and episode selection | Select two sibling episodes across a season selection, edit the second episode and save. Verify its NFO changes and the sibling and show metadata survive. |
| Next | Scraper result selection and cancellation | Supply two fixed results at the scraper job boundary. Select the second and verify its metadata reaches the editor. Cancel a pending request and deliver a late completion. Verify the current item retains its original metadata. |
| Next | Failed save | Arrange a deterministic write failure in the temporary library. Check that the user can identify the failure, recover the edit and retry successfully. Record a product defect if the editor silently loses the edit. |
| Later | Settings, concerts and music | Persist a library setting across restart. Add one save/reload case each for a concert and an album, including selection of the album under the correct artist. |

Add a case when a UI regression escapes or a new interaction carries a distinct risk. Keep native file
choosers, platform shortcuts, high-DPI layouts and theme legibility on a short manual release pass on
the supported desktops. Record the OS, Qt version and observed failure for anything that needs reproduction.

### CI and failure diagnosis

Jenkins builds both Qt presets and currently runs its Xvfb test commands from `build/ci`. Run the proposed
UI executable under `xvfb-run -a` from **both** `build/ci` and `build/ci-qt6`. Give each invocation its own
JUnit filename under the collected reports directory and use Catch2's `--warn NoTests`. Keep test discovery
working without a display if registering cases with CTest, since discovery can run at build time.

Wait for a specific signal or observable state with a bounded event-loop wait. Attach observers before the
action so synchronous delivery is captured too. On timeout report the expected state, current selection,
active modal dialog and Qt version. Preserve the temporary NFO files on a save failure. Use screenshots
as diagnostic attachments when a window is visible. Before enabling the gate, repeat the suite in varied
case order on both Qt builds, check for leaked settings or files, and record runtime. A failing first run
should remain visible in CI even when a diagnostic rerun passes.


## Code Coverage

A CMake target exists to create Mediaelch's coverage: `coverage`

<!-- Note: Keep in sync with ../admin/coverity.md -->
```sh
# Use GCC; versions must match (also for gcov)
export CC=gcc
export CXX=g++
# Run CMake with coverage enabled and debug infos.
# We use Ninja as our build system instead of make.
export ASAN_OPTIONS=detect_leaks=0
cmake -S . -B build/coverage \
  -DDISABLE_UPDATER=ON \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSANITIZE_ADDRESS=ON \
  -DENABLE_TESTS=ON \
  -DENABLE_COVERAGE=ON \
  -DMEDIAELCH_FORCE_QT5=ON \
  -DMEDIAELCH_FORCE_QT6=OFF \
  -GNinja
# build all of MediaElch including test executables
cd build/coverage
ninja
# runs the target `test` (see above) and the creates an HTML
# coverage report in build/coverage`
ninja coverage
```

You can also use the CMake preset `ci`.

__Troubleshooting__:

 - _`.gcno:version '404*', prefer '407*'`_  
   See [StackOverflow](https://stackoverflow.com/questions/12454175/gcov-out-of-memory-mismatched-version).
   Likely a version mismatch, e.g. GCC12, GCOV11.
 - _No GCOV data generated_  
   If you have multiple builds next to each other, e.g. `build/asan` and `build/ci`,
   check that LCOV did not accidentally use `build/asan` when only `build/ci`
   has coverage enabled.
 - _Mismatch error_  
   See <https://github.com/linux-test-project/lcov/issues/209>.  In the past, adapting the coding slightly helped.

## Other checks

MediaElch uses Coverity for further security checks.  It's automated via Jenkins.
See [`coverity.md`](../admin/coverity.md).
