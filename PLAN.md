# Plan

Working plan for the October 2026 hardening pass. Updated with every commit.
Legend: `[x]` done · `[ ]` open · `[~]` in progress.

## 0. Baseline

- [x] Run `./localPipeline.sh --noRun` and record the starting state
- [x] Commit formatter drift (`style:` FrameProfiler, main.cpp, Tile.qml)
- [x] Fix the coverage stage: stale gcov data from an older GCC aborted gcovr

## 1. Review findings (reviewBranch + /code-review on the whole tree)

- [ ] R1 `src/main.cpp` – `HighScoreManager` is destroyed before the QML engine that references it
- [ ] R2 `src/HighScoreManager.cpp` – non-atomic save; a failed write wipes the scoreboard silently
- [ ] R3 `qml/ScoreDialog.qml` – score list is evaluated once and goes stale (reset, browsing)
- [ ] R4 `qml/Tile.qml` – every landed tile replays the spawn drop and blinks (opacity 0 → 1)
- [ ] R5 `qml/Tile.qml` – animations overwrite the `y` and `scale` bindings (resize/press break)
- [ ] R6 `qml/main.qml` – board sizing ignores height; overflows on short or portrait screens
- [ ] R7 `qml/main.qml` – one-click, unconfirmed "Reset Highscores" (also in ISSUES.md: remove it)
- [ ] R8 `qml/ScoreDialog.qml` – whitespace names accepted, no max length, no Enter-to-submit
- [ ] R9 `src/main.cpp` – no organization/application name; storage path depends on binary name
- [ ] R10 `src/GameLogicBridge.*` – unused in the app and duplicates controller conversion helpers
- [ ] Findings from the `/code-review` pass (merged here when it reports)

## 2. Unfinished items from ISSUES.md

- [ ] Remove the "reset highscore" button and functionality
- [ ] Collapse animation polish (covered by R4/R5)
- [ ] "All groups of three must be removable" – verify with rule-level end-to-end tests
- [ ] Option to abort a run: allow a qualifying score to be submitted ("End Run")
- [ ] Highscore entry flow: clear post-save state, Enter submits, input reset
- [ ] Console logging in Qt Creator (force stderr output / message pattern)
- [ ] Sound effects – decision needed (out of scope unless requested: needs Qt Multimedia)
- [ ] Rewrite ISSUES.md as a short, current backlog

## 3. Screen adaptation and UI/UX

- [ ] Board fits both width and height of the available area
- [ ] Header, fonts and margins scale with the window; minimum window size
- [ ] Start windowed-maximized on desktop, full-screen opt-in (`--fullscreen`)
- [ ] Score dialog fits small screens

## 4. Tests

- [ ] Cover `benchmarkStep`
- [ ] End-to-end tests: load real `main.qml` offscreen, click tiles, play to game over
- [ ] Rule conformance tests mapped to RULES.md sections (randomized property checks)

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
