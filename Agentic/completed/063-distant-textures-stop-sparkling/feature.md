# 063 — Distant textures stop sparkling

## What
A textured model far away, or a textured surface seen at a low angle, looks smooth and stays still as the
camera moves, instead of sparkling and crawling. Up close, its texels are still hard-edged squares exactly as
today. Text and sprites look unchanged (0359).

## Why
Textures without mipmaps shimmer at a distance and waste GPU time reading them.

## How to test
1. Open the tank game. Fly the editor camera close to a textured model. Its texels are hard-edged squares, as
   before.
2. Back away until the model is small on screen, and move slowly side to side. The texture does not sparkle or
   crawl; it settles into a steady blend of its colours.
3. Look along a large textured surface at a low angle. The far end is calm, not a noisy band.
4. Look at editor text, world text and a sprite. They look the same as before.
5. Press Play and check the same in the game window.