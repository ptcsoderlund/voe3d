# 0201 — The engine takes the fastest card and says which it took
date: 2026-09-20
by: tech-lead

## Decision
The engine chooses its graphics card by what the card is, never by the order the driver hands them
over, and it always takes the fastest one it can find. A discrete card beats an integrated one, an
integrated one beats a software rasteriser, and a software rasteriser is never chosen silently. There
is no power-saving choice and no override: this is a game engine, and performance is the default. If
the fastest card cannot present to the window the engine falls back to the next one down and says
that it did, rather than failing. Whatever it takes, it says on startup which card it took and why,
in one line. This holds for every program built from this tree: the editor, `voe_dev` and the
headless app alike. Letting a developer pick the card from code, and letting the player pick it in a
finished game's settings, are later work and deliberately not part of this.

## Reasoning
On this machine Vulkan enumerates the Intel integrated chip as device 0, the RTX 4070 as device 1,
and llvmpipe as device 2. Anything that takes the first device, or the first device that supports the
surface, lands on the Intel chip — which is also the chip the whole desktop is drawn on, so the
engine and the compositor fight over it. That fight is what 017 was written for. Taking the first
device is also one bad driver ordering away from rendering the editor on the CPU without anyone
noticing.

Alternatives: keeping the first supported device, which is what we appear to do now and is how we got
here; preferring the discrete card but allowing a power-saving override, rejected by the sponsor
because watt efficiency is not this engine's priority and a knob nobody has asked for is a knob to
maintain; letting the environment decide through the vendor's offload variables, rejected because it
is invisible, differs on every distribution, and does not exist on Windows.

The cost is real and accepted: on a hybrid laptop the display belongs to the integrated chip, so
rendering on the discrete one means the finished frame is copied across for scanout every frame, and
the discrete card costs watts and fans. That is the right trade for a game engine.

## Replaces
nothing
