# 0223 — The scene camera is seen through a render view, drawn as a marker and shown in a corner
date: 2026-09-23
by: planner

## Decision
For feature 019, how 0218 and 0222 are built.

- **The viewpoint value is `render`'s `voe_render_view`** (view, projection, eye). The outline and the gizmo
  already take one; `voe_3d_pick_ray` takes one instead of a camera, and `voe_3d_view(pose, lens, aspect, out)`
  in `3d/projection.h` builds one from a transform and a lens. The world's camera and an editor view's orbit
  both turn into it; no new type.
- **The view matrix is the inverse of the transform's matrix**, scale included (0222). A pose with no inverse (a
  scale of nought on an axis) sees nothing: `voe_scene_camera_view` returns false, `voe_3d_view` returns false,
  and a frame whose camera sees nothing has `blind` set and draws no world. Zero means seen, so a frame a caller
  builds by hand keeps drawing.
- **The lens is edited through one whole-lens intent**, registered as the camera's replace. The drain keeps the
  last valid row, with one line on stderr, when a number is not finite, `fov_y` is not inside (0, π),
  `near_plane` is not above 0 or `far_plane` is not above `near_plane`.
- **The dev program keeps its own flight**: position, yaw and pitch, the speeds and the pitch limit moved from
  `scene` unchanged, turned into a transform (yaw about +Y, then pitch about the turned X, no roll) it submits.
  The exhibits that face the camera are handed this frame's pose, so nothing lags a frame.
- **A camera is drawn in a scene view as a marker**: every edge of a box (half extents 0.1, 0.075, 0.15 m,
  centred on the camera) and of its frustum from the camera's origin out to 1 m, at the lens's `fov_y` and an
  aspect of 16:9, all as lines of a fixed pixel width in the camera's own space under its whole matrix. It is in
  the world layer, unlit, in one caller's colour: the outline's when selected, the gizmo's rest colour otherwise.
  A ray picks the box, not the lines; `voe_3d_pick` walks cameras beside shapes. Only the editor's views ask
  for it; a game's frame never does.
- **The corner picture is one 16:9 target of 480 × 270 pixels**, drawn once a frame with the world's camera
  while the selected entity has a camera, with no marker, outline or gizmo in it, and shown anchored to the
  bottom-right corner of every drawn scene view at 30 % of that view's width.
- **The camera a scene is given** is an entity named "Camera" with the lens's default row, at (0, 2, 6), yaw 0,
  pitch −atan(2/6), no roll: back from the centre and looking at it. The editor's world has room for one camera;
  a file holding two is refused as a world that ran out of room.
- **The camera cannot be deleted or duplicated** because `scene.c`'s Delete and Duplicate do nothing for an
  entity with a camera and the Inspector draws no Duplicate and Delete row for it; the camera type joins the
  Inspector's kept list (0221), so its section has no Remove.

## Reasoning
`voe_render_view` is already the value the outline and the gizmo are built against, so making the pick take it
too is one type fewer than a viewpoint of `3d`'s own. A blind camera is 0222's "strips nothing" taken at its
word: a camera scaled to nothing sees nothing, and asserting would let a typed scale stop the editor. The
marker's aspect and the preview's are one number because the game window's aspect is not known to the editor;
16:9 is the common screen. Picking the box only keeps a click beside the camera from selecting it through its
thin lines. Refusing in `scene.c`, which both the Inspector's buttons and the shortcuts call, fixes it once.

## Replaces
nothing
