# 0304 — A sound is a voice with a handle, and a component in `audio` placed from the camera
date: 2026-09-30
by: planner

## Decision
For 044, filling in what the feature, 0265 and 0266 leave to the planner:

1. **`audio` gains the edges `scene ecs math`** in `cmake/voe.cmake`, so the sound component and
   its system live beside the mixer, as the emitter lives in `3d` (0298). All three sit above
   `audio` in the map; nothing below names `audio`.
2. **A voice has a handle.** `voe_audio_voice` is a `uint32_t id`, 0 none: slot and generation,
   so a handle to a finished or stolen voice is stale and every call on it is a no-op.
   `voe_audio_mixer_start(mixer, voe_audio_start)` takes path, loop, held, volume, pitch and an
   optional place, and returns the handle; `_play(path)` stays as the one-shot shorthand the
   coin uses. `_stop`, `_playing`, `_tune(volume, pitch)` and `_move(where)` act on a handle.
3. **Loop, pitch, volume.** A loop wraps its read position without a gap, interpolating across
   the wrap. Pitch is the read rate, linear interpolation, clamped to [0.25, 4]; volume is a
   gain clamped to [0, 4]. A change of gain or pan ramps linearly across the next mix call, so
   a per-step change never clicks. `VOE_AUDIO_VOICES` becomes 32. A full mixer takes the oldest
   voice that does not loop; when every voice loops the start is refused and returns 0.
4. **Held voices are swept.** A voice started `held` must be tuned or moved between two calls
   of `voe_audio_mixer_sweep`, or that sweep stops it. The sound system holds its voices and
   sweeps each run, so a removed thing's sound stops the step after it goes, however it went.
5. **Placement is the mixer's, from a listener.** `audio/place.h`: `voe_audio_listener` is the
   camera's world position (double), its right and forward (unit float3), the tangent of half
   its horizontal field of view, and a reference distance. `voe_audio_place(listener, where)`
   gives a gain and a pan. Gain is `min(1, (reference / distance)^2)`. Pan is the point's
   screen x: `dot(d, right) / (dot(d, forward) * tan_half_width)` clamped to ±1, or the sign
   of `dot(d, right)` when the point is not in front. Left and right gains are the balance law
   `min(1, 1 - pan)` and `min(1, 1 + pan)`, so an unplaced voice (the coin) is as loud as
   before. The reference is the distance from the camera to where its view axis meets the
   ground plane y = 0, or `VOE_AUDIO_REFERENCE` (10 m) when the axis does not reach it ahead:
   the centre of a top-down screen is full, its edges quieter, what is off screen quieter
   still. With no listener set, a placed voice plays as an unplaced one.
6. **The component.** `voe_audio_sound` at "Audio / Sound", needing nothing: path (CHAR 128,
   project-relative, empty is silence), playing (default true, so an authored sound plays at
   once), loop (false), volume (1), pitch (1). A row whose entity has a transform is placed at
   its world position and follows it; one without plays unplaced. Intents: the replace (whole
   row) and a control: play (from the start, again if it plays), stop, tune (volume, pitch).
   A runtime-only row, `voe_audio_sound_voice`, holds the handle and a restart flag.
7. **The system.** `voe_audio_sound_system_run(world, mixer, aspect)` drains both intents into
   the rows, adds and drops runtime rows, and, with a mixer, sets the listener from the first
   camera with a transform, reconciles each row with its voice (start, restart, stop, tune,
   move), clears `playing` on a one-shot whose voice ended, and sweeps. A NULL mixer only
   drains and adds: `voe_game_world_step` runs it so, which is how the editor applies
   Inspector edits and plays nothing (0266 point 6). The game runs it with the mixer last in
   each fixed step, the aspect from the window (1 headless).
8. **The tank game's sounds** are four WAVs in `examples/tank_game/Assets/sounds/`, synthesised
   once by the coder with a throwaway script that is not kept: an engine hum of whole periods
   that loops without a seam, a shot, a hit and an explosion, each mono 48 kHz PCM16.

## Reasoning
A handle is the smallest thing game code can hold to change a sound while it plays; a stale
one being a no-op keeps a stolen voice from being a crash. Placing in the mixer lets a one-shot
at a spot (a hit, where no thing remains) use the same law as a sound on a thing. Sweeping held
voices stops a removed thing's sound without the system keeping state outside the world or
the ecs growing a removal hook. A reference taken from the camera's own view keeps a high
top-down camera from making everything quiet. Rejected: a listener component (one camera is
0303's rule already), panning by angle alone (the feature says by where it is on screen),
constant-power panning (would make the coin quieter).

## Replaces
nothing; amends 0266 point 4 (the voice count and the steal rule).
