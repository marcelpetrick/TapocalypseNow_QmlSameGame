# Backlog

No external board is used; this file is the backlog. Severity: **S1** breaks play or data · **S2** visible defect ·
**S3** polish / idea.

## Open

| # | Sev | Item | Notes |
|---|---|---|---|
| 1 | S3 | Sound effects for pop, fall and game over | Needs Qt Multimedia and audio assets; keep optional (off on embedded targets). |
| 2 | S3 | Signed and notarized macOS build, signed Windows build | Release archives are unsigned today. |
| 3 | S3 | Linux AppImage or Flatpak | Release ships a self-contained tarball. |
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

See [PLAN.md](PLAN.md) for the full list with details.
