# 003 — Render to a texture, and save it

Approved 2026-09-15. Accepted 2026-09-15. Built ahead of 002 by the sponsor's choice on 2026-09-15, because it makes every later feature cheaper to verify.

## What

A program can draw a frame into a texture rather than onto the screen, and then do either of two
things with it: put it on something in the scene, or write it out as a PNG file. The sponsor wants
both halves available and the choice left to whoever is drawing — a developer wanting a camera
preview on a surface in the world, or an agent wanting to see what it just built. Today neither
exists: a picture has been hand-written three times as a throwaway program, and the last one had
to write out the PNG format by hand.

## Why

A picture of the engine had been hand-written three times as a throwaway program, the last one writing out the PNG format by hand; a developer wants a camera preview on a surface and an agent wants to see what it just built.

## How to test

1. A program draws a view into a texture and puts that texture on something in the scene — a
   second camera's view showing on a surface in the world — and the surface shows the view,
   updating as the scene changes.
2. A program saves a texture it has drawn to a PNG file. The file opens in an ordinary image
   viewer and shows what was drawn, at the size asked for, with the colours right.
3. Both are available separately: drawing into a texture does not require saving it, and saving
   does not require the texture to be in the scene.
4. The editor can be started so that it draws one frame with no window, writes a PNG and exits,
   naming the file and the size on the command line. Running that on this machine over a terminal,
   with no desktop, produces a picture of the editor.
5. A program that does not draw anything is unaffected: it still builds and runs without graphics
   being pulled in.
6. A picture saved with no display looks the same as the same frame drawn on screen.
7. `cmake -P check.cmake` exits zero on Linux.

## Out of scope

- Video, image sequences or animation — one picture per call.
- Image formats other than PNG.
- Reading an image file back in.
- A screenshot key inside the editor while you are using it (see Open questions).
- Capturing anything the engine did not draw — a desktop, another program's window.

## Constraints

- Linux first: acceptance is on Linux; Windows problems arrive later as reports.
- Drawing with no display must not become a requirement for programs that do not draw. A server
  that uses none of the drawing code must not be made to carry it.

## Defaults

- PNG, 8 bits per channel, no transparency unless what was drawn had it.
- The caller names the file and the size. Nothing is written anywhere the caller did not name.
- A failure to write — a bad path, a full disk — is reported to the caller, not a crash.

## Open questions

- Should the editor also have a key that saves what you are looking at, while you are using it?
  The sponsor approved the spec without it; it is worth asking again once the rest works.
