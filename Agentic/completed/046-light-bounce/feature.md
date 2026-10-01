# 046 — Light bounce

## What
Sunlight bounces. A lit surface lights what is near it, in its own colour: a sunny red wall throws a
red tint on the ground beside it, and shadowed places are lit by what is around them instead of by a
flat fill. It happens while running, in the editor and the game, with no bake step and nothing to
press. It follows the sun when it turns and things when they move, settling within a moment. The
technique is chosen by its own decision, as 0268 says. The shape is the one the ideas list records:
a lit surface becomes a light, cached.

## Why
Milestone 13 of 0268. Real bounce light is what makes a level look lit rather than coloured.

## How to test
1. In the editor, place a bright red wall in full sun beside grey ground. The ground near the wall
   turns pink, fading with distance.
2. Turn the sun away from the wall. The pink fades within a second.
3. Look into a deep shadow next to lit ground. It is lit softly by the ground, not flat grey.
4. Drive a tank past the wall in Play. The bounce follows without flicker.
5. The tank game at full action stays smooth with bounce on.
