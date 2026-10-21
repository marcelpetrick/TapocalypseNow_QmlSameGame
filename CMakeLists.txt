cmake_minimum_required(VERSION 3.24)
project(Tapocalypse VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

option(TAPOCALYPSE_ENABLE_COVERAGE "Build with gcov/gcovr coverage instrumentation" OFF)

# Find the Qt 6.5+ bits we actually use
find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Quick Qml Test)

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

target_compile_definitions(Tapocalypse PRIVATE TAPOCALYPSE_VERSION="${PROJECT_VERSION}")

# Link the Qt libs needed by QGuiApplication, QML, and Quick
target_link_libraries(Tapocalypse
    PRIVATE
        TapocalypseQml
        TapocalypseQmlplugin
        TapocalypseProfiler
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

# Put runtime in bin/ and use a lowercase binary name to avoid clashes
set_target_properties(Tapocalypse PROPERTIES
    OUTPUT_NAME tapocalypse
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS Tapocalypse
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

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

    qt_add_executable(EndToEndTests
        tests/test_e2e.cpp
    )

    target_link_libraries(EndToEndTests
        PRIVATE
            TapocalypseQml
            TapocalypseQmlplugin
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
                DEPENDS GameRulesTests GameBoardControllerTests GameLogicBridgeTests HighScoreManagerTests RulesConformanceTests FrameProfilerTests EndToEndTests
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
                DEPENDS GameRulesTests GameBoardControllerTests GameLogicBridgeTests HighScoreManagerTests RulesConformanceTests FrameProfilerTests EndToEndTests
                USES_TERMINAL
                COMMENT "Running tests and generating HTML coverage summary"
            )
        else()
            message(STATUS "Coverage targets disabled: gcovr and gcov are required.")
        endif()
    endif()
endif()
