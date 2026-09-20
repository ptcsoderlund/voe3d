# 0198 — A described field's values can be named, and a named field is a dropdown
date: 2026-09-20
by: planner

## Decision
For spec 012, the mechanism 0195 calls "a general one-of-a-named-list field".

- **The names ride on the struct's description, beside its fields, and not in the field's kind.**
  `base/describe.h` gains `voe_base_field_names { const char *field; const char *const *values; uint32_t
  value_count; }`, `voe_base_struct_description` gains `names` and `names_count`, and a declaring folder opts in
  with `VOE_BASE_DESCRIBE_STRUCT_NAMED(struct, field_list, names_list)`. `VOE_BASE_DESCRIBE_STRUCT` keeps its two
  arguments and describes a struct with no names at all. A tool asks
  `voe_base_names_find(description, field->name)` and gets the entry or NULL.
- **Entry `i` is the name of value `i`, and an entry may be NULL.** A shape's kinds start at 1
  (`VOE_3D_SHAPE_CUBE 1u`), so its table is `{ NULL, "Cube", "Capsule", "Cylinder" }`; a tool offers the entries
  that have a name and shows a value with none as the number it is. The names array is `extern const char *const
  <struct>_<field>_names[N]` in the declaring header and defined in its `.c`, so a build without descriptions
  carries no unused copy of it and the row's `value_count` is `sizeof` of the declared bound.
- **The field's kind does not change.** A named field is a `UINT32` and is written and read in scene text as one,
  so no file changes and neither `authoring/src/scene_read.c` nor `scene_write.c` is touched. Names are a note to
  a tool, exactly as `read_only` is.
- **A tool shows a named field that is editable as a dropdown** of the named values, and writes the chosen index
  as the value through the component's replace intent, like every other Inspector edit. A named field that is
  read-only, or whose component has no replace intent, stays the label it is today.
- **The open list is the editor's, composed from what `ui` already has.** `voe_ui_choice_begin` is already "one
  of a list, marked by inversion", so the closed control is a button and the open list is a panel of choice rows
  placed by `editor/src/interface.c` as an anchored child over the dock, the way the colour picker is; which
  field it is open on lives in `editor/src/scene.h` beside `picking`, for the same reason. `ui` gains nothing.
- **An intent naming a kind this build does not know is corrected back to the entity's own kind** and reported as
  a corrected intent, through the machinery `3d/shape_system.h` already has. A kind read off a file is still
  drawn as nothing with a warning: a file may be newer than the build, a click may not.

## Reasoning
A third hook on the field list (`F_NAMED` beside `F` and `F_READ_ONLY`) would break every described struct in the
tree at once — every field list in every folder is spelled `(F, F_READ_ONLY)` and a three-argument invocation is
a hard error — for one component's sake. Reinterpreting the field line's variadic tail as names for one kind
would need a per-kind dispatch of the rank, dims and names macros, twenty definitions apiece. A table beside the
fields costs base two members and one small macro, touches no existing declaration, and answers the same
question for a light's type or an alpha mode tomorrow.

Names as a table keyed by the field's name, rather than a new field kind, keeps the scene text and both of
`authoring`'s halves out of it: the bytes on disk are the number they already were, and an old file opens.
Entry `i` naming value `i` means the tool never has to know that a kind's numbering starts at one, and a NULL
entry says "this number has no name" without a second list of numbers.

The list is the editor's and not `ui`'s because `ui` keeps nothing between frames but what is held and what is
focused (ui/widgets.h), and the colour picker already showed what that means: the caller holds what the popup is
open on and places the panel. A dropdown widget in `ui` would be a second answer to a question that folder has
already answered.

Rejected: a new `VOE_BASE_FIELD_NAMED` kind (teaches two more files nothing they need to know); the names in the
field's variadic tail (sixty macros); a shape-only dropdown in the Inspector (0195 forbids it); a `ui` dropdown
widget (a popup's state is not `ui`'s).

## Replaces
nothing. Carries out 0195 and amends 0191's read-only `kind` as 0195 already did.
