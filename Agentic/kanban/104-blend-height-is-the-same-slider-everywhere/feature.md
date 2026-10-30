# 104 — Blend height is the same slider everywhere

## What
A model's Blend height and a foliage kind's Blend height look and work the same: both are sliders,
both show metres with decimals, both run from 0.05 m to 10 m. Other number fields in the Inspector
that have a natural range show as sliders over that range, the way the foliage kind's settings do,
and fields without one stay number boxes. Projects saved before this open with the same blend
heights as before (0494).

## Why
Two inputs for the same thing, in different units, is confusing, and every ranged number in the
Inspector should be as easy to set as the foliage kind's.

## How to test
1. Open the hill. Select a model with Blend with ground on. Its Blend height is a slider showing
   metres, e.g. 0.50 m. Drag it to both ends: it stops at 0.05 m and 10 m, and the blend on the
   model follows as I drag.
2. Select the landscape and open a foliage kind's row in the Foliage section. Its Blend height is
   the same slider, in metres, with the same ends. A kind that blended at 50 cm before shows 0.50 m.
3. Set both to the same value, e.g. 1.20 m: a model and a tree beside it blend up to the same height.
4. Undo each change: the value and the look go back.
5. Save, close and reopen the project: both values are as I left them. Open a project saved before
   this change: its blend heights look as they did.
6. Click through a light, the clouds row and a fog volume. A value with a natural range is a slider
   that stops at its ends; a value without one (a position, say) is still a number box.
