# 04 — The camera scrolls forward with the tank and knows the screen's edges
folder: examples/tank_game/Code
after: none
decisions: 0168, 0272, 0334

## Change
0334 point 1. Agents never edit `main.scene` (0272). Read `tank_camera.h` and
`tank_camera_system.c` (a runtime-only row added to the scene's camera: the pattern to follow),
`tank_hull.h`, `project.c`, `Code.md`, and the headers of
`scene/include/scene/transform_system.h`, `scene/include/scene/transform_component.h`,
`scene/include/scene/camera_component.h` and `game/include/game/project.h`.

- `examples/tank_game/Code/tank_scroll.h`, new: `typedef struct { double lead; double bottom;
  double top; } tank_scroll;`, its key, `tank_scroll_register(world)` (runtime-only, capacity 1,
  no menu), `const tank_scroll *tank_scroll_get(const voe_ecs_world *world)` (NULL before it is
  made) and `void tank_scroll_run(const voe_game_project_step *step)`. Header: the level runs
  along −Z; the row's meaning; one writer; the camera is the scene's one camera and a root; runs
  after the move (0256) so it follows where the hull went this step.
- `examples/tank_game/Code/tank_scroll_system.c`, new: the first step with a camera and a
  hull queues the row onto the camera, `lead` the camera's z less the first hull's. After: when
  the hull's z plus `lead` is below the camera's z, one transform intent moves the camera there,
  x, y and rotation kept. Then `bottom` and `top`: from the camera's pose and its row's current
  `fov_y`, the view's bottom and top centre rays met with the plane y = the hull's y; a ray
  that never falls to it gives the camera's z less `far_plane`. The row written whole. Headless
  too: nothing here reads the window.
- `examples/tank_game/Code/project.c`: registers the scroll; `systems_after_move` runs it;
  the header's slot order says so.
- `examples/tank_game/Code/Code.md`: entries for both new files; the project entry.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `grep -q tank_scroll_run examples/tank_game/Code/project.c` exits 0.
