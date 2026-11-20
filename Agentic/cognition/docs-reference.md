# The Docs reference

What the editor writes into a game project's `Docs/` for agents: when it is written, what a row says, and what it leaves out.

## When it is written
- The editor rewrites `Docs/` on each open, so the reference is true to the editor that last opened the project. [assumed · tech-lead · 2026-11-06] see 0506, 0511
- `Docs/` is rewritten on open, first Save and a Refresh that builds; a Refresh in a project with no `.c` in `Code/`, or whose build fails, leaves it as it was. [observed · coder 116-19 · 2026-11-20] see 0511
- A Refresh that builds rewrites `Docs/` even when the library's bytes are unchanged. [assumed · coder 116-19 · 2026-11-20] see 0511
- `Docs/` is written under `--capture` too, as capture opens the project like a session. [assumed · planner 116 · 2026-11-20]
- If the engine source folder cannot be listed, nothing in `Docs/` is written and the failure is a notice; Play already depends on that folder. [assumed · coder 116-18 · 2026-11-20]
- An example project's `Docs/` is kept out of git because it holds machine-local paths. [assumed · planner 116 · 2026-11-20] see 0511
- A `.scene` a user places in `Docs/` is still watched and checked: the project's walk does not skip `Docs/`. [assumed · planner 116 · 2026-11-20]

## What a row says
- Ranges are in the file's units, as stored (a foliage size in centimetres), not the Inspector's scaled figures. [assumed · planner 116 · 2026-11-20]
- An empty default reads as "required"; `layer<N>`, `kind<K>` and `grow<L>_<K>` have one though none is required (absent ends the list or grows nothing, no spelled value means absent, `grow=0` is refused), so their description says otherwise. [assumed · coder 116-12 · 2026-11-20]
- An enum field's "missing reads as" is "—": the writer never spells an enum, so the reference does not ask an agent to. [assumed · coder 116-14 · 2026-11-20]
- A numbered row such as `share<N>` carries one default, share1's 50, and its description lists 50, 25, 12, 6, 3; the old `strength` and `swap` word keys get no row. [assumed · coder 116-11 · 2026-11-20] see 0435
- A landscape's least `cells`, 4, is the reader's own figure: no owner constant names it. [assumed · coder 116-12 · 2026-11-20] see 0511
- A paint row's weight has the range 0–255, refused; heights and copies, many numbers of mixed meaning, have none. [assumed · coder 116-12 · 2026-11-20]
- A material's Strength 0–1000 and Repeat 0.01–1000 are the Inspector's clamps, which 116 makes owner constants; the strict read refuses Repeat at neither end and Strength only below 0. [observed · planner 116 · 2026-11-20]
- A light's intensity has no range, so the Inspector shows it as a plain number field and `Docs/` gives it none. [observed · planner 116 · 2026-11-20]

## What it covers
- A landscape's vary, turn, tilt, shadow, blend and blendheight are per foliage kind, so their rows are `vary<K>` and so on. [observed · coder 116-12 · 2026-11-20]
- The scene identity type has its own heading like every registered type, saying its keys go under `[N]` and that `[N.voe_scene_identity]` is refused; it has no `id` row. [assumed · coder 116-14 · 2026-11-20]
- The check command in `Docs/voe3d.md` puts both absolute paths in double quotes, so it runs when copied from a path with spaces such as Windows' Program Files. [assumed · coder 116-18 · 2026-11-20]
