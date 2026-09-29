# 0284 — A field's names are bound when its description is asked for
date: 2026-09-29
by: planner

## Decision
For 038 bug 01: the names table that `VOE_BASE_DESCRIBE_STRUCT_NAMED` emits in a struct's
`_description()` accessor holds each names array's address only at run time. The table is a
`static`, non-`const` array whose rows carry the field and the count as constants; the
accessor stores each row's `values` pointer on every call before it returns. No row is ever
initialised from a names array's address, so a names array may be `VOE_BASE_IMPORTED` engine
data (0245) read by `project.dll`. The accessor is called from one thread at a time, as
registration and the editor's panels already are. `voe_base_field_names` and every caller are
unchanged.

## Reasoning
An imported address comes from the import table at load time and is not a constant, so a
static initialiser naming `voe_3d_shape_kind_names` or `voe_physics_collider_kind_names`
does not compile under `VOE_BASE_IMPORTING`; 038 made every project file include those
headers through `game/project.h`. Fixing it in `base` fixes every named struct at once.
Rejected: a static copy of each names array in its header (a build without descriptions
would carry it, against describe.h's own promise); a getter function in the row (changes the
public struct and every reader); `call_once` (not reliably in the Windows C runtime).
One path on every platform, so the Linux test proves the Windows expansion.

## Replaces
None.
