# 04 — The line of scene texts a step is taken from
folder: editor
decisions: 0168, 0190, 0204

## Change
The history itself, in one new file. Nothing calls it yet; card 05 wires it into the loop. It needs
`project.h` (card 03) and `scene.h`, and nothing else in this folder.

`editor/src/undo.h` and `undo.c` — new:

```c
#define VOE_EDITOR_UNDO_STEPS 64
#define VOE_EDITOR_UNDO_TEXT (32u * 1024u)

typedef struct {
	size_t size;
	char text[VOE_EDITOR_UNDO_TEXT];
} voe_editor_undo_state;

typedef struct {
	voe_editor_undo_state *states;  // VOE_EDITOR_UNDO_STEPS, pushed once
	uint32_t count;                 // states in the line, 0 for none yet
	uint32_t at;                    // which one the world is
	bool edited;                    // an edit has reached the project
} voe_editor_undo;

void voe_editor_undo_create(voe_editor_undo *undo, voe_base_arena *arena);
void voe_editor_undo_edited(voe_editor_undo *undo);
void voe_editor_undo_forget(voe_editor_undo *undo);
void voe_editor_undo_settle(voe_editor_undo *undo, voe_editor_project *project,
			    voe_base_arena *scratch, bool at_rest);
bool voe_editor_undo_take(voe_editor_undo *undo, voe_editor_project *project,
			  voe_editor_scene *scene, voe_editor_notice *why,
			  bool forward);
```

- `_create` pushes the states once and leaves `count` at nought. `_edited` sets the flag, `_forget` clears
  the flag and the line — a different project is being worked on.
- `_settle` does nothing unless `count == 0` or (`edited` and `at_rest`). Otherwise it marks `scratch`,
  calls `voe_editor_project_scene_text`, and: with `count == 0` that text becomes the one state; else, when
  it differs from `states[at]`, it becomes the state after `at` — `at + 1`, `count = at + 2`, which is what
  throws away everything that could have been redone — and when `at` is the last state the line shifts down
  one and the oldest is dropped. A text longer than a state empties the line. It rewinds `scratch` and clears
  `edited` whichever way it went; a write the authoring layer refuses leaves the line as it was.
- `_take` is undo, or redo when `forward`. False and nothing done when there is no state that way (`at == 0`,
  or `at + 1 == count`). Otherwise: note the selected entity's `voe_scene_identity` id, move `at`, hand that
  state to `voe_editor_project_scene_set`, and select the entity whose identity row carries the noted id
  through `voe_editor_scene_select`, clearing the selection when no row has it. A `scene_set` that refused
  empties the line and answers false with `why` filled.

The header's points: what a step is and what bounds it (0204), and the cost of that rule — Tab from one field
to the next never lets the keyboard go, so a row of numbers filled that way undoes as one step; that `_take`
belongs at the top of a frame, before the structural queue is applied and the systems run, so rows put back
are given their meshes before they are drawn; that the line is 64 states of 32 KB in the arena it was created
with, a scene of 32 entities being a fraction of one state; that selecting is not an edit and that the
selection is re-found by authored id because no entity handle survives a step; that an undo does not clear
the project's unsaved flag, and marking it edited afterwards is the caller's.

`editor/src/src.md` gains `undo.h` and `undo.c`.

## Done when
`checks.sh --folder editor` exits 0 — the file compiles into the program, unused — and `voe_editor
--capture <scratch>/f.png` still writes a picture. Card 05 is what proves the behaviour.
