# Tapocalypse Now — a Qt 6 / QML /C++ SameGame

A tiny, flashy SameGame clone built with Qt 6 (Quick + QML) and C++. Working title pays homage to a certain 1979 classic.

[https://github.com/marcelpetrick/TapocalypseNow_QmlSameGame](https://github.com/marcelpetrick/TapocalypseNow_QmlSameGame).

---

## Intention

* **Showcase** a clean, modern Qt 6 + QML app structure with a compiled QML module, a small C++ backend, and a minimal CMake setup.
* **Be hackable**: one file per QML type, tiny C++ class for highscores, and simple game logic to tweak.
* **Look nice**: neon theme, soft glows, subtle animations.

---

## What is SameGame?

SameGame is a classic tile-clearing puzzle: you click a tile to remove the **connected group** (4‑neighborhood) of tiles that share its color. After removal, tiles **fall down** to fill gaps; when a column is empty, columns **shift left**.

### Rules as implemented here

* You can only remove a **group of size ≥ 2**.
* **Score** for a move = *groupSize × groupSize*.
* The board **collapses down** and then **shifts left** after each move.
* The game **ends** when there are **no more adjacent pairs** (no valid groups ≥ 2).
* There is **no end‑game bonus** beyond the points you earn per move.

---

## Features

* **Neon UI** (QML/Qt Quick) with gentle animations and hover ripples.
* **Resizable grid** via `gridCols`/`gridRows` in `qml/main.qml` (default **16×10**).
* **Highscores** (top 10) with name + timestamp.

  * Stored as JSON at your platform’s *AppData* folder (see **Data location** below).
  * Ties: higher score first; if equal, **older entry wins**.
  * Names are trimmed and capped at **20 chars**.
* **One-click actions**: *New Game*, *Highscores*, *Reset Highscores*.

---

## Build & Run

### Requirements

* **Qt 6.5+** (modules: **Gui**, **Quick**, **Qml**)
* **CMake ≥ 3.24**
* A C++20 compiler (GCC/Clang/MSVC) and a build tool (e.g. Ninja or MSBuild)

The project uses a single top-level `CMakeLists.txt` that:

* Finds Qt components: `find_package(Qt6 6.5 REQUIRED COMPONENTS Gui Quick Qml)`
* Builds an app target called **`SameGame`** with output binary **`samegame`** into `build/bin/`
* Compiles QML resources into the executable as a QML module **`SameGame 1.0`**

### Quick start (Linux/macOS) - auto generated, not tested; usually built with QtCreator

```bash
# from repo root
mkdir -p build && cd build
# Update the Qt path to your installation
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/6.6.3/gcc_64 \
  ..
cmake --build .

# run
./bin/samegame
```

### Quick start (Windows, MSVC + Ninja) - auto generated, not tested

```bat
REM from repo root
mkdir build && cd build
cmake -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_PREFIX_PATH=C:\Qt\6.6.3\msvc2019_64 ^
  ..
cmake --build .

REM run
bin\samegame.exe
```

> **Tip:** If you installed Qt via the Maintenance Tool, the correct `CMAKE_PREFIX_PATH` is typically the *compiler‑specific* Qt directory (e.g. `.../msvc2022_64`, `.../gcc_64`, `.../clang_64`).

### Qt Creator

* Open `CMakeLists.txt` and configure a **Qt 6** kit.
* Build and **Run** from the IDE. The QML files are embedded; no extra deploy step is needed on desktop.

### Install (optional)

```bash
cmake --install . --prefix ./appdir
```

This places the runtime in `appdir/` (see the `install(TARGETS ...)` rule). On macOS/Windows you may still need to run **windeployqt**/**macdeployqt** for a redistributable bundle.

---

## How to play

* **Click** any tile that’s part of a group of **2+** same‑colored neighbors.
* Watch tiles **drop** and columns **shift**.
* Chain larger groups to earn **quadratic points** (`size²`).
* When **no moves remain**, the game ends. Use **New Game** to retry.

Controls are mouse‑only and visible in the toolbar: *New Game*, *Highscores*, *Reset Highscores*.

---

## Data location (Highscores)

Highscores are saved as `highscores.json` under your platform’s **AppData** directory via `QStandardPaths::AppDataLocation`. Typical paths:

* **Linux:** `~/.local/share/<org or app>/highscores.json`
* **macOS:** `~/Library/Application Support/<app>/highscores.json`
* **Windows:** `%AppData%\\<Company>\\<App>\\highscores.json`

> Exact subfolder names are managed by Qt and may vary by build metadata; check your console or search your profile if unsure.

To **reset**, use the in‑app button or delete the file while the app is closed.

---

## Customize

* **Grid size:** edit `gridCols` / `gridRows` in `qml/main.qml`.
* **Palette:** tweak `neonPalette()` in `qml/GameBoard.qml`.
* **Scoring / rules:** adjust `removeGroup()` and `collapse()` in `qml/GameBoard.qml`.
* **Highscore limits:** change `kMaxEntries` in `src/HighScoreManager.h`.

---

## Project layout

```
.
├─ CMakeLists.txt              # Qt 6 project + QML module
├─ qml/
│  ├─ main.qml                 # Window, HUD, board wiring
│  ├─ GameBoard.qml            # Core logic: grid, flood fill, scoring, collapse
│  ├─ Tile.qml                 # Visual tile, hover/press effects
│  └─ ScoreDialog.qml          # Highscore entry + list
└─ src/
   ├─ main.cpp                 # Engine setup, exposes HighScoreManager to QML
   ├─ HighScoreManager.h/.cpp  # Top‑10 highscores, JSON on disk
```

---

## License

GPLv3

---

## Credits

**Tapocalypse Now** — a tiny Qt 6/QML SameGame by *Marcel PEtrick (mail@marcelpetrick.it)*.
