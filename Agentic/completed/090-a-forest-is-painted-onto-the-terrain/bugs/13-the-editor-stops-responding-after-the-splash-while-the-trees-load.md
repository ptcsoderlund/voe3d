# 13 — The editor stops responding after the splash while the trees load

## Seen
"Starting up the editor is slow. It also becomes unresponsive for a few seconds. This is a small project, I
can imagine this does not scale well." The splash ends on "Loading models 2/2", then the window is frozen
and the desktop marks it "svarar inte" (not responding) until the first picture appears.

Measured on 2026-10-22 with lumberhill (`<settings>/voe3d/start.log`, and the same from `--capture`):
- debug: `scene and shapes 5.6 s`, then `first frame 27.9 s`.
- release: `scene and shapes 1.7 s`, then `first frame 9.6 s`.

During that first frame stderr says `game: models: 1 files in 5.711 s … decoding pictures 5.115 s` and
`game: materials: 4 files in 3.843 s … decoding pictures 3.556 s` (release). That file is the hill's
foliage kind, `Assets/Lanscapestuff/tree asset.glb`, with its four materials. The splash counted only the
scene's own two files, `Hill.landscape` and `wooden_stable.glb`.

## Expected
Everything the opened scene needs, a landscape's foliage kinds and their materials included, loads under
the splash, which counts it ("Loading models 3/3") and keeps the window answering. The first frame after
the splash is an ordinary frame, not a load, so a second kind of tree, or grass, makes the splash longer
and never freezes the window. The same holds for New and Open. How fast the pictures decode is not this
bug; that is 093's.

## How to reproduce
1. Open the editor on lumberhill, whose hill has trees painted on it.
2. Watch the splash end on "Loading models 2/2".
3. The window stops responding for about 10 s in release, about 28 s in debug, before the hill appears.
4. `start.log`'s last block shows `first frame` far longer than every other step.
