// A world, written out as scene text (ADR-0149, ADR-0150, ADR-0151).
//
//     [1]
//     name = "Cube"
//     [1.voe_scene_transform]
//     position = [0, 0, 0]
//     rotation = [0, 0, 0, 1]
//     scale = [1, 1, 1]
//
//     [2]
//     name = "Sun"
//
// IT READS THE WORLD AND NEVER WRITES IT. The world is const, nothing is
// submitted, and nothing here opens a file: the text lands in the arena and
// where it goes next is the caller's.
//
// WHAT IS WRITTEN IS WHAT A PERSON AUTHORED, AND NOTHING ELSE:
//
//   - Only entities with a voe_scene_identity, ascending by authored id. Each is
//     `[N]` and then the identity's fields other than `id`, which is the N.
//   - Beneath each, every described component type it has, except the identity,
//     ascending by the byte order of its key name, as `[N.<key name>]`, and then
//     every field in declaration order as `key = value`. A runtime-only type is
//     not authored data and is skipped.
//   - Beneath each too, the kept sections of that authored id — the sections
//     authoring/scene_read.h read and did not understand — each in the place its
//     key name sorts among the component sections, its lines exactly as they were
//     read. A kept section whose id no entity in the world has any more is dropped,
//     and that is a warning.
//   - A blank line before every `[N]` but the first and nowhere else, `\n` line
//     endings and a final newline. An empty world is the empty text.
//
// THE SAME WORLD IS THE SAME BYTES, EVERY TIME. Nothing depends on the order rows
// sit in a table, which a removal changes (ecs/component.h), or on the order types
// were registered in, which a program's startup decides.
//
// EVERY REFUSAL OR WARNING NAMING A FIELD INSIDE AN ARRAY NAMES THE ELEMENT'S
// INDEX TOO, PER LEVEL IT SITS UNDER — `tags[1]`, `grid[1][0]` — built as the
// nesting is walked.
//
// THE NUMBERS ARE FORMATTED BY THE C LIBRARY IN THE "C" LOCALE. A program that
// calls setlocale for LC_NUMERIC would get commas; nothing in the engine does.
#pragma once

#include <authoring/scene_read.h>
#include <base/arena.h>
#include <ecs/world.h>

#include <stddef.h>

// The text a write hands back: `text` is NUL-terminated for convenience, and
// `size` its length without the NUL.
typedef struct voe_authoring_text {
	const char *text;
	size_t size;
} voe_authoring_text;

// Writes the world's authored entities, and the sections in `kept` — NULL for
// none — as scene text into `arena`. `kept` is read and not kept. On success
// `*out` holds the text. On failure it returns false, reports why, and leaves
// `*out` untouched. The arena also holds this call's working memory, beside
// the text; what is pushed — on success or failure — is the caller's to
// rewind, as with every reader in assets.
//
// A VALUE IS WRITTEN BY ITS FIELD'S KIND AND SHAPE (ADR-0154):
//
//   - integers in decimal, BOOL as `true` or `false` (any non-zero byte is true);
//   - FLOAT32 and FLOAT64 as the shortest decimal that reads back to the same
//     bits, so `0.1f` is `0.1` and `-0.0` is `-0`;
//   - FLOAT2, FLOAT3, COLOUR, FLOAT4, QUAT and FLOAT4X4 as `[a, b, c]`, in
//     memory order (COLOUR exactly as FLOAT3) —
//     a vector kind's own bracket, however the field around it nests;
//   - a field of rank 1 or more nests one `[…]` per dimension, outermost first,
//     `, ` between items: `F(voe_math_float3, path, FLOAT3, 4, 2)` writes four
//     `[[x, y, z], [x, y, z]]`. CHAR is the exception;
//   - CHAR as the bytes up to the first NUL, in quotes, with `"` and `\` escaped.
//     The innermost dimension is the string's own bytes and not a bracket level:
//     `CHAR, 32` is one string, written as today; `CHAR, 8, 32` is eight,
//     written `["a", "b", …]`. A bare value's quotes and escapes are the
//     sectioned reader's to undo (assets/sectioned.h); a string inside an array
//     is not the whole value, so this writer's own escaping is what a reader
//     must undo for one (authoring/scene_read.h);
//   - ENTITY as the target's authored id, and `0` for a dead one. A live target
//     with no identity is written `0` too, and that one is a warning naming both
//     entities, because a reference to something that will not be in the file is
//     a link the save is about to lose.
//
// IT REFUSES, RATHER THAN WRITING A FILE THAT CANNOT BE READ BACK AS WHAT WAS
// SAVED, ON:
//
//   - an ENUM field, because no value names exist yet to write it with;
//   - a NaN or an infinite float, which has no decimal to write;
//   - a CHAR value holding a byte below 0x20, which the format cannot carry;
//   - two entities with the same authored id, which a file cannot tell apart;
//   - a described type whose description this build compiled out, because
//     skipping it would save a scene with that component silently gone;
//   - a kept section whose key name a type has been registered under since it was
//     kept, because written beside that type it is a section the reader refuses.
//
// Every refusal is reported through base/report.h naming the entity, the
// component and the field.
[[nodiscard]] bool voe_authoring_scene_write(const voe_ecs_world *world,
					     const voe_authoring_kept *kept,
					     voe_base_arena *arena,
					     voe_authoring_text *out);
