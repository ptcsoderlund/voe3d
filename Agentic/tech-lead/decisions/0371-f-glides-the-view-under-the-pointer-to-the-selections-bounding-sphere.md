# 0371 — F glides the view under the pointer to the selection's bounding sphere
date: 2026-10-06
by: planner

## Decision
For 065:
1. **The size is `3d`'s.** `3d/bounds.h` answers an entity's size in the world: the axis-aligned box
   of every shape's and loaded model's triangles on the entity and everything under it
   (`voe_scene_parent_within`), each through its world transform, given as the box's centre in
   double and its half diagonal as the radius. Water, particles, markers and colliders have no size;
   a tree with no shape or loaded model has none.
2. **The distance.** The eye stands where that sphere spans two thirds of the narrower of the
   picture's height and width: half angle `h` the smaller of half `fov_y` and its horizontal
   counterpart from the aspect, distance `r / sin(atan(2/3 · tan h))`, never under the orbit's
   closest. Without a size the focus is the entity's world position and the distance a fixed 3 m.
   An entity with no transform and no size does nothing.
3. **The glide.** Only the view under the pointer moves: its focus and distance go from where they
   are to the target in 0.25 s, eased by smoothstep, the focus lerped in double; yaw and pitch stay,
   so the eye follows by the orbit. A middle drag or a fly starting on that view ends the glide
   where it is; a new project's focus on the camera ends every glide. Nothing is saved.
4. **The key.** `platform` gains `VOE_PLATFORM_KEY_F`. F without Control is a shortcut flag in
   `shortcuts.h`, silenced by the same guards as R (typing, flying, browser, picker, dropdown). No
   selection, a stale one, or a pointer over no view: nothing.

## Reasoning
A bounding sphere is what other engines frame by and is orientation-free, so the view can keep its
angle. Picking geometry and transforms are `3d`'s already (pick.h), so the size sits beside them
and the editor stays a caller. Smoothstep over a quarter second reads as a glide without a jump.

## Replaces
Nothing.
