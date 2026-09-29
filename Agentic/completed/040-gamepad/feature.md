# 040 — Gamepad

## What
The engine reads gamepads on Linux and Windows, next to keyboard and mouse, raw the way 0239 serves
input: sticks, triggers and buttons, from any common pad (Xbox, PlayStation, generic), plugged in
before or during play. A game can tell which pad is which, and when one is pulled out. The tank game
plays on a pad: the left stick drives, the right stick aims the turret, and a trigger fires. Keyboard
and mouse keep working, and whichever was touched last is the one in use.

## Why
Milestone 7 of 0268. A Gunsmoke-style game wants a pad in the hands.

## How to test
Have a gamepad, and a Windows machine for the last step.
1. Plug the pad in and Play the tank game. The left stick drives, the right stick turns the turret, and
   the trigger fires.
2. Let go of the sticks. The tank stops dead, with no drift.
3. Pull the pad out while playing. Nothing crashes, and the keyboard takes over. Plug it back in. The
   pad works again without a restart.
4. Start the game with the pad already plugged in. It works from the first frame.
5. Do steps 1 and 3 on Windows, with the shipped game.
