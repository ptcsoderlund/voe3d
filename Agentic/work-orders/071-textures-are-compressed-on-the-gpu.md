# 071 — Textures are compressed on the GPU

## What
Every model's textures sit in video memory in a format the graphics card decodes itself, at a quarter of
today's size or less, and look the same as before at a glance. Normal maps keep their bumps smooth and do
not turn blocky. Compressing happens once per texture, in the background on import, and is kept in the project's
cache, so reopening a project is not slowed down. A texture that is still being compressed draws
uncompressed until it is ready. The frame breakdown (062) shows how much video memory textures take. The
glyph atlas and sprite sheets are not compressed, as 0359 already has them.

## Why
The most hardware win there is: less memory and less bandwidth on every card in 0318's range, before any scene grows large.

## Models
From https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models: Sponza, FlightHelmet, DamagedHelmet, SciFiHelmet, CompareNormal.

## How to test
1. Open the scene with Sponza and the helmets from 070, and note the textures' video memory in the frame
   breakdown. The breakdown names it.
2. Make a copy of the project folder with its `Cache/` folder deleted, and open it. The splash shows the
   compression's progress. The editor then opens, and the textures' video memory is a quarter or less of
   what it was.
3. Fly close to the helmets and Sponza's walls. They look the same as before at a glance. Place CompareNormal:
   the bumps are still smooth.
4. Close and reopen the project. It opens without compressing anything again.
5. Re-export one of Sponza's textures with a change. Within a second or so the change shows, and the
   editor does not stall.
6. Ship the project and run the game. The textures are compressed there too.
