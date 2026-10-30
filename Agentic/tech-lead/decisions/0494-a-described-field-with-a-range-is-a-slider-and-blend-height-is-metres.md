# 0494 — A described field with a range is a slider, and blend height is metres
date: 2026-10-30
by: tech-lead

## Decision
1. **A field's description may carry a range.** A described number field can say its least and
   greatest value. The editor draws a described field that has a range as a slider over that range,
   and one without a range as a number box, as today. A described field with a natural range is
   given one: the model's `blend_height` takes 0.05–10 m, the range its draw already clamps to.
2. **One quantity, one unit, one widget.** A value that means the same thing in two places is shown
   with the same unit and the same widget in both. Blend height is shown in metres with decimals on a
   foliage kind's Inspector row and on a model, both as a slider over 0.05–10 m. How each is stored
   is the planner's choice, but a project saved before this opens with the same blend heights.
3. **Order.** This is work order 104 and is built before the hill ships; the hill becomes 105 and
   stays 0.3's last work order.

## Reasoning
The two blend height inputs differ by accident. The kind's row is hand-drawn with a slider in
centimetres, and the model's field fell back to a number box in metres because descriptions had no
range. Alternatives: a slider written for the model's row alone fixes one field and leaves every
other ranged field a bare number box; making both number boxes throws away a range we know. A range
in the description fixes the cause once for every component's fields (0483: the work is no reason).

## Replaces
Amends 0481 point 4 (the kind's blend height is shown in metres, not centimetres) and 0479 (the hill
renumbered 103 → 105).
