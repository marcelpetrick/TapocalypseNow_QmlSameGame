cmake_minimum_required(VERSION 3.24)
project(SameGameQt6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the Qt 6.5+ bits we actually use
find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Quick Qml)

qt_standard_project_setup()

# App
qt_add_executable(SameGame
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

# Put runtime in bin/ and use a lowercase binary name to avoid clashes
set_target_properties(SameGame PROPERTIES
    OUTPUT_NAME samegame
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS SameGame
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)
