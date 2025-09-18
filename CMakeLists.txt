cmake_minimum_required(VERSION 3.21)
project(SameGameQt6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick Qml)

qt_standard_project_setup()

qt_add_executable(SameGame
src/main.cpp
src/HighScoreManager.cpp
src/HighScoreManager.h
)

qt_add_qml_module(SameGame
    URI SameGame
    VERSION 1.0
    QML_FILES
        qml/main.qml
        qml/GameBoard.qml
        qml/Tile.qml
        qml/ScoreDialog.qml
)

install(TARGETS SameGame
BUNDLE DESTINATION .
RUNTIME DESTINATION .
)
