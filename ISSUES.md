# Backlog

No external board is used; this file is the backlog. Severity: **S1** breaks play or data · **S2** visible defect ·
**S3** polish / idea.

## Open

| # | Sev | Item | Notes |
|---|---|---|---|
| 2 | S3 | Developer ID–signed and notarized macOS build, Authenticode-signed Windows build | Needs the owner's certificates as CI secrets. The macOS bundle is ad-hoc sealed today, so it opens after one right-click → Open. |
| 3 | S3 | Flatpak / Flathub listing | The AppImage covers single-file Linux installs; Flathub would add store discovery and updates. |
| 4 | S3 | Keyboard and gamepad play | Today the board is mouse/touch only. |
| 5 | S3 | Undo last move | Would need an explicit RULES.md decision on scoring. |

## Done in the October 2026 hardening pass

- Reset-highscores button removed; highscores written atomically; score list never stale; submit-once entry with
  trimmed names, 20-character limit and Enter-to-save.
- End Run banks a score early (RULES.md §10).
- Collapse animation no longer replays the drop-in or blinks; geometry bindings survive animations and resizes.
- Layout adapts to every window shape; grid never overflows; themed buttons; hover preview of the removable group.
- `console.log` reaches Qt Creator (stderr forced).
- New Game during the pop delay can no longer touch the new board (controller cancels every phase timer on reset).
- Rule conformance tests against RULES.md and end-to-end tests of the shipped QML.
- CI on Linux, Windows and macOS (Qt 6.12 and the 6.5 minimum) and tagged releases with self-contained archives.

## Done in the v1.1.0 pass

- Linux release runs natively on Wayland and X11 (Wayland platform plugin bundled; smoke-tested on both).
- Sound effects (#1): synthesized in-repo, optional Qt Multimedia, persistent mute (button, **M**, `--mute`).
- macOS bundle re-sealed ad hoc after deployment and verified in CI.
- Board frame hugs the grid on portrait and ultrawide windows.
- README screenshots and GIF regenerated from the real QML (`tools/make_media.sh`).
- Linux AppImage (#3, first half) next to the tarball, smoke-tested on X11 and Wayland; app icon and desktop entry.

See [PLAN.md](PLAN.md) for the full list with details.
