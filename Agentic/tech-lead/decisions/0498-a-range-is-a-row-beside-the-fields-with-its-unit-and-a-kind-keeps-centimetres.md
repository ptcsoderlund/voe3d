# 0498 — A range is a row beside the fields, with its unit, and a kind keeps centimetres
date: 2026-10-30
by: planner

## Decision
Feature 104 carries out 0494 so:

1. **A range sits beside the field list, as names do.** A ranges list invokes its one parameter once
   per ranged field as `R(field, least, greatest, unit)`. `base/describe.h` gains
   `voe_base_field_range` (field, least, greatest as doubles, unit), `ranges` and `ranges_count` on
   the struct description, `voe_base_ranges_find`, and the macros `VOE_BASE_DESCRIBE_STRUCT_RANGED`
   and `VOE_BASE_DESCRIBE_STRUCT_NAMED_RANGED`. The field's kind and its scene text do not change.
   A range is meant for a real-number field (FLOAT32, FLOAT64) of rank 0; a tool ignores it on any
   other.
2. **The unit travels with the range.** It is the word or symbol shown after the value: `m`, `°`,
   `stops`, `rad`; empty for a fraction from 0 to 1.
3. **One drawing for every ranged value.** The editor shows a ranged field as its title, a 40 mm
   slider (the Foliage section's width) and a figure: the value at two decimals, then the unit. A
   stored value outside the range shows at the nearer end and is not written until the person moves
   it. A foliage kind's Blend height row is drawn the same way, its ends the model header's least
   and greatest blend height, the constants the model's range is written with, so the two cannot
   drift apart.
4. **Storage stays as it is.** A model's `blend_height` stays a float in metres. A kind's stays whole
   centimetres in its look and in `blendheight<k>`, 0 still meaning 50. Its slider shows centimetres
   / 100 and a drag writes round(metres × 100), clamped 5–1000. Every project saved before reads the
   same heights, and two decimals of a metre are exactly the centimetres kept.
5. **The fields given a range** are the ones whose header states both ends, plus wind directions:
   clouds coverage and shadow_strength 0–1, wind_direction 0–360 °; fog volume thickness,
   patchiness and godrays_strength 0–1, wind_direction 0–360 °; sky horizon_height 0–1; sun
   disc_size 0–VOE_SCENE_LIGHT_DISC_SIZE_MAX °, glow_size 0–1; point light falloff
   FALLOFF_LEAST–FALLOFF_MOST; picture exposure ±VOE_SCENE_PICTURE_EXPOSURE_MAX stops; model fade
   0–1 and blend_height 0.05–10 m; emitter alpha_start and alpha_end 0–1; body slope_limit 0–π/2
   rad. Everything else stays a number box.

## Reasoning
Names already sit beside the field list with their own macro; a range is the same kind of note to a
tool. Putting the range on the field line would change the arity of every field list in the tree. A
slider is read with its unit, and the description is the only place that knows a field's unit
without the editor naming a component. Keeping the kind in centimetres needs no file change and no
migration, and the slider's two decimals of a metre are its whole precision anyway. Fields whose
header states only one end (intensity, speeds) have no natural top, so they stay number boxes.

## Replaces
Nothing. Carries out 0494 points 1 and 2.
