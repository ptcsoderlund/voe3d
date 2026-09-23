# Needs decision — where the scene camera's position and aim live

## Question
The camera component today holds its own pose: `eye`, `yaw`, `pitch` beside the lens (`fov_y`, `near_plane`,
`far_plane`), moved only by `scene`'s placement and motion intents (`scene/camera_system.h`). 0218 says the one
camera "is moved and aimed like any entity", and 019 moves it with the move gizmo (a transform intent) and aims
it by typing a rotation in the Inspector (the transform's rotation, shown as three angles). An entity with both
a transform and today's camera has two poses that can disagree, both saved, both shown in the Inspector. Which
one is the camera's pose is engine-wide: it is how Play, the dev program and every game's own code ("the
game's own code moving that one camera", 0218) move the camera, and it changes a public component read by
`3d`, `sprite`, `dev` and `editor`.

## Options
1. **The transform is the pose; the camera component is the lens.** The component keeps `fov_y`, `near_plane`
   and `far_plane`, needs a transform (`ecs` needed type), and the view is built from the entity's transform.
   A camera is moved like anything else, by a transform intent; `3d`'s draw frame reads both rows. The place
   and motion intents go: the dev program's flying and orbit become transform intents it submits. The editor's
   own view cameras stay values that are never entities (view.h), so `3d`'s pick, gizmo and outline calls take
   a viewpoint value (eye, yaw, pitch, lens) that both the world's camera and a view's orbit can be turned into.
   Cost: a card per using folder (`scene`, `3d`, `sprite`, `dev`, `editor`) before 019's own cards.
2. **The camera component stays the pose; the camera entity has no transform.** The Inspector edits `eye`,
   `yaw`, `pitch` and `fov_y` through a new camera replace intent; the move gizmo and the pick learn to stand
   on and hit a camera's eye. Cheapest for `dev` and `3d`'s existing calls, but the camera is then the one
   entity not moved "like any entity", the gizmo grows a second kind of target, and the "transform cannot be
   removed" invariant of 0217/0221 gains an exception. A rotation has no roll, so "typing its rotation" means
   typing two angles.
3. **Both, with the camera's pose derived.** The transform is the truth for an entity that has one, and the
   camera system copies it into `eye`/`yaw`/`pitch` (marked read-only) each run; an entity without a
   transform (the dev program's) keeps using place and motion. Least churn, but two poses are saved and shown,
   roll in the transform is silently dropped from the view, and a placement on a transformed camera is
   overwritten the next run.

## Recommendation
Option 1. It is what 0218's "moved and aimed like any entity" says, it leaves one pose per entity and one way to
move anything, and it is the shape Play and a game's follow camera want. The churn is paid once, now, while
the only users are the engine's own folders.
