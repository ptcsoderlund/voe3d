# 19 — Docs are written on open, Refresh and code reload
folder: editor
after: 18
decisions: 0168, 0509, 0511
read: feature.md

## Change
- `editor/src/session.h`: add `bool docs_due;` to the session, with a comment saying when it is set
  and who clears it.
- `editor/src/session.c` (767 lines; add only these few lines):
  - set `docs_due = true` wherever `refresh_due = true` is set, and after a code swap succeeds;
  - in `voe_editor_session_step`, when `docs_due` is set, clear it and call `voe_editor_docs_write`
    (`editor/src/docs.h`) with the step's scratch arena;
  - a failure goes to the notice the session already uses for a failed refresh.
- `editor/src/startup.c`: set `docs_due` beside `refresh_due` where an opened project sets it.

## Meaning
- Open: rewriting `Docs/` on every open is the tech-lead's assumption; this card builds on it.

## Done when
This bash script exits 0:
```
d=$(mktemp -d); p=$d/p; mkdir $p
printf '[project]\nscene = "main.scene"\n' > $p/project.voe3d
printf '[1]\nname = "Sun"\n[1.voe_scene_transform]\n[1.voe_scene_light]\n' > $p/main.scene
run() { XDG_CONFIG_HOME=$d build/debug/editor/voe_editor $p --capture $d/a.png; }
run && test -f $p/Docs/voe3d.md && test -f $p/Docs/voe3d-files.md || exit 1
L=$(sed -n '/^## voe_scene_light$/,/^## /p' $p/Docs/voe3d-scene.md)
echo "$L" | grep -q intensity && ! echo "$L" | grep -q texture || exit 1
grep -q -- '--check' $p/Docs/voe3d.md && grep -q '/include/' $p/Docs/voe3d.md || exit 1
echo mine > $p/Docs/notes.md; echo stale > $p/Docs/voe3d.md
run && grep -qx mine $p/Docs/notes.md && ! grep -qx stale $p/Docs/voe3d.md || exit 1
for f in CLAUDE.md AGENTS.md; do test ! -e $p/$f && test ! -e $p/Docs/$f || exit 1; done
```
The human's steps, from `## How to test` in feature.md:
- step 2: compare the light strength, light colour and fog thickness sliders with `Docs/`;
- step 4: have an agent told "read Docs/" warm the sun, halve its strength and add a point light;
- step 5: ask the agent to give a light a texture;
- step 7: ask the agent where the engine headers are.
