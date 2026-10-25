# 102 — Fog drifts between the trees, and the sun falls in beams

## What
Decisions 0479 and 0480. "I want godrays in 0.3 yes." "Haze is also consistent and regular fog. I would like to
see fog moving irregularly between trees as well." Haze stays as it is, everywhere. Fog is only where I put it:
I add a fog volume to the level like any entity, and move and size its box with the gizmos. Inside it the fog
is thick in patches and thin between them, and it drifts and changes shape with a wind. Towards the box's edges
it fades out softly, so no box shape shows. In the Inspector I set its thickness, how patchy it is, and the wind.
Outside every fog volume there is no fog, only haze.

The sun lights the fog, and wherever clouds, trees or buildings block the sun the fog is in shadow, so the light
falls through in beams: between the trunks, through gaps in the clouds, past the tower. Each fog volume has a
Godrays strength for how strongly the beams show in it. Beams show whether or not the sun is on screen. A large,
thin, even volume gives beams without visible fog banks. The fog looks the same in the editor's view, in Play
and in a shipped build, and the hill stays smooth on my laptop.

## Why
Moving fog and sunbeams make the forest feel alive, and the sponsor wants them before the hill ships (0479).

## How to test
1. Add a fog volume to the hill and size it over part of the forest. Fog lies among the trees in patches.
   Outside the box there is none, and no straight edge or box shape shows where it ends.
2. Move it and resize it with the gizmos: the fog goes with it. Make it thicker and thinner, and more and less
   patchy.
3. Set a wind: the patches drift between the trunks and change shape as they go, not as one sheet sliding.
4. Stand in the foggy forest with a low sun: beams of light fall between the trunks. Turn the camera away from
   the sun: the beams still show.
5. Add a second, large, thin and even volume over the valley. With the Clouds from 101 drifting, look across
   it: beams fall through the gaps in the clouds and move as the clouds move, with no fog banks.
6. Raise and lower Godrays strength on one volume: its beams get stronger and fade away, while the fog stays.
7. Turn off both volumes: only the haze is left, as it was before.
8. Fly through the forest in fog: it stays smooth, and the frame breakdown shows what the fog costs.
9. Save, close and reopen: the fog is as I left it. Press Play, and ship: it looks as it does in the editor's
   view.
