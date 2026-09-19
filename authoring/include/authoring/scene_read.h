// Scene text, read into a world (ADR-0149, ADR-0152) — the other half of
// authoring/scene_write.h, whose header says what the text looks like.
//
//     voe_authoring_kept kept;
//
//     if (!voe_authoring_scene_read(text, size, world, arena, &kept))
//             ...                     // reported; see what the world is left as
//     ...
//     voe_authoring_text out;
//     voe_authoring_scene_write(world, &kept, arena, &out);
//
// A LOAD CREATES, AND IT IS THE ONE SANCTIONED GENERIC WRITER. A component is
// written by its own system (ecs/component.h), and the exception is creation:
// a folder's typed creation call, and this reader. Every entity it touches is one
// it has just created, so it adds rows with voe_ecs_component_add straight from
// the field descriptions and asks no system. It may not do anything else: it sets
// no existing row, removes nothing, submits no intent, and touches no entity it
// did not create. A world that already holds an authored entity is the caller's
// bug and asserts — a scene is loaded into a world with nothing authored in it.
//
// THE FILE IS VALIDATED WHOLE BEFORE ANYTHING IS CREATED. Every section name,
// every value and every section's `[N]` is checked first, with the world untouched;
// a file that is wrong anywhere is refused, reported with its line number, and
// creates nothing.
//
// A WORLD THAT RUNS OUT OF ROOM PART-WAY THROUGH CREATING IS LEFT HALF-LOADED.
// Running out is the world's capacity, which no amount of reading the file could
// have predicted, and there is no remove to undo with. It returns false, and the
// world must be discarded — rewind or destroy the arena it lives in.
//
// A LOADED VALUE IS NOT SETTLED BY ANY DRAIN. The rows go in as the file spells
// them, and what a system corrects on the way through an intent — an identity's
// name with no terminating zero, say — is not applied here. The reader refuses
// what it can check on its own: a string that leaves no room for its zero, a
// control character, a float with no finite value.
//
// WHAT IT READS, AND WHAT IT KEEPS WITHOUT READING:
//
//   - `[N]`, where N is a decimal from 1 with no leading zero, is an entity with
//     authored id N. Its keys are the identity's fields other than `id`.
//   - `[N.<key name>]` is a component of that entity, and the file must also hold
//     `[N]`. A registered, described type is read field by field. A registered
//     runtime-only type is refused: the file claims to hold something that is
//     never authored. A name nothing registered is kept, not read — see below.
//   - Any other section name is refused.
//
// A VALUE IS READ AGAINST ITS FIELD'S KIND AND SHAPE (ADR-0154), spelled exactly
// as scene_write.h writes it, with blanks tolerated around brackets and commas.
// AN ARRAY NESTS AS THE FIELD'S DIMENSIONS SAY, outermost first, and every level
// must hold exactly its dimension's count of the next level's shape — a row too
// short, too long, flattened or mixed with another kind all refuse the file,
// naming the line. NESTING IS REFUSED PAST 8 BRACKET LEVELS, a vector kind's own
// counted the same as one the field's shape opens, walked with an explicit stack
// and not a recursive call (CLAUDE.md rule 14). An ENUM field is refused, because
// no value names exist to read one with. AN ENTITY is an authored id: `0` is no
// entity, and an id with no `[N]` in the file is no entity and a warning. A field
// the section does not mention starts from that field's bytes in the type's
// default row (voe_ecs_component_default), or zero when the type has none, and
// is a warning — so a scene saved before spec 010 opens its shapes grey, not
// black; a key naming no field is
// ignored and a warning. A CHAR VALUE'S quotes are the sectioned reader's to
// strip (assets/sectioned.h), so `name = Cube` reads the same as `name = "Cube"`
// — for a field of rank 1, one string. A field with more dimensions is an array
// of strings, and there the sectioned reader has left the text exactly as the
// file spelled it — an array is not one whole value to it — so this reader
// strips each string's own quotes and undoes its own `\"` and `\\` escapes.
//
// THE KEPT SECTIONS LIVE IN `arena` AND MUST OUTLIVE THE SAVE THAT WRITES THEM
// BACK. A section naming a type this program never registered is some other
// program's component; it is kept as its lines of text, handed to
// voe_authoring_scene_write, and written back where it was, so a file survives a
// trip through a program that does not know everything in it.
#pragma once

#include <base/arena.h>
#include <ecs/world.h>

#include <stddef.h>
#include <stdint.h>

// One section read and not understood.
typedef struct {
	// The N of `[N.<key name>]`.
	uint64_t id;
	// The key name, NUL-terminated.
	const char *key;
	// Its `key = value` lines, each ending in `\n`, in file order and as the
	// file spelled them less the blanks at either end; `size` bytes, not
	// NUL-terminated.
	const char *lines;
	size_t size;
} voe_authoring_kept_section;

// Every kept section, in file order. A file with none is a count of nought and
// NULL.
typedef struct {
	const voe_authoring_kept_section *sections;
	uint32_t count;
} voe_authoring_kept;

// Reads `size` bytes of scene text into `world`, which must hold no entity with an
// identity. True with every entity and row created and `*out_kept` set. False,
// reported, with `*out_kept` untouched: the world is untouched too when the text
// was refused, and half-loaded when the world ran out of room — see the header.
// The arena holds the kept sections and this call's working memory, beside each
// other; what is pushed is the caller's to rewind, as with every reader in assets.
[[nodiscard]] bool voe_authoring_scene_read(const char *text, size_t size,
					    voe_ecs_world *world,
					    voe_base_arena *arena,
					    voe_authoring_kept *out_kept);
