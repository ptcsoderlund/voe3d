# 0333 — A game's interface pauses the run and starts the level again
date: 2026-10-03
by: planner

## Decision
For 052, the seam a project's menus use to stop the game and to play it again:

1. **`game/project.h` gains `voe_game_project_asks`**, two bools: `paused` and `restart`. The
   run keeps one, both false at the start, and `voe_game_project_frame` carries a pointer to it
   (`asks`, never NULL). The project's interface writes it; the run reads it after the interface,
   each frame. `voe_game_interface_run` takes it and puts it in the frame.
2. **Paused, no fixed step runs.** The frame's elapsed time is dropped, not banked, so the game
   resumes where it was; the draw keeps its last lag; the interface still runs every frame, so
   the project can unpause. Every engine system is in the step, so bodies, particles, water,
   point lights and sounds all stop together.
3. **The mixer pauses with it: `voe_audio_mixer_pause(mixer, paused)`.** While paused, a mix
   writes silence and no voice advances, ramps or ends; starts, stops, tunes and sweeps act as
   ever, a started voice waiting. The pump still writes, so the device never starves.
4. **Restart builds the level again** at the start of the next frame, before its steps: the
   world's own arena cleared, a fresh `voe_game_world_new`, the project's register, the cooked
   scene built, the bank zeroed, the models read; then `restart` and `paused` are false and the
   mixer is unpaused. The old world's held voices end at the sound system's sweeps, as any
   voice whose thing is gone. A scene that no longer fits ends the run as at the start.

## Reasoning
A pause in game code alone stops only the project's systems: particles drift, water moves and
the engine's hum plays on under a pause menu. A reset in game code alone must undo every change
the game made, and a wrecked house is gone from the world; building the cooked scene again is
the one reset that cannot miss anything. Rejected: an enum answer from the interface (a pause is
a state across frames, not one frame's answer); the run starting paused (a project's state row
is made in a step, so it could never be made); a mixer stop-all at restart (the sweep already
ends voices whose owner is gone).

## Replaces
nothing. Amends 0259 point 2: the interface still returns false to end the run.
