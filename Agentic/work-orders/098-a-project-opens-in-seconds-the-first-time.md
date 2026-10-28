# 098 — A project opens in seconds the first time

## What
Opening a project for the first time, with nothing in its Cache folder, takes seconds, not 15 minutes. The
textures are compressed on the graphics card, not the processor. They look as they do today, and they are as
small in video memory. Changing a texture in the editor shows the compressed result almost at once. Play,
Ship and a shipped game's first start no longer wait on compression either (0484).

## Why
Today a project with a couple of models and textures takes 15 minutes or more to open the first time. Nobody can
work like that, and the hill's world will have many more textures.

## How to test
1. Close the editor. Delete the project's Cache folder. Open the project in the debug build I normally run. The
   splash shows its steps and the project is ready in under 30 seconds.
2. Look at the models, the ground and the materials up close and far away. They look as before: no blocky
   patches, no colour bands, normal maps as smooth as before.
3. Open the Frame panel. Every material texture is counted as compressed, and the texture memory is about what
   it was before.
4. Close and open the project again. It opens without compressing anything.
5. Change a material's texture in the Inspector to another picture. Within about a second it shows, compressed,
   and the view does not stutter.
6. Import a model whose textures are 4096 on a side and place it. It is ready within a few seconds.
7. Delete the Cache folder again and press Play. Play starts within a few seconds of the usual time. Ship the
   project, delete the shipped game's cache if it has one, and run it. It starts without a long wait and looks as
   in the editor.
