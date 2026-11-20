# 04 — Reference text from descriptions and key tables
folder: authoring
after: 01, 03
decisions: 0168, 0498, 0511

## Change
- `authoring/include/authoring/reference.h` is new. It declares:
  - `[[nodiscard]] bool voe_authoring_reference_scene(const voe_ecs_world *world,
    voe_base_arena *arena, voe_authoring_text *out);`
  - `[[nodiscard]] bool voe_authoring_reference_keys(const char *title,
    const voe_assets_sectioned_key *keys, uint32_t count, voe_base_arena *arena,
    voe_authoring_text *out);`

  The header comment makes these points:
  - the text is Markdown, for a reader (an agent) who writes the files by hand;
  - it is generated from the descriptions and tables, so it says what the strict read enforces;
  - what each function holds;
  - on failure it returns false, reports why and leaves `*out` untouched, as `scene_write.h` does.
- `authoring/src/reference.c` is new.
  - **Scene text.** It opens with the text form:
    - `[N]` is an entity, with the keys `authoring/scene_read.h`'s header names for an entity;
    - `[N.<type key>]` is a component;
    - a key that is not listed is refused, with its line;
    - a section of a type nothing registered passes unread.

    Then comes one `## <type key>` heading per registered type, taken from
    `voe_ecs_component_type_count`/`_type_at`, in key order, skipping `_runtime_only` types. Under
    each heading put the type's menu, needs and former names where it has them, then a table with one
    row per field:
    - the key;
    - a kind phrase from the field's kind, plus the allowed names from `voe_base_names_find`;
    - the range and unit from `voe_base_ranges_find`, or "—";
    - what a missing key reads as (the row `ecs/component.h`'s header says), spelled by
      `voe_authoring_value_put_field` (`authoring/src/value_write.h`);
    - read-only fields marked as the editor's.
  - **Key tables.** `## <title>`, then one table row per key: section, key, kind, default (or
    "required"), range and unit, and says. A line follows on what `refused` means: refused
    rather than clamped.
- `authoring/src/src.md`, `authoring/authoring.md`: add entries for the new files.
- `authoring/tests/reference.c` is new; add its line to `authoring/tests/tests.md`. Set up the world as
  `authoring/tests/scene_check.c` does, with identity, transform and `voe_scene_light_register`.
  - `reference_scene_light`: the text has `## voe_scene_light` and rows for `colour`, `intensity` and
    `disc_size`. The `disc_size` row shows 0, 10 and °. The text has no `texture`.
  - `reference_keys_material`: built from `voe_assets_material_keys`, the text names
    `emission_strength` with 1000, and `repeat`.

## Meaning
- Holds: only scene components have described tables; the other formats are plain structs with
  ranges in header constants (now in their key tables, cards 01–03).
- Holds: a section of an unregistered type passes the scene check.

## Done when
`ctest --test-dir build/debug -R '^authoring/reference$' --no-tests=error` exits 0.
