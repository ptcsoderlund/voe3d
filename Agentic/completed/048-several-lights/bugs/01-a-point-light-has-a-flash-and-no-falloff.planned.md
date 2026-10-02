# 01 — A point light has a flash and no falloff

## Seen
The point lights look good. The flash parameter isn't needed, because a flash can be scripted with the
intensity. What a point light lacks is a falloff: there is no way to choose how its light fades
between the lamp and its reach.

## Expected
See 0321. The point light shows Colour, Intensity, Range and Falloff in the Inspector. Flash and
"flash when made" are gone. Range is still where the light ends. Falloff is how fast it fades on the
way out: low gives an even pool with a soft rim, and high gives a bright core that dims quickly. A new
light, and every lamp already saved, looks as it does today. Directional lights get no falloff. In
Play, shots and explosions still light the ground for a moment, now by the tank game's own code
fading the intensity.

## How to reproduce
1. Open `examples/tank_game` at dusk (a dim orange sun) and add a point light near the ground.
2. Look in the Inspector: it shows Flash and Flash when made, and there is no Falloff.
3. After the fix, the lamp looks as it did before. Drag Falloff down and the pool turns even, with a
   soft rim. Drag it up and the light gathers at the lamp. Range still ends the pool in the same place,
   and undo brings the old falloff back.
4. Select the sun: it has no Falloff.
5. Play. Each shot flashes the ground around the barrel, and each explosion lights up its surroundings
   for a moment, as before.
6. The lamps look the same in the game as in the editor, at any falloff.
