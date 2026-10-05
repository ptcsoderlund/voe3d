# 02 — A click names water and bare places, and a marker beats a mesh
folder: 3d
after: 01
decisions: 0168, 0354, 0365

## Change
`3d/src/pick.c`, `voe_3d_pick` (signature unchanged):
- Keep two bests while walking: the nearest mesh hit (shapes, models, and
  now waters) and the nearest marker hit (camera boxes, sun cubes, point
  light cubes, and now place cubes). Return the marker hit if there is one,
  else the mesh hit; `distance` is the returned hit's (0365 point 4).
- Waters (`3d/include/3d/water_component.h`, only when its table is
  registered, by `has_store`): a row with a transform is hit where the ray,
  carried into the entity's own space as `geometry_hit` carries it, crosses
  local y = 0 within |x| ≤ width/2 and |z| ≤ length/2, either face; the
  determinant-of-nothing skip applies as for shapes. A static helper.
- Place markers: walk the transform table's owners
  (`scene/include/scene/transform_component.h`), and for each that
  `voe_3d_place_marker_wanted` (`3d/include/3d/place_marker.h`) test
  `voe_3d_place_marker_hit` at its world position.
- The file's top comment: now walks waters and place markers; markers are
  tested as one group that wins over meshes.

`3d/include/3d/pick.h`: header points change — it walks waters on their
plane and every place marker; the "all compete on distance" paragraph
becomes: markers among themselves by distance, any marker over any mesh,
whichever is nearer (0354), and why (a small mark was meant; a marker
inside a house stays reachable). The usage comment's list of kinds gains
water and place.

`3d/tests/pick.c`:
- `a_camera_competes_with_the_cube_on_distance` and
  `a_sun_competes_with_the_cube_on_distance` (rename both to say a marker
  wins): the camera or sun behind the cube is now the hit, at its own
  distance; in front it still wins.
- New: a water plane at the origin is hit from above at its distance and
  missed beyond its width; a bare transform entity behind a cube is the hit
  (place marker beats the mesh); one beside the ray is not.
- Any existing test whose world holds a transform with no shape (a parent,
  in `a_child_is_hit_at_its_world_place`) under the ray now meets its place
  marker: move that entity off the ray or give it what the test needs, so
  the claim tested stays the same.

`3d/tests/point_light_marker.c`: its pick check "behind one it loses to the
cube" becomes "behind one it still wins", at the lamp cube's distance.
`3d/tests/far.c` and `3d/tests/draw_markers.c` call `voe_3d_pick`: run them;
if a bare transform there now answers, adjust as above.

`3d/tests/tests.md`: the `pick.c` and `point_light_marker.c` entries say
what they now claim.

## Done when
`ctest --test-dir build/debug -R '^3d/(pick|point_light_marker|far|draw_markers)$'`
passes after a build.
