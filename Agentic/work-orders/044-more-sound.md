# 044 — More sound

## What
Sounds can loop, sit on a thing, and have a volume of their own. A sound placed on a thing plays from
where the thing is: louder when near the camera, and to the left or right speaker by where it is on
screen. It follows the thing as it moves. A looping sound plays until it is stopped or its thing is
removed. Game code can start, stop and change the volume and pitch of a sound while it plays. In the
tank game, the engine hums, higher when driving. Every shot, hit and explosion has its sound, heard
from where it happens.

## Why
Milestone 11 of 0268. One sound at a time was enough for coins, and a battle needs more.

## How to test
Use headphones.
1. Play the tank game. The engine hums without a gap in its loop. Driving raises its pitch, and
   stopping lowers it.
2. Shoot an enemy on the left of the screen. The explosion is in the left ear. Do the same on the
   right.
3. A far explosion is quieter than a near one.
4. Destroy an enemy whose engine hums. Its hum stops with it.
5. Stop the game while sounds play. Everything goes silent at once.
