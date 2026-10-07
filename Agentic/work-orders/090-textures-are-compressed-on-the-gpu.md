# 090 — Textures are compressed on the GPU

## What
Every material texture sits in video memory in a format the graphics card decodes itself, at a quarter of
today's size or less, and looks the same as before at a glance. Normal maps stay smooth. Compressing
happens once per texture, in the background, and is kept in the project's cache. A texture still being
compressed draws uncompressed until it is ready. The frame breakdown shows how much video memory textures
take. The glyph atlas and sprite sheets are not compressed (0359).

## Why
The ground's layers, the house, the trees and the rocks bring many large textures. Compressing them is the
biggest memory and bandwidth win on every card in 0318's range.

## How to test
1. Open the hill and note the textures' video memory in the frame breakdown.
2. Copy the project folder, delete the copy's `Cache/` folder, and open the copy. The splash shows the
   compression's progress. Afterwards the textures' video memory is a quarter or less of what it was.
3. Fly close to the ground, a log wall and a tree. They look the same as before at a glance, and the log
   bumps are still smooth.
4. Close and reopen. Nothing is compressed again.
5. Re-export a ground texture with a change. Within a second or so the change shows, and the editor does
   not stall.
6. Ship the project and run it. The textures are compressed there too.
