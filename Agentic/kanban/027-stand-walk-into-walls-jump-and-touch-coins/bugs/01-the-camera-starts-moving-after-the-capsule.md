# 01 — The camera starts moving after the capsule

## Seen
"When I start moving the capsule, it starts moving before the camera does. So it looks
unsmooth. This usually happens in Unity when camera and movement are on the same update. When I
move the camera to LateUpdate it usually looks better. It just doesn't look good."

Everything else in 027 works.

## Expected
The camera following the capsule and the capsule move together, from the first frame of a move
to the last: the capsule holds still on screen relative to the camera while it walks, starts,
stops, jumps and lands, whatever the frame rate. The camera is drawn at the same moment in time
as everything else in the frame (0254's lag applies to it as to the capsule), and it is placed
after the capsule has moved, not before. No smoothing or easing of the camera is asked for: it
stays rigidly on the capsule, as it was meant to.

## How to reproduce
1. Open the editor, open `examples/capsule/`, press Play.
2. Stand still, then start walking. The capsule moves off first and the camera catches up a
   moment later; the same when stopping.
3. Watch while walking steadily and while jumping: the capsule should not shift against the
   camera.

## Decided
Decision 0256: the camera follow runs in the new after-the-move slot.
