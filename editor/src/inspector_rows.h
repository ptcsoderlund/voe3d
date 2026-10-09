// One described field's row on the Inspector: its name, then whatever control
// its kind gets, each control recorded on the inspector for the read after the
// frame (inspector_edit.h). inspector.c's walk calls it once per field.
//
//     voe_editor_inspector_field_row(ui, inspector, world, type, editable,
//                                    description, &description->fields[i], row);
//
// A CHAR field of rank 1 is one text field; of rank 2 it is one text field per
// string, "Materials 1" to "Materials 8", each writing only its own string. A
// field shown read-only is the same rows as labels.
#pragma once

#include "inspector.h"
#include "themes.h"

#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <ui/layout.h>

#include <stdbool.h>
#include <stdint.h>

// Between the things on one row of a component's panel. Millimetres.
#define ROW_GAP (2.0f * VOE_EDITOR_SPACING)

// Draws `field` of `row`, a row of `type`, and records its controls. Not
// `editable`, read-only, a kind with no control, or no room left in the
// inspector's controls, and it is labels. `description` is the field's struct,
// for the names a field's values may have.
void voe_editor_inspector_field_row(voe_ui_context *ui,
				    voe_editor_inspector *inspector,
				    const voe_ecs_world *world,
				    voe_ecs_type type, bool editable,
				    const voe_base_struct_description *description,
				    const voe_base_field_description *field,
				    const uint8_t *row);
