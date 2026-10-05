# 26 — The draw system tests split by what they test
folder: 3d/tests
after: none
decisions: 0168

## Change
`3d/tests/draw_system.c` is 1004 lines; card 27 changes its marker case. Split it by function,
moving code unchanged; each new file is its own test executable (the folder's glob registers it).

- `3d/tests/draw_markers.c`, new: `draws_with_a_marker` and `a_marked_camera_is_one_more_draw`
  (lines ~826–962), its own `main` running that case, and copies of the helpers it calls
  (`a_world`, `add_the_sun`, `at_depth` and any other it uses). Header comment: what it tests (a
  marked camera or sun is one more draw than none) and why a draw count is the measure, taken
  from the part of `draw_system.c`'s header that says so.
- `3d/tests/draw_gizmo.c`, new: `a_gizmo_frame` through `a_gizmo_shows_through_what_it_stands_in`
  (lines ~537–763), its own `main`, copies of the helpers they call. Header comment: what the gizmo
  case proves and how it reads the picture, moved from `draw_system.c`'s header.
- `3d/tests/draw_system.c`: those cases, their now-unused helpers and their lines in `main` and
  in the header comment gone.
- `3d/tests/tests.md`: the `draw_system.c` entry keeps only what stays; an entry each for
  `draw_markers.c` and `draw_gizmo.c`.

## Done when
`[ $(wc -l < 3d/tests/draw_system.c) -lt 800 ]` exits 0, and the tests `3d/draw_system`,
`3d/draw_markers` and `3d/draw_gizmo` pass.
