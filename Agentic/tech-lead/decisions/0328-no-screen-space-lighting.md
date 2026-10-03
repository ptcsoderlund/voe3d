# 0328 — No screen-space lighting
date: 2026-10-03
by: tech-lead

## Decision
Light in VOE3D belongs to the world, not to the picture. No lighting effect (bounce, ambient occlusion,
reflections, shadows or anything that stands in for them) may depend on what is on screen or where the camera
looks: turning the camera in place never changes how bright anything is. Screen-space techniques (SSAO, SSGI,
SSR and their kin) are not used. A world-space cache that follows the camera is allowed, as long as its
edges and refreshes cannot be seen from where the player stands.

## Reasoning
The sponsor does not like screen-space looks: light that comes and goes as you turn is the artefact they
noticed in 051 (bug 01). Alternatives: allow screen-space as an opt-in extra (rejected, the sponsor said no);
allow it only for ambient occlusion (rejected for the same reason).

## Replaces
nothing
