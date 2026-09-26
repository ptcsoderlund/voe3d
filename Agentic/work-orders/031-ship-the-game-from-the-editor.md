# 031 — Ship the game from the editor

## What
A **Ship** button in the editor's top bar that turns the open project into a finished game I can
hand to someone else.

- Ship makes a **release** build of the project's game: optimised, no debug build, no editor and
  no editor-only code in it.
- The result is **one folder** holding everything the game needs to run: the program and any file
  it reads. I can zip that folder, send it to a friend, and it runs on their Linux machine with no
  engine source, no Clang, no CMake and nothing else from my machine. A Vulkan driver is all it
  needs.
- The game in that folder is the scene as it was when I pressed Ship, with the same code, and it
  plays exactly like Play does.
- While it builds, the button shows that it is working; when it is done, the editor tells me
  where the folder is. If the build fails, the Errors panel shows why, like a failed Play does.
- Pressing Ship again replaces the old shipped folder with a new one.
- **Ship builds for the system the editor runs on,** nothing else: the editor on Linux ships a
  Linux game, the editor on Windows a Windows game. No cross-compiling. Anyone who wants a
  Windows game builds the engine and editor from source on Windows and presses Ship there. I
  only test it on Linux.
- Where the folder goes and what the program is called are the planner's call.

## Why
The coin game is not a game until someone else can play it. This is the last step of 0186's road
and what v0.1 ships with.

## How to test
1. Open `examples/coin_game/` in the editor and press Ship. The button shows it is building, then
   the editor says where the shipped folder is.
2. Copy that folder somewhere far away from the engine, for example `/tmp/coin_game/`, and
   temporarily rename the engine's source folder so nothing can reach back into it.
3. Run the game from the copied folder: the menu comes up, and I can play to You win and to Game
   over exactly as with Play.
4. The program in the folder is a release build: noticeably smaller than `Build/debug/game`, and
   `file` on it does not say "with debug_info".
5. Move a coin in the editor and press Ship again: the new folder has the coin where I moved it.
6. Break the project's code on purpose and press Ship: the Errors panel shows the compiler error,
   and the shipped folder from before is left as it was. Fix it and Ship works again.
7. Play still works as before, and pressing Ship while the game from Play is running does not
   stop it.
