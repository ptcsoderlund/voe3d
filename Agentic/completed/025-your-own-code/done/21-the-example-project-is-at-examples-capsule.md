# 21 — The example project is at `examples/capsule/`
folder: examples/capsule
decisions: 0168, 0244, 0246

## Change
Bug 02, the new half. `examples/` and `examples/capsule/` do not exist yet; this card makes them.
`game/example/` stays until card 22 removes it. Per 0246 the folder check builds nothing here:
it passes on the `.md` pages and file headers alone.

- `examples/capsule/` — a copy of every file `git ls-files game/example` lists, same relative
  paths (`project.voe3d`, `main.scene`, `.gitignore`, `Code/*.h`, `Code/*.c`, `Code/Code.md`),
  bytes unchanged except the page below. Not `game/example/Build/`: it is ignored local output.
- `examples/capsule/capsule.md` — `game/example/example.md` renamed for its folder: title
  `# capsule`, same points (what the project shows, what `project.voe3d` and `main.scene` hold,
  the `Code` entry). Nothing named `example.md` in the new folder.
- `examples/capsule/Code/Code.md` and the `Code/` headers — open each; only if one names
  `game/example` or `example.md`, it names `examples/capsule` / `capsule.md` instead.
- No `examples/examples.md`, no CMake file: `examples/` is data, not an engine folder (0244).

## Done when
1. `checks.sh --folder examples/capsule` prints `FINDINGS: 0`.
2. `diff <(git ls-files game/example | sed 's|^game/example/||; s|^example.md$|capsule.md|' | sort) <(git ls-files --others --cached --exclude-standard examples/capsule | sed 's|^examples/capsule/||' | sort)` prints nothing.
3. `grep -rn 'game/example' examples/` prints nothing.
4. `cmake --build --preset debug --target voe_editor`, then in `p=$(mktemp -d)` with
   `cp -r examples/capsule/. $p`:
   `build/debug/editor/voe_editor --capture $p/shot.png $p 2>$p/err` exits 0, `$p/err` is
   empty and `$p/Build/editor/loaded/project-1.so` exists.
