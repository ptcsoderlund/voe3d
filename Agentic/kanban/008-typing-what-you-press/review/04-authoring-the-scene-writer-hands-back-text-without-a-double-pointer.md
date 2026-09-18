# 04 — authoring: the scene writer hands back text without a double pointer
folder: authoring
decisions: 0168

## Change

`voe_authoring_scene_write` breaks rule 6 (no `**`, ADR-0043). Today, in
`include/authoring/scene_write.h` line 95 and `src/scene_write.c` line 746:

```c
[[nodiscard]] bool voe_authoring_scene_write(const voe_ecs_world *world,
					     const voe_authoring_kept *kept,
					     voe_base_arena *arena,
					     const char **out_text,
					     size_t *out_size);
```

Replace the two out-parameters with one out-struct, declared in `include/authoring/scene_write.h` beside the
function:

```c
typedef struct voe_authoring_text {
	const char *text;
	size_t size;
} voe_authoring_text;

[[nodiscard]] bool voe_authoring_scene_write(const voe_ecs_world *world,
					     const voe_authoring_kept *kept,
					     voe_base_arena *arena,
					     voe_authoring_text *out);
```

`false` still means the one way this can fail and leaves `*out` untouched (rule 13); nothing else about the
function changes, and the text is still arena-allocated by the arena the caller passed.

Update, in this folder: `src/scene_write.c`, the nine call sites in `tests/scene_read.c` (lines 175, 358, 478,
537, 640) and `tests/scene_write.c` (lines 101, 112, 609, 634), the worked example in
`include/authoring/scene_read.h` line 9 and the prose naming the call at line 67, the header comments of
`scene_write.h` and `src/scene_write.c` where they describe what comes back, and the
`include/authoring/scene_write.h` entry in `authoring/authoring.md`.

One call site is downstream and this card owns it (ADR-0113): `editor/src/project.c` line 323, plus the comment
in `editor/src/project.h` line 68 that names the call. It is the only place outside `authoring` that calls this
function — `grep -rn voe_authoring_scene_write` outside `history/` confirms it. Expect
`checks.sh --folder authoring` to print one finding for `editor/src/project.c` saying it changed outside the
card's folder: that is this line, it is expected, and it is reported rather than worked around. Change nothing
else in `editor/`.

## Done when

`grep -nE '\*[[:space:]]*\*' authoring/include/authoring/scene_write.h authoring/src/scene_write.c` prints
nothing outside comments, and
`cmake --preset debug && cmake --build --preset debug --target voe_authoring voe_editor && ctest --test-dir build/debug -R '^authoring/'`
exits 0. If `slangc` is not found, it is installed at `~/voe3d-scratch/tools/slang/bin/` — put that on PATH for
the session; the tool is the programmer's (ADR-0021), so do not teach `check.cmake` or the build to look for it.
