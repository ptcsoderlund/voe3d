# 0321 — A local light has a reach and a falloff, and flashing is game code
date: 2026-10-02
by: tech-lead

## Decision
1. **Falloff.** Every light that sits at a place, the point light now and any later local light such as
   a spot light, has a `falloff` beside its `range`. The range is still where the light ends. Falloff
   sets how fast the light fades on the way out: low gives an even pool with a soft rim, high gives a
   bright core that dims quickly. The default looks exactly as 048's lights do today, so scenes saved
   before this keep their look. A directional light (sun, moon) never has a range or a falloff.
2. **No flash in the engine.** `voe_scene_point_light` loses `flash` and `flash_when_made`, along with the
   flash intent and the glow rows that count it down. A light shines at its intensity, and that's all.
   Game code that wants a flash fades the intensity itself. The tank game's shots and explosions keep
   their flashes this way.
3. Done inside 048 as its bug 01, before 048 is accepted.

The planner picks the falloff curve, the range of values it accepts, its control in the Inspector, and
what happens to `flash` fields in files already saved.

## Reasoning
The sponsor wants to choose how a lamp's light fades, and the reach alone can't do that. A fade shaped
by a curve keeps the sun's meaning of strength and the hard end at reach that 0320 chose. The engine's
flash only did what any game can already do with intensity, so it was a second way to do one thing.
- Soft edge as a fraction of reach: easy to picture, but a lamp with no fade has a hard rim.
- Physical inverse square: 0320's objection still holds, since a strength would no longer mean the sun's.
- Keep the flash as a convenience: two ways to light a shot, and only one is ever needed.

## Replaces
Decision 0320, points 1 and 2: the flash fields, the intents and the glow rows. Point 4's fade becomes
the default falloff.
