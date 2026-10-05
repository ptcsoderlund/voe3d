# 07 — The public headers' index lists place_marker.h
folder: 3d/include/3d
after: none
decisions: 0168

## Change
`3d/include/3d/3d.md` has no entry for `place_marker.h`. Read the header
comment of `3d/include/3d/place_marker.h` and add one entry for it, after
`point_light_marker.h`, in the shape of its neighbours (one sentence, under
300 characters): a meshless place drawn in an editor's view as a wire diamond
in line quads about the eye, the ray that picks it, and which entities wear
it. No other file changes.

## Done when
`grep -q '`place_marker.h`' 3d/include/3d/3d.md` exits 0, and
`checks.sh --folder 3d/include/3d` prints no finding for `3d.md`.
