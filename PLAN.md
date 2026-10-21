# Plan

Working plan for the October 2026 hardening pass. Updated with every commit.
Legend: `[x]` done · `[ ]` open · `[~]` in progress.

## 0. Baseline

- [x] Run `./localPipeline.sh --noRun` and record the starting state
- [x] Pipeline green: coverage 98.9% lines / 100% functions
- [x] Commit formatter drift (`style:` FrameProfiler, main.cpp, Tile.qml)
- [x] Fix the coverage stage: stale gcov data from an older GCC aborted gcovr
- [x] Add `--noOpen` so agent/CI runs do not open browser windows

## 1. Review findings (reviewBranch + /code-review on the whole tree)

- [x] R1 `src/main.cpp` – `HighScoreManager` is destroyed before the QML engine that references it
- [x] R2 `src/HighScoreManager.cpp` – non-atomic save; a failed write wipes the scoreboard silently
- [x] R3 `qml/ScoreDialog.qml` – score list is evaluated once and goes stale 
- [x] R4 `qml/Tile.qml` – every landed tile replays the spawn drop and blinks (opacity 0 → 1)
- [x] R5 `qml/Tile.qml` – animations overwrite the `y` and `scale` bindings (resize/press break)
- [x] R6 `qml/main.qml` – board sizing ignores height; overflows on short or portrait screens
- [x] R7 `qml/main.qml` – one-click, unconfirmed "Reset Highscores" (also in ISSUES.md: remove it)
- [x] R8 `qml/ScoreDialog.qml` – whitespace names accepted, no max length, no Enter-to-submit
- [x] R9 `src/main.cpp` – no organization/application name; storage path depends on binary name
- [x] R10 `src/GameLogicBridge.*` – unused in the app and duplicates controller conversion helpers
- [x] CR1 `localPipeline.sh` – compiler guard ran `rm -rf` on a user-supplied path → delete gcov files only
- [x] CR2 `qml/GameBoard.qml` – moving overlays started at opacity 0 (covered by R4)
- [x] CR3 `localPipeline.sh` – "Coverage Gate" never compared a threshold → `TAPOCALYPSE_COVERAGE_MIN_LINES`
- [x] CR4 `localPipeline.sh` – qmllint discovery only worked with Makefiles → generator-agnostic
- [x] CR5 `CMakeLists.txt` – coverage targets reused stale counters → clear `.gcda` first
- [x] CR6 `src/FrameProfiler.cpp` – frames timed on the GUI thread → render-thread timestamps
- [x] CR7 cleanups: per-tile QVariant copies, double grid publish, duplicated converters
- [x] Found while testing: grid overflowed width-limited boards (gap not in the cell formula)
- [x] Found while testing: installed qmlformat rejects `pragma ComponentBehavior`
- [x] Found while testing: orphaned gcov notes after a target move broke coverage

## 2. Unfinished items from ISSUES.md

- [x] Remove the "reset highscore" button and functionality
- [x] Collapse animation polish (covered by R4/R5)
- [x] "All groups of three must be removable" – verify with rule-level end-to-end tests
- [x] Option to abort a run: allow a qualifying score to be submitted ("End Run")
- [x] Highscore entry flow: clear post-save state, Enter submits, input reset
- [x] Console logging in Qt Creator (force stderr output / message pattern)
- [ ] Sound effects – deferred: needs Qt Multimedia and audio assets; kept in the backlog
- [ ] Rewrite ISSUES.md as a short, current backlog

## 3. Screen adaptation and UI/UX

- [x] Board fits both width and height of the available area
- [x] Header, fonts and margins scale with the window; minimum window size
- [x] Start at screen size; full screen stays default (kiosk/SBC), `--windowed` for desktop
- [x] Score dialog fits small screens

- [x] Themed buttons (NeonButton) and hover preview of the removable group

## 4. Tests

- [x] Cover `benchmarkStep`
- [x] End-to-end tests: load real `main.qml` offscreen, click tiles, play to game over
- [x] Rule conformance tests mapped to RULES.md sections (randomized property checks)

## 5. Build everywhere and release

- [ ] GitHub Actions CI: Linux, Windows, macOS build + tests
- [ ] GitHub Actions release: tagged builds packaged per platform and attached to a release
- [ ] `/updateDependencies` pass (pin Qt and Action versions)

## 6. Documentation ("flight manual" style: quick reference first, checklists, then detail)

- [ ] README: crisp pitch that makes people want to play, then quick start
- [ ] AGENTS.md: operating manual for coding agents (preflight, procedures, limits)
- [ ] RULES.md: check against code
- [ ] GitHub About text and topics (`/githubAbout`)

## 7. Ship

- [ ] Pipeline green, push to `origin/master`
- [ ] Tag `v1.0.0` and publish a public GitHub release
