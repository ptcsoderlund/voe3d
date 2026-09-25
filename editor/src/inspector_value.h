// What a field's bytes say: a kind and an offset in, a number, an angle or a
// string out. Nothing in here draws and nothing in here writes — every function
// takes one field's kind and one field's bytes and answers a question about
// them, which is why the Inspector's two halves can share it.
//
//     const char *shown = value_text(arena, world, field, bytes);
//     double number = dragged(field->kind, bytes);
//
// IT IS inspector.c'S AND inspector_edit.c'S SHARED ARITHMETIC. The panel that
// draws a field and the edit that replaces one both have to read the same bytes
// the same way, so the reading lives here once rather than twice; adding a kind
// to base/describe.h is a build error in this file's exhaustive switches, which
// is the point of them.
//
// THE STRINGS IT FORMATS GO IN THE FRAME'S ARENA, for the reason inspector.h
// gives under AND SO DOES EVERY LABEL'S TEXT: a label is read at
// voe_ui_frame_end and never copied, so a number formatted onto a stack would be
// gone by the time it was drawn. `text` and `chars` push into the arena they are
// handed and the caller keeps it alive as long as the nodes are.
//
// THE THREE ANGLES ARE SHOWN AND NEVER STORED. `shown_angles` decomposes a
// quaternion into the three numbers a person drags for the length of one frame;
// nothing here keeps them, and what an edit submits is the difference turned
// back into a rotation about a world axis (inspector.h, ADR-0134 point 4).
// Keeping the angles instead would be two sources of one truth, which disagree
// the moment anything else writes the rotation.
#pragma once

#include <base/arena.h>
#include <base/describe.h>

#include <ecs/component.h>
#include <ecs/world.h>

#include <math/float3.h>
#include <math/quat.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// The words a boolean says, here rather than beside the drawing because both
// halves of the panel say them. Literals, because a label's text is read after
// the call that made it has returned and a literal is still there then.
#define TRUE_TEXT "true"
#define FALSE_TEXT "false"

const char *text(voe_base_arena *arena, const char *format, ...);
const char *chars(voe_base_arena *arena, size_t size, const uint8_t *bytes);
const char *heading(voe_base_arena *arena, const voe_ecs_world *world,
		    voe_ecs_type type);

float real32_at(const uint8_t *bytes, uint32_t lane);
int64_t whole_signed(voe_base_field_kind kind, const uint8_t *bytes);
uint64_t whole_unsigned(voe_base_field_kind kind, const uint8_t *bytes);
bool is_signed(voe_base_field_kind kind);
bool is_unsigned(voe_base_field_kind kind);
double dragged(voe_base_field_kind kind, const uint8_t *bytes);

voe_math_float3 shown_angles(voe_math_quat q);
float angle_of(voe_math_float3 angles, uint32_t axis);
voe_math_float3 world_axis(uint32_t axis);
const char *axis_name(uint32_t axis);

const char *value_text(voe_base_arena *arena, const voe_ecs_world *world,
		       const voe_base_field_description *field,
		       const uint8_t *bytes);

uint32_t lanes(voe_base_field_kind kind);
bool is_vector(voe_base_field_kind kind);
