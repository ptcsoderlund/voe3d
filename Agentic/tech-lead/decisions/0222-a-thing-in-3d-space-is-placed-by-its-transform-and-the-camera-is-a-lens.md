# 0222 — A thing in 3D space is placed by its transform; the camera component is a lens
date: 2026-09-23
by: tech-lead

## Decision
Anything that exists in 3D space has its position and orientation in its **transform component** and nowhere
else; anything that is not in 3D space has no transform. The scene camera (0218) is such a thing: its entity has
a transform, and the camera component holds **only the lens** (`fov_y`, `near_plane`, `far_plane`) and needs a
transform. The view is built from the entity's transform, **roll included**: a rolled camera films tilted, and
the Inspector shows its rotation as three angles like any other entity's. The engine strips nothing from the
camera's transform, scale included: it is the full transform like anything else's, so a camera parented under a
scaled entity (if hierarchy comes) behaves as that parent's children do, and how a project is structured is the
developer's call. The camera is moved only as anything
else is, by transform intents; the camera's own place and motion intents go. The dev program's flying and orbit
become transform intents it submits, and a first-person flight simulator (free yaw, pitch and roll) must be
buildable by a game's own code moving that transform. Editor view cameras are not entities and stay values;
`3d`'s pick, gizmo and outline take a viewpoint value that both the world's camera and a view's orbit turn
into.

## Reasoning
The sponsor's call (2026-09-23), answering 019's needs-decision: "just like anything else in 3D space; stuff
that ain't in 3D space don't use transform." One pose per entity and one way to move anything, as 0218 already
said. The sponsor also chose (2026-09-23) no camera-only exceptions such as ignoring scale: the engine should not
limit how developers structure their projects. The churn (`scene`, `3d`, `sprite`, `dev`, `editor`) is paid now while only the engine's own folders use
the camera. Rejected: the camera keeping its own eye/yaw/pitch with no transform, which makes it the one entity
not moved like the rest and has no roll; both poses with the camera's copied from the transform, which saves
and shows two poses and drops roll silently.

## Replaces
nothing (amends how 0218's "moved and aimed like any entity" is met)
