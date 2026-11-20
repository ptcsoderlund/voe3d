# 116 — A project tells agents how its files are written

## What
Every project the editor opens has a `Docs/` folder beside `Assets/`, written by the editor, that tells an
agent working in the project folder how the project's `[Section]` files are written: every section each file
can hold, every key a section takes, and each key's kind, range, unit and default, exactly as the Inspector
shows them. It also says where the engine's public headers are, for the project's code. There is a command an
agent can run in the project folder that checks every `[Section]` file and lists each mistake by file, line,
key and why, or says all is well; `Docs/` tells how to run it. The editor never writes, creates or names
`CLAUDE.md` or `AGENTS.md`: those are mine. (0505, 0506)

## Why
The format is simple but its keys are strict: a light takes no `texture=`. Written down, an agent can write
correct files and help with a game, not just its code.

## How to test
1. Open a project. `Docs/` is there beside `Assets/`. Reading it I find the light: its keys, their kinds,
   ranges, units and defaults, and no `texture`.
2. Pick three Inspector rows — a light's strength, its colour, a fog volume's thickness. `Docs/` gives the
   same range and unit as each slider.
3. Put a file of my own in `Docs/`, close and reopen the project. My file is still there; the editor's
   files are rewritten.
4. Start Claude Code in the project folder with no hint from me beyond "read Docs/". Ask it to make the sun
   warmer and half as strong, then to add a point light by the house. With the editor open, both show in the
   view (115), and neither is refused.
5. Ask it to give a light a texture. It says lights take none, or the check refuses the file.
6. Run the check command in the project folder: it says all is well. Put a wrong key in the scene file and
   run it again: it names the file, the line, the key and why.
7. Ask the agent where the engine's headers are. It finds the answer in `Docs/`.
8. No `CLAUDE.md` or `AGENTS.md` appears in the project from the editor.
