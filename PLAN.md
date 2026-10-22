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
- [x] Rewrite ISSUES.md as a short, current backlog

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

- [x] GitHub Actions CI: Linux, Windows, macOS build + tests
- [x] GitHub Actions release: tagged builds packaged per platform and attached to a release
- [x] `/updateDependencies` pass: Actions pinned to SHAs (checkout 7.0.1, install-qt 4.4.1, upload 7.0.2,
      download 8.0.2, gh-release 3.0.3), Qt 6.12.0, aqtinstall pinned to upstream 076e1659 (Windows layout fix)
- [x] Found in CI: `#define private public` broke MSVC linking → friend test classes

## 6. Documentation ("flight manual" style: quick reference first, checklists, then detail)

- [x] README: crisp pitch that makes people want to play, then quick start
- [x] AGENTS.md: operating manual for coding agents (preflight, procedures, limits)
- [x] RULES.md: checked against code; End Run specified (§10, §12)
- [x] GitHub About, homepage (latest release) and ten topics applied (`/githubAbout`)

## 7. Ship

- [x] Pipeline green, CI green on Linux (6.12, 6.5), Windows, macOS; pushed to `origin/master`
- [x] Final self-review (`/code-review` on 64d07a1..HEAD): 8 findings, all fixed – press-only scale Behavior,
      fall-phase publish order (with regression test), hover refresh on grid/threshold change, End Run guard in
      C++, Windows logging left on the debugger output, tag version stamped into releases, bash < 4.4 array
      expansion, profiler test proves recording (mutation-checked)
- [x] Tag `v1.0.0` and publish a public GitHub release
- [x] Found after release: CI smoke test leaned on the CI Qt install, and native Controls styles ignored the
      theme on macOS/Windows → realistic smoke test, Basic style pinned, `v1.0.1` published (v1.0.0 marked
      superseded)

## 8. Second pass (v1.1.0) — ranked by severity × impact

- [x] P1 (S1) Linux release aborts on Wayland-only sessions ("Available platform plugins are: xcb"): bundle the
      Wayland platform plugin and smoke-test the archive under both X11 (Xvfb) and Wayland (headless Weston)
- [x] P2 (S1) macOS bundle: re-seal it with an ad-hoc signature after deployment rewrites the frameworks, and
      verify it in CI, so Apple silicon does not report the unsigned download as damaged
- [x] P3 (S2) AGENTS.md still recommends `#define private public`, which breaks MSVC linking → friend classes
- [x] P4 (S3) Sound effects: synthesized in-repo (no licences, no downloads), optional Qt Multimedia, mute toggle
      that persists, off switch for embedded builds
- [x] P5 (S3) Fresh screenshots and gameplay GIF generated from the real QML
- [x] P6 (S3) Refresh every Markdown document and the release notes
- [x] P7 Re-check aqtinstall: still 3.3.0 on PyPI → keep the upstream pin
- [x] P8 Release v1.1.0, verify the downloaded archives, then `/githubAbout` (add and remove topics): published,
      checksum verified, Linux archive started natively on Wayland; About updated (sound, Wayland/X11), topics
      `game`, `cross-platform` → `arcade-game`, `procedural-audio`
- [x] Found while testing: portrait/ultrawide frame left a big empty box → frame hugs the grid
- [x] Found in CI: runners have no audio device, so QSoundEffect cannot load there → WAV format checked from the
      resources, device loading only where an output exists
- Not doable here: Developer ID / Authenticode signing needs the owner's certificates (ISSUES.md #2 stays open)
