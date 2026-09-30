# 051 — Open starts beside the current project

## What
When I press Open in the editor, the file browser starts in the folder that
holds the project I have open, with that project's folder already highlighted.
Pressing Open in the browser straight away reopens it, and the other projects
beside it are one click away.

If no project is open (an untitled scene), the browser starts beside the last
project I opened, with that project highlighted. If there is no last project,
or its folder no longer exists, it starts in my home folder as before.

Save (the first save of an untitled scene) and New browse for a folder too;
they start in the same place.

## Why
Browsing from my home folder every time is slow when my projects all live in
one place, and the editor already knows where that is.

## How to test
1. Open the editor on `examples/tank_game`. Press Open. The browser shows the
   `examples` folder, with `tank_game` highlighted.
2. Press Open in the browser without clicking anything else. `tank_game` opens
   again.
3. Press Open, pick `examples/coin_game` and open it. Press Open again. The
   browser shows `examples`, now with `coin_game` highlighted.
4. Press New. The browser that asks where to put the new project starts in
   `examples`, with `coin_game` highlighted.
5. Close the editor and start it again. It opens `coin_game`, as before. Press
   Open: `examples`, with `coin_game` highlighted.
6. Rename the `coin_game` folder outside the editor while the editor is closed,
   then start the editor. Press Open. The browser starts in your home folder
   and nothing breaks.
