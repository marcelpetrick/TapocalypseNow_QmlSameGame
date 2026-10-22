# AGENTS.md — operating manual for coding agents

Read this before changing anything. Normal procedures first, then limits, then abnormal procedures.

## 1. Preflight

- [ ] `git status` is clean, or you know whose changes are pending.
- [ ] You read [RULES.md](RULES.md) if the change touches gameplay. RULES.md wins over code, UI text and README.
- [ ] You know which layer you are changing (see §3) and which tests cover it (see §4).

## 2. Normal procedures

| Task | Command |
|---|---|
| Configure + build | `cmake -S . -B build && cmake --build build -j` |
| Run all tests | `ctest --test-dir build --output-on-failure` (use `/usr/bin/ctest` if a pip shim shadows it) |
| Full quality gate | `./localPipeline.sh --noRun --noOpen` — **always pass both flags**; never launch the game or open browser tabs |
| Look at the UI | `TAPOCALYPSE_E2E_SCREENSHOT_DIR=<dir> ./build/EndToEndTests`, then read the PNGs |
| Run the app headless | `QT_QPA_PLATFORM=offscreen timeout 3 ./build/bin/tapocalypse --windowed --mute` (exit 124 = it ran) |
| Change a sound | Edit `tools/generate_sounds.py`, run `python3 tools/generate_sounds.py`, commit script and WAVs together |
| Refresh README media | `tools/make_media.sh` (screenshots, GIF, MP4 from the real QML; needs `ffmpeg`) |
| Build an AppImage | Deploy-install (`-DTAPOCALYPSE_DEPLOY_QT=ON`, `cmake --install`), then `tools/make_appimage.sh` (see the release workflow for the pinned tools) |

**Definition of done:** the pipeline summary has no `FAIL`, coverage stays ≥ 90 % lines, clang-format and qmlformat
change nothing, and new behaviour has a test.

**Commits:** one concern per commit, Conventional Commits (`fix(ui): …`, `feat(game): …`, `test(e2e): …`,
`build(deps): …`), body explains *why*. Keep [PLAN.md](PLAN.md) current when working through it.

## 3. Map of the system

| Layer | Files | Rules |
|---|---|---|
| Pure rules | `src/GameRules.*` | No Qt GUI types. Every rule change needs a RULES.md update and tests. |
| Controller | `src/GameBoardController.*` | Owns all game state and timers; QML only renders it. Registered via `QML_ELEMENT` in the `TapocalypseQml` module. |
| Wire format | `src/VariantConversions.h` | Single definition of grid / selection / motion maps shared with QML. |
| Persistence | `src/HighScoreManager.*` | Atomic writes via `QSaveFile`; notify through `scoresChanged`. |
| Sound | `src/SoundEffects.*`, `sounds/`, `tools/generate_sounds.py` | Cues are synthesized, never downloaded. Triggered from controller signals in `GameBoard.qml`; QML must tolerate `sounds: null`. |
| UI | `qml/*.qml` | Sizes derive from the window's `unit`; never hard-code pixels. Animate the `effect*` / offset channels in `Tile.qml`, never `x`, `y`, `scale` or `opacity` of bound items. |
| Build | `CMakeLists.txt`, `cmake/` | App, QML module library, tests, coverage targets, install/deploy. |
| CI | `.github/workflows/` | Actions pinned to commit SHAs with a version comment. Release smoke-tests Linux on X11 and Wayland. |

## 4. Tests to extend

- Rule change → `tests/test_gamerules.cpp` and, if it is a RULES.md invariant, `tests/test_rules_conformance.cpp`.
- Controller behaviour → `tests/test_gameboardcontroller.cpp` (white-box access through `friend class ::GameBoardControllerTest`; never `#define private public`, which
  breaks MSVC linking because MSVC mangles access specifiers).
- Anything the player sees or clicks → `tests/test_e2e.cpp`; items are found by `objectName`, so keep those stable.
- Sound → `tests/test_soundeffects.cpp` (cue choice, mute, WAV format) and the cue order in `test_e2e.cpp`. Tests
  call `setVolume(0)` and listen to `cuePlayed`; never assert on audio hardware, CI has none.

## 5. Limits

- Qt **6.5** is the minimum; CI proves it. Do not use APIs newer than 6.5 without a version guard.
- Do **not** add `pragma ComponentBehavior` — the installed qmlformat rejects it and the pipeline fails.
- Do not reintroduce a highscore reset in the UI; it was removed on purpose.
- Full screen is the default (kiosk/embedded target); keep `--windowed` working.
- `-DTAPOCALYPSE_SOUND=OFF` must keep building and passing (CI job "no sound"): guard Multimedia code with
  `TAPOCALYPSE_HAS_SOUND`.
- Linux bundles must keep both platform plugins (`xcb` and `wayland`, see `INCLUDE_PLUGINS` in `CMakeLists.txt`).
- Downloaded release tooling (appimagetool, AppImage runtime) is pinned to a release and verified by SHA-256; bump
  URL and checksum together.
- Performance target is an i.MX8M Mini: avoid per-tile work that scales with the board (convert controller data to
  JS once in `GameBoard.qml`, as the existing `grid`/`*Set` properties do).

## 6. Abnormal procedures

| Situation | Action |
|---|---|
| Coverage fails with gcov version / stamp mismatch or a gcovr "function on multiple lines" error | Rerun the pipeline: a compiler or `CMakeLists.txt` change triggers a clean coverage rebuild. Otherwise delete `*.gcda`/`*.gcno` under `build-coverage/`. Never `rm -rf` user-supplied paths. |
| `qmlformat execution failed` without a message | Look for `pragma ComponentBehavior` and remove it. |
| CI cannot install Qt on Windows | aqtinstall needs the pinned upstream commit (see the workflow comment) until a release supports the per-arch layout. |
| Tiles misplaced after resize | Something animates or assigns a bound geometry property; move it to an effect channel. |
| E2E test flaky on timing | Use `setAnimationTimingsForTesting` and `QTRY_*` waits, not fixed sleeps. |
| `QSoundEffect: playback of this format is not supported` / `pa_context_connect() failed` | No audio device (CI, containers). Expected; cues stay silent and `cuePlayed` still fires. |
| Linux release aborts with "Available platform plugins are: xcb" | The Wayland plugin was dropped from the bundle; restore `INCLUDE_PLUGINS qwayland …`. |
