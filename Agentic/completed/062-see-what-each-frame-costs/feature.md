# 062 — See what each frame costs

## What
In the editor and in dev I can open a frame breakdown. It lists every GPU pass the frame ran, by name, with its
time in milliseconds, plus the frame's total. A pass that did not run that frame is not listed. A capture in
RenderDoc shows the same pass names, and names every image and buffer. The debug build runs Vulkan's Best
Practices checks, and a new warning fails the checks (0358).

## Why
Before we spend work on renderer speed we need to see where the time goes, and whether every costly look really is
cheap when on and free when off (0316).

## How to test
1. Open the tank game in the editor and open the frame breakdown. Each pass is listed with a time, and the
   numbers add up to roughly the total.
2. Turn shadows on for the sun. A shadow pass appears with its cost. Turn them off again and it is gone.
3. Turn bounce light on for a light. Its passes appear; turn it off and they are gone.
4. Open the breakdown in dev as well; it works the same way.
5. If you have RenderDoc: capture a frame of the editor. The passes and resources carry readable names, not
   numbers.
6. Start a debug build. The start log says Best Practices checks are on and how many allowed warnings remain.
   The checks pass.
