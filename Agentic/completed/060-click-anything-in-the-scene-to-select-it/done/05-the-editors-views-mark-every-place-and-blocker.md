# 05 — The editor's views mark every place and line every blocker
folder: editor
after: 02, 03, 04
decisions: 0168, 0354, 0365

## Change
Mends the build card 04 broke and turns the marks on.

`editor/src/view.h` / `view.c`:
- `voe_math_float3 voe_editor_view_faint_colour(const voe_ui_theme
  *palette)`: the outline colour times `VOE_EDITOR_VIEW_FAINT` 0.3, defined
  beside `VOE_EDITOR_VIEW_GIZMO_REST` with a `static_assert` that it is
  below the rest; header points: what it colours (a blocker's box when not
  selected), why a lightness step on the outline colour and not a new role
  (as the gizmo's rest colour), why fainter than a handle at rest (0365
  point 5).

`editor/src/view_passes.c`, in the `voe_3d_frame` the view's pass draws:
- `.places`: shown, `selected`, `shapes->outline`, the gizmo's rest colour,
  the outline colour, the outline's pixel width and the view's size — the
  `.point_lights` record's values.
- `.light_blocker` becomes `.light_blockers`: shown, `selected`,
  `shapes->outline`, `voe_editor_view_faint_colour(palette)`, the outline
  colour as `selected_colour`, the same pixels and size.

`editor/src/view_passes.h`, the capacities macro:
- `transient_vertices` per view gains `VOE_3D_PLACE_MARKER_VERTICES *
  VOE_GAME_WORLD_AUTHORED` and `VOE_3D_COLLIDER_MARKER_VERTICES *
  VOE_GAME_WORLD_LIGHT_BLOCKERS`; `transient_indices` the same with
  `_INDICES`. The existing `2 * VOE_3D_COLLIDER_MARKER_*` stays (collider
  and selected blocker).
- `transient_geometries` per view 10 → 13 and the objects' per-view `+ 10`
  → `+ 13` (places two ranges, blockers one more).
- Comment: places sized by the authored room because the editor's world
  holds only authored entities (0365).
- `#include <3d/place_marker.h>` where the other marker constants come
  from.

`editor/src/pick.h` and `pick.c` top comments: every placed thing is
picked, on its mesh or its marker, a marker before a mesh (0354); the code
is unchanged.

`editor/src/src.md`: the `view.h`, `view.c`, `view_passes.h`,
`view_passes.c` and `pick.h` entries say what changed, as phrases.

## Done when
`grep -q '\.places' editor/src/view_passes.c && grep -q
'\.light_blockers' editor/src/view_passes.c` exits 0, and the editor
builds with `checks.sh --folder`.

For the human, the feature's `## How to test`, all seven steps, in the
tank game:
1. Every blocker shows faint box lines and a diamond marker; lights,
   emitters, sounds show markers; nothing without a transform does.
2. A blocker's marker click selects it: Inspector, brighter box, the move
   gizmo moves the box.
3. A click inside a blocker's box on the house floor selects the floor.
4. A point light's, an emitter's, a sound's marker each select theirs.
5. Add entity with only a transform, moved somewhere empty: a diamond;
   click away, click it: selected again.
6. A light placed inside the house where it shows through: its marker
   selects the light, the house beside it the house.
7. Play: no markers or blocker lines in the game.
