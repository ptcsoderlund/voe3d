# 112 — Fog can be thicker

## What
A fog volume's Thickness slider in the Inspector goes up to 4 instead of 1. Fog at 4 is far denser than
at 1, thick enough to hide the trees a few metres inside it. A thickness I set before looks exactly as
it did (0499).

## Why
At thickness 1, the most I can set today, the fog is too thin for what I want.

## How to test
1. Open the tank project. Select the Fog entity. Its Thickness slider runs from 0 to 4 and shows 1.00.
   The fog looks as it did before.
2. Drag Thickness to 4. The fog becomes much denser: looking through the length of the box, I see
   nothing behind it.
3. Drag it back down through 2 and 1 to 0. The fog thins smoothly and is gone at 0.
4. Set it to 3, save, close and reopen the project. The slider shows 3.00 and the fog looks the same.
5. Press Play: the fog looks as it does in the editor's view.
