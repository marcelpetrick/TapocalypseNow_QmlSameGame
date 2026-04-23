cmake_minimum_required(VERSION 3.24)
project(SameGameQt6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)

# Find the Qt 6.5+ bits we actually use
find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Quick Qml Test)

qt_standard_project_setup()

# App
qt_add_executable(SameGame
    src/GameLogicBridge.cpp
    src/GameLogicBridge.h
    src/GameRules.cpp
    src/GameRules.h
    src/main.cpp
    src/HighScoreManager.cpp
    src/HighScoreManager.h
)

# QML module (resources get compiled into the app)
qt_add_qml_module(SameGame
    URI SameGame
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/GameBoard.qml
        qml/Tile.qml
        qml/ScoreDialog.qml
)

# Link the Qt libs needed by QGuiApplication, QML, and Quick
target_link_libraries(SameGame
    PRIVATE
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
)

target_include_directories(SameGame PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Put runtime in bin/ and use a lowercase binary name to avoid clashes
set_target_properties(SameGame PROPERTIES
    OUTPUT_NAME samegame
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS SameGame
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)

if(BUILD_TESTING)
    qt_add_executable(GameRulesTests
        src/GameRules.cpp
        src/GameRules.h
        tests/test_gamerules.cpp
    )

    target_link_libraries(GameRulesTests
        PRIVATE
            Qt6::Core
            Qt6::Test
    )

    target_include_directories(GameRulesTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    add_test(NAME GameRulesTests COMMAND GameRulesTests)

    qt_add_executable(HighScoreManagerTests
        src/HighScoreManager.cpp
        src/HighScoreManager.h
        tests/test_highscoremanager.cpp
    )

    target_link_libraries(HighScoreManagerTests
        PRIVATE
            Qt6::Core
            Qt6::Test
    )

    target_include_directories(HighScoreManagerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

    add_test(NAME HighScoreManagerTests COMMAND HighScoreManagerTests)
endif()
