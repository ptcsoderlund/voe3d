# 0324 — Cast shadows is a bool on the light, the shape and the model, and an old file keeps its sun's shadows
date: 2026-10-02
by: planner

## Decision
For 049, carrying out 0301 and 0316:
1. **The fields.** `voe_scene_light`, `voe_3d_shape` and `voe_3d_model` each gain `cast_shadows`, a
   `bool` described as BOOL, last in their field lists. The Inspector shows it as the checkbox every
   BOOL gets, under the field's own name; undo, save, prefabs and the cook carry it through the
   description with no code of their own.
2. **Defaults.** The light's default row has it false (0316): Add component, the untitled scene's
   light and any light built from a zeroed literal cast nothing. The shape's and the model's default
   rows have it true. Zero is false, so code that builds a shape or model row from a literal says
   `.cast_shadows = true` (the editor's untitled cube, tests); a light in code that should cast says so
   too (dev's sun, shadow tests).
3. **An old file keeps its sun's shadows.** `ecs` gains an optional second row per described type,
   the *unsaid row*: what a field a file does not mention stands for. `authoring`'s reader takes a
   missing field from it first, then the default row, then zero. The light registers one equal to its
   default row with `cast_shadows` true; the shape and model register none, so a missing key reads
   their default, true. A scene or prefab saved before 049 opens with every shadow it had.
4. **The light's switch is 3d's.** `voe_3d_draw_system_shadows` opens the cascades only when, beside
   today's conditions, the world has a light row (the one `voe_3d_draw_system_light` reads) with
   `cast_shadows` true. A world with no light row casts none, so the editor's preview light has no
   shadows (0316). The bounce pass needs the cascades, so a light that casts nothing bounces nothing
   until 051 rebuilds the bounce. `render` is unchanged.
5. **The caster's switch is 3d's too.** A mesh whose entity has a shape with `cast_shadows` false, and
   every part of a model row with it false, is left out of the cascades and the bounce map; it is still
   drawn, lit and shadowed in the view. A mesh with no shape (imported nodes, text, marks) casts by
   today's rules.

## Reasoning
A BOOL field is the existing checkbox path, and the light row is the one thing every picture of a
world shares, as 0319 found for the bounce. The default row is two things today, what Add component
makes and what an old file meant; 0316 makes them differ for the light alone, so the second meaning
gets its own row rather than a case for the light inside a generic reader.
- Default row true and the editor zeroing the flag when it adds a light: a per-type case in a generic
  menu, and a light added by any other tool would cast.
- Default row false and the examples rewritten: every scene a user saved loses its shadows, against 0316.
- An inverted field (`casts_no_shadow`): zero-init would cast, but the box would read backwards.
- A flag on render's light record: render would gate passes its caller chooses to open (0319).
- A separate shadow-caster component: two rows per thing for one bit, and a new thing would lack it.

## Replaces
Nothing. Amends 0289 and 0287 (the preview light casts no shadows), 0258 point 5 (casters) and 0190
(a described type may also have an unsaid row).
