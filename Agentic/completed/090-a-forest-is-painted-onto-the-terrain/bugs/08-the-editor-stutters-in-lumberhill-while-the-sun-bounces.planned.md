# 05 — The editor stutters in lumberhill while the sun bounces

## Seen
I get frame drops every now and then, really bad in lumberhill. With light bounce set to 0 there is no
stuttering at all, so it is the bounce's work, not the forest's drawing: foliage is not drawn into the
bounce's captures (0442 point 5).

What the bounce does now and then rather than every frame, from the code (not measured): the nests at
16, 4 and 1 m recapture a slice of probes when the camera has moved more than 2 cells; a stale sphere
queues probes near a change; a frame opens up to VOE_RENDER_BOUNCE_CAPTURE_PASSES (4) capture passes of
16 probes and 6 faces each; a change of lights relights the whole grid. Which of these is the spike
is not known. The Frame panel shows only card time, four times a second, so one bad frame can slip
past it.

## Expected
No visible hitch while moving about or painting in lumberhill with the sun bouncing. First a
measurement that names the pass and the cause of the spike, then the fix of that cause.

## How to reproduce
Open lumberhill in the editor with the sun's bounces at 1 or more. Fly and walk about the hill and
paint some foliage: every now and then a frame stalls. Set bounces to 0: it stops.
