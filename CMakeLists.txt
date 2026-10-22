cmake_minimum_required(VERSION 3.24)
project(Tapocalypse VERSION 1.1.1 LANGUAGES CXX)

# Version shown by the app and stamped into bundles. Releases pass the tag's version (which may carry a suffix).
set(TAPOCALYPSE_VERSION "${PROJECT_VERSION}" CACHE STRING "Version string reported by the app")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

option(TAPOCALYPSE_ENABLE_COVERAGE "Build with gcov/gcovr coverage instrumentation" OFF)
# Sound needs Qt Multimedia. Embedded images without it (or without audio) can switch it off; the game stays silent.
option(TAPOCALYPSE_SOUND "Play sound effects through Qt Multimedia" ON)

# Find the Qt 6.5+ bits we actually use
find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Quick Qml Test)
if(TAPOCALYPSE_SOUND)
    find_package(Qt6 6.5 COMPONENTS Multimedia)
    if(NOT Qt6Multimedia_FOUND)
        message(WARNING "Qt Multimedia not found: building without sound. Install it or pass -DTAPOCALYPSE_SOUND=OFF.")
    endif()
endif()

qt_standard_project_setup(REQUIRES 6.5)
# Newer Qt releases know more policies than 6.5; opt into the ones that apply to this layout.
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

if(TAPOCALYPSE_ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    else()
        message(FATAL_ERROR "TAPOCALYPSE_ENABLE_COVERAGE requires GCC or Clang.")
    endif()
endif()

# Shared production code so app and tests execute one instrumented copy.
add_library(TapocalypseCore STATIC
    src/GameLogicBridge.cpp
    src/GameLogicBridge.h
    src/GameRules.cpp
    src/GameRules.h
    src/HighScoreManager.cpp
    src/HighScoreManager.h
    src/VariantConversions.h
)

target_link_libraries(TapocalypseCore
    PUBLIC
        Qt6::Core
)

target_include_directories(TapocalypseCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# QML module "Tapocalypse": the UI plus the QML-registered controller. A static library so the app and the
# end-to-end tests load exactly the same compiled QML.
qt_add_library(TapocalypseQml STATIC)
qt_add_qml_module(TapocalypseQml
    URI Tapocalypse
    VERSION 1.0
    RESOURCE_PREFIX /
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Tapocalypse
    SOURCES
        src/GameBoardController.cpp
        src/GameBoardController.h
    QML_FILES
        qml/main.qml
        qml/GameBoard.qml
        qml/Tile.qml
        qml/ScoreDialog.qml
        qml/NeonButton.qml
)

target_link_libraries(TapocalypseQml
    PUBLIC
        TapocalypseCore
        Qt6::Qml
        Qt6::Quick
)

# Sound cues, synthesized by tools/generate_sounds.py. Without Qt Multimedia the cue player is silent.
add_library(TapocalypseSound STATIC
    src/SoundEffects.cpp
    src/SoundEffects.h
)

target_link_libraries(TapocalypseSound PUBLIC Qt6::Core)
target_include_directories(TapocalypseSound PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(TAPOCALYPSE_SOUND AND Qt6Multimedia_FOUND)
    target_link_libraries(TapocalypseSound PUBLIC Qt6::Multimedia)
    target_compile_definitions(TapocalypseSound PUBLIC TAPOCALYPSE_HAS_SOUND)
    qt_add_resources(TapocalypseSound tapocalypse_sounds
        PREFIX /
        FILES
            sounds/deal.wav
            sounds/gameover.wav
            sounds/highscore.wav
            sounds/land.wav
            sounds/pop1.wav
            sounds/pop2.wav
            sounds/pop3.wav
            sounds/pop4.wav
            sounds/reject.wav
    )
endif()

# Opt-in frame-time telemetry for field profiling (TAPOCALYPSE_PROFILE=1)
add_library(TapocalypseProfiler STATIC
    src/FrameProfiler.cpp
    src/FrameProfiler.h
)

target_link_libraries(TapocalypseProfiler PUBLIC Qt6::Quick)
target_include_directories(TapocalypseProfiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# App
qt_add_executable(Tapocalypse
    src/main.cpp
)

target_compile_definitions(Tapocalypse PRIVATE TAPOCALYPSE_VERSION="${TAPOCALYPSE_VERSION}")

# Window icon; the same SVG is the AppImage and desktop-entry icon (packaging/).
qt_add_resources(Tapocalypse tapocalypse_icon
    PREFIX /
    FILES packaging/tapocalypse.svg
)

# Link the Qt libs needed by QGuiApplication, QML, and Quick
target_link_libraries(Tapocalypse
    PRIVATE
        TapocalypseQml
        TapocalypseQmlplugin
        TapocalypseProfiler
        TapocalypseSound
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

# Put runtime in bin/ and use a lowercase binary name to avoid clashes. A GUI-subsystem executable on Windows and
# an app bundle on macOS, so neither opens a console window.
set_target_properties(Tapocalypse PROPERTIES
    OUTPUT_NAME tapocalypse
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME Tapocalypse
    MACOSX_BUNDLE_GUI_IDENTIFIER it.marcelpetrick.tapocalypse
    MACOSX_BUNDLE_BUNDLE_VERSION ${TAPOCALYPSE_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${TAPOCALYPSE_VERSION}
)

include(GNUInstallDirs)
install(TARGETS Tapocalypse
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# With TAPOCALYPSE_DEPLOY_QT, 'cmake --install' also copies the Qt libraries, plugins and QML modules the app needs,
# giving a self-contained folder or bundle for releases. Off by default on Linux, where distribution Qt packages are
# shared system libraries; the release workflow enables it with the official Qt binaries.
if(WIN32 OR APPLE)
    set(_tapocalypse_deploy_default ON)
else()
    set(_tapocalypse_deploy_default OFF)
endif()
option(TAPOCALYPSE_DEPLOY_QT "Bundle the Qt runtime on 'cmake --install'" ${_tapocalypse_deploy_default})

if(TAPOCALYPSE_DEPLOY_QT)
    # Qt deploys only its default platform plugin (xcb on Linux), so a bundle would abort on Wayland-only sessions.
    # Ship the Wayland platform plugin too: one plugin since Qt 6.10, a generic/EGL pair before.
    set(_tapocalypse_extra_plugins "")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(_tapocalypse_extra_plugins INCLUDE_PLUGINS qwayland qwayland-generic qwayland-egl)
    endif()
    qt_generate_deploy_qml_app_script(
        TARGET Tapocalypse
        OUTPUT_SCRIPT tapocalypse_deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
        ${_tapocalypse_extra_plugins}
    )
    install(SCRIPT ${tapocalypse_deploy_script})
endif()

if(BUILD_TESTING)
    qt_add_executable(GameRulesTests
        tests/test_gamerules.cpp
    )

    target_link_libraries(GameRulesTests
        PRIVATE
            TapocalypseCore
            Qt6::Core
            Qt6::Test
    )

    add_test(NAME GameRulesTests COMMAND GameRulesTests)

    qt_add_executable(GameBoardControllerTests
        tests/test_gameboardcontroller.cpp
    )

    target_link_libraries(GameBoardControllerTests
        PRIVATE
            TapocalypseQml
            Qt6::Core
            Qt6::Test
    )

    add_test(NAME GameBoardControllerTests COMMAND GameBoardControllerTests)

    qt_add_executable(GameLogicBridgeTests
        tests/test_gamelogicbridge.cpp
    )

    target_link_libraries(GameLogicBridgeTests
        PRIVATE
            TapocalypseCore
            Qt6::Core
            Qt6::Test
    )

    add_test(NAME GameLogicBridgeTests COMMAND GameLogicBridgeTests)

    qt_add_executable(HighScoreManagerTests
        tests/test_highscoremanager.cpp
    )

    target_link_libraries(HighScoreManagerTests
        PRIVATE
            TapocalypseCore
            Qt6::Core
            Qt6::Test
    )

    add_test(NAME HighScoreManagerTests COMMAND HighScoreManagerTests)

    qt_add_executable(RulesConformanceTests
        tests/test_rules_conformance.cpp
    )

    target_link_libraries(RulesConformanceTests
        PRIVATE
            TapocalypseCore
            Qt6::Core
            Qt6::Test
    )

    add_test(NAME RulesConformanceTests COMMAND RulesConformanceTests)

    qt_add_executable(FrameProfilerTests
        tests/test_frameprofiler.cpp
    )

    target_link_libraries(FrameProfilerTests
        PRIVATE
            TapocalypseProfiler
            Qt6::Quick
            Qt6::Test
    )

    add_test(NAME FrameProfilerTests COMMAND FrameProfilerTests)
    set_tests_properties(FrameProfilerTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    qt_add_executable(SoundEffectsTests
        tests/test_soundeffects.cpp
    )

    target_link_libraries(SoundEffectsTests
        PRIVATE
            TapocalypseSound
            Qt6::Core
            Qt6::Test
    )

    add_test(NAME SoundEffectsTests COMMAND SoundEffectsTests)

    qt_add_executable(EndToEndTests
        tests/test_e2e.cpp
    )

    target_link_libraries(EndToEndTests
        PRIVATE
            TapocalypseQml
            TapocalypseQmlplugin
            TapocalypseSound
            Qt6::Gui
            Qt6::Quick
            Qt6::Test
    )

    add_test(NAME EndToEndTests COMMAND EndToEndTests)
    set_tests_properties(EndToEndTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    if(TAPOCALYPSE_ENABLE_COVERAGE)
        find_program(GCOVR_EXECUTABLE gcovr)
        find_program(GCOV_EXECUTABLE gcov)

        if(GCOVR_EXECUTABLE AND GCOV_EXECUTABLE)
            set(TAPOCALYPSE_COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
            set(TAPOCALYPSE_COVERAGE_HTML_DIR "${TAPOCALYPSE_COVERAGE_OUTPUT_DIR}/html")

            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/RemoveGcda.cmake
                COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
                COMMAND ${CMAKE_COMMAND} -E make_directory ${TAPOCALYPSE_COVERAGE_OUTPUT_DIR}
                COMMAND ${GCOVR_EXECUTABLE}
                    --root ${CMAKE_SOURCE_DIR}
                    --filter ^src/
                    --exclude ^src/main.cpp$
                    --exclude-throw-branches
                    --exclude-unreachable-branches
                    --gcov-executable ${GCOV_EXECUTABLE}
                    --object-directory ${CMAKE_BINARY_DIR}
                    --txt ${TAPOCALYPSE_COVERAGE_OUTPUT_DIR}/coverage.txt
                    --json-summary ${TAPOCALYPSE_COVERAGE_OUTPUT_DIR}/summary.json
                    --json-summary-pretty
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                DEPENDS GameRulesTests GameBoardControllerTests GameLogicBridgeTests HighScoreManagerTests RulesConformanceTests FrameProfilerTests SoundEffectsTests EndToEndTests
                USES_TERMINAL
                COMMENT "Running tests and generating text/json coverage report"
            )

            add_custom_target(coverage-html
                COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_BINARY_DIR} -P ${CMAKE_SOURCE_DIR}/cmake/RemoveGcda.cmake
                COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
                COMMAND ${CMAKE_COMMAND} -E make_directory ${TAPOCALYPSE_COVERAGE_HTML_DIR}
                COMMAND ${GCOVR_EXECUTABLE}
                    --root ${CMAKE_SOURCE_DIR}
                    --filter ^src/
                    --exclude ^src/main.cpp$
                    --exclude-throw-branches
                    --exclude-unreachable-branches
                    --gcov-executable ${GCOV_EXECUTABLE}
                    --object-directory ${CMAKE_BINARY_DIR}
                    --html ${TAPOCALYPSE_COVERAGE_HTML_DIR}/index.html
                    --json-summary ${TAPOCALYPSE_COVERAGE_OUTPUT_DIR}/summary.json
                    --json-summary-pretty
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                DEPENDS GameRulesTests GameBoardControllerTests GameLogicBridgeTests HighScoreManagerTests RulesConformanceTests FrameProfilerTests SoundEffectsTests EndToEndTests
                USES_TERMINAL
                COMMENT "Running tests and generating HTML coverage summary"
            )
        else()
            message(STATUS "Coverage targets disabled: gcovr and gcov are required.")
        endif()
    endif()
endif()
