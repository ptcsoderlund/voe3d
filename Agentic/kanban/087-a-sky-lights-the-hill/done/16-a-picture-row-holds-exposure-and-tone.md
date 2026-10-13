# 16 — A picture row holds exposure and tone
folder: scene
after: 15
decisions: 0168, 0406

## Change
0406 point 4: `voe_scene_picture`, built as the sky row is. Read `scene/include/scene/sky_component.h`,
`scene/include/scene/sky_system.h`, `scene/src/sky_component.c`, `scene/src/sky_system.c` and
`scene/tests/sky.c` as the pattern, and the `bounces` field of `scene/include/scene/light_component.h`
for a named UINT32.

- `scene/include/scene/picture_component.h` (new): `voe_scene_picture`, fields `exposure` FLOAT32 and
  `tone` UINT32 named by `voe_scene_picture_tone_names` ("Clip", "Soft");
  `VOE_SCENE_PICTURE_TONE_CLIP` 0, `VOE_SCENE_PICTURE_TONE_SOFT` 1,
  `VOE_SCENE_PICTURE_EXPOSURE_MAX` 16; its key; `_get`, `_count`, `_rows`, `_entities`. Header
  points: the whole picture's settings, not a camera's (the editor's views have none); no row is ×1
  and Clip, the old picture; exposure in stops then the curve (Clip as is, Soft Khronos PBR
  Neutral), overlays and the interface untouched; the first row in table order is used; bloom and
  grading would join it later.
- `scene/include/scene/picture_system.h` (new): `voe_scene_picture_register(world, capacity)` (default
  exposure 0, tone Soft, no needed type, menu "Rendering / Picture", the intent as its replace),
  `[[nodiscard]] bool voe_scene_picture_add(...)`, `voe_scene_picture_intent { entity; picture; }`,
  `[[nodiscard]] bool voe_scene_picture_submit(...)`, `void voe_scene_picture_system_run(world)`
  refusing a number not finite, an exposure past ±16 or a tone past Soft.
- `scene/src/picture_component.c`, `scene/src/picture_system.c` (new): the above, each with its header.
- `scene/tests/picture.c` (new): registration (menu path, no needed type, default row); add and read
  back; a replace changes the exposure after a run; a NaN exposure, 17 and a tone of 2 are refused
  and keep the row; add on a dead entity is false; the tone names read "Clip" and "Soft".
- `scene/scene.md`: entries for the two headers; `scene/src/src.md`, `scene/tests/tests.md`: one each
  for the new files. Each at most 300 characters.

## Done when
The tests `scene/picture` and `scene/sky` pass after the folder's build.
