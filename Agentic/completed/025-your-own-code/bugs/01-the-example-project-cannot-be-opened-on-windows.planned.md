# 01 — The example project cannot be opened on Windows

## Seen
On a Windows machine, opening the example project from the editor fails. It says
`VOE_GAME_LIBRARY is Linux-only for now`. The project's code is never built or loaded.

## Expected
On Windows, everything in 025's How to test works as on Linux: opening the example project builds
and loads its code, Keyboard Input, Player and Follow Camera are offered by Add component, Refresh
and Play work, and a failed build shows the Errors panel. Decision 0243.

## How to reproduce
1. On Windows, build and start the editor.
2. Open the example project.
3. The build refuses with `VOE_GAME_LIBRARY is Linux-only for now`.
