// Component tables. One contiguous array per component type, plus the two
// directions between an entity and its row.
//
//     const struct voe_ecs_key my_key = { "my_component" };
//
//     voe_ecs_type type = voe_ecs_component_register(world, &my_key,
//                                                    sizeof(struct mine), 1024,
//                                                    &voe_ecs_runtime_only);
//     if (!voe_ecs_component_add(world, type, entity, &value))
//             ...                                  // the table is full
//
//     const struct mine *rows = voe_ecs_component_rows(world, type);
//     const voe_ecs_entity *owners = voe_ecs_component_entities(world, type);
//     for (uint32_t i = 0; i < voe_ecs_component_count(world, type); i++)
//             ...                                  // rows[i] belongs to owners[i]
//
// ITERATING IS A LINEAR WALK OVER ONE ARRAY AND THAT PROPERTY IS THE POINT. The
// rows are packed from 0 to count with no holes and no pointers in between, so a
// system that reads every one of them reads memory in order. _rows and
// _entities hand out the two arrays themselves rather than a per-row accessor,
// because an accessor per row is the pointer chasing this shape exists to avoid.
//
// ENTITY TO ROW IS A DIRECT INDEX, PAID FOR IN MEMORY. Each table keeps one
// uint32_t per entity slot the world can hold, so a lookup by entity is one
// load and a bounds check, and a table of a rare component costs the world's
// entity count in indices whether or not anything has one. That is the trade
// this engine took: tables are few and entity counts are bounded, so four bytes
// per slot per type is cheaper to reason about than a hash. A sparse set would
// swap that memory for a second indirection, and nothing has needed to yet.
//
// A COMPONENT IS READ BY ANYONE AND WRITTEN BY ONE. Everything here that reads
// is const, and everything that writes is meant to be called from inside the
// module that owns the type — which is why the typed wrapper a folder exposes
// (voe_scene_transform_get and friends) is the surface everyone else uses.
// Nothing in ecs can enforce that; the folder that owns the component is what
// enforces it, by exposing reads publicly and keeping the writes to itself.
//
// WHICH ROWS EXIST IS THE WORLD'S, AND WHAT IS IN THEM IS THE OWNER'S (0190).
// A row's values are written only by its own system, and anyone else changes
// them through its intent. Rows are added and removed through the world's
// structural queue (ecs/structure.h), which anyone may submit to and the
// program applies once a frame. The creation exceptions stand as they are: a
// folder's typed creation call (voe_scene_transform_add) and authoring's scene
// reader, which adds the rows of entities it has just created straight from
// their descriptions (ADR-0152). Both create and neither edits. That _add is
// public does not widen them: calling it on an entity somebody else made is
// still writing their data.
//
// A STALE ENTITY IS REFUSED RATHER THAN ANSWERED. Every function here checks the
// generation, so an id from a destroyed entity gets NULL or false and never the
// row of whatever now lives in that slot.
#pragma once

#include <base/describe.h>
#include <base/imported.h>
#include <ecs/intent.h>
#include <ecs/world.h>

#include <stddef.h>
#include <stdint.h>

// A registered component type. Passed by value; means nothing outside the world
// that handed it out.
typedef struct {
	uint32_t value;
} voe_ecs_type;

// A TYPE IS DESCRIBED OR RUNTIME-ONLY, AND A REGISTRATION SAYS WHICH. A described
// type carries the struct description its folder wrote (base/describe.h), and its
// fields are what a scene saves (ADR-0149, ADR-0150). A runtime-only type carries
// &voe_ecs_runtime_only instead: state that means nothing in a file — a GPU id, a
// frame's range of vertices — and is rebuilt rather than saved.
//
// NULL IS REFUSED, BY THE COMPILER WHERE IT CAN SEE IT AND BY AN ASSERT WHERE IT
// CANNOT. A NULL that meant "undescribed" would be the easiest thing in the tree
// to forget, and forgetting it would not fail anywhere: every save would quietly
// leave that component out. Saying runtime-only out loud costs one word at the
// one site that knows.
//
// A DESCRIBED TYPE IN A BUILD WITHOUT DESCRIPTIONS PASSES
// &voe_ecs_description_compiled_out — never NULL and never runtime-only. It is
// still authored data; this build merely has no table to show for it, and a
// writer that can tell the two apart can refuse loudly instead of saving nothing.
// Its descriptions-off branch is where the declaring folder writes it:
//
//     #if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
//             return voe_scene_transform_description();
//     #else
//             return &voe_ecs_description_compiled_out;
//     #endif
//
// The two markers a registration passes instead of a description. Compared by
// address; their contents are an empty description and nothing reads them.
// A project's library imports them on Windows (0245), so it takes their
// addresses at run time there, never in a static initialiser.
//
// runtime_only: the type is not authored data and is never saved.
// description_compiled_out: the type is described, in a build that compiled the
// descriptions out.
extern VOE_BASE_IMPORTED const voe_base_struct_description voe_ecs_runtime_only;
extern VOE_BASE_IMPORTED const voe_base_struct_description voe_ecs_description_compiled_out;

// Registering twice with the same key, registering more types than the world was
// made for, a size of zero or a capacity of zero are all the caller's bugs and
// assert. Nothing about a registration comes out of a file. `description` is the
// type's description or one of the two markers above, and never NULL — the
// paragraphs above them say why.
[[gnu::nonnull(5)]]
voe_ecs_type voe_ecs_component_register(
	voe_ecs_world *world, const struct voe_ecs_key *key, size_t size,
	uint32_t capacity, const voe_base_struct_description *description);

// THE WORLD LISTS ITS TYPES, AND WHAT AN ENTITY IS MADE OF IS THE CALLER'S LOOP.
// Types are enumerated in registration order, and an entity's makeup is found by
// asking every one of them for a row:
//
//     for (uint32_t i = 0; i < voe_ecs_component_type_count(world); i++) {
//             voe_ecs_type type = voe_ecs_component_type_at(world, i);
//             if (voe_ecs_component_get(world, type, entity) != NULL)
//                     ...          // voe_ecs_component_key and _description
//     }
//
// There is no per-entity set of types to keep in step: the direct index in the
// header is already the answer, one load per type.
//
// How many types have been registered. The types are 0 to count, in the order
// they were registered.
uint32_t voe_ecs_component_type_count(const voe_ecs_world *world);

// The type registered `index`-th. An index at or past the count asserts.
voe_ecs_type voe_ecs_component_type_at(const voe_ecs_world *world,
				       uint32_t index);

// The key the type was registered against — its name is what a caller shows.
const struct voe_ecs_key *voe_ecs_component_key(const voe_ecs_world *world,
						voe_ecs_type type);

// A DESCRIPTION IS STORED AND NEVER READ. The world hands the pointer back — it
// does not know what a field is — and hands back NULL for either marker, which are
// compared by address and have nothing in them. The pointer must outlive the
// world, which a description's static table always does.
//
// The description the type was registered with, or NULL when it was registered
// with either marker.
const voe_base_struct_description *
voe_ecs_component_description(const voe_ecs_world *world, voe_ecs_type type);

// True for a type registered with voe_ecs_runtime_only, and only for that: a
// described type is false whether or not this build compiled its description in.
[[nodiscard]] bool voe_ecs_component_runtime_only(const voe_ecs_world *world,
						  voe_ecs_type type);

// THE INTENT THAT REPLACES A ROW IS STORED AND NEVER READ, LIKE A DESCRIPTION.
// A folder may name the intent a whole row of its component is written through,
// and the world hands that back beside the description — it never reads a queued
// value and never applies one. That is still the owning system's, exactly as rule 4 says:
//
//     typedef struct {
//             voe_ecs_entity entity;              // at offset zero, always
//             voe_scene_transform transform;      // the whole row
//     } voe_scene_transform_intent;
//
//     voe_ecs_component_replace_set(world, type, intent_type,
//                                   offsetof(voe_scene_transform_intent,
//                                            transform));
//
// THE ENTITY IS AT OFFSET ZERO AND THE ROW IS WHEREVER THE DECLARING FOLDER SAYS.
// Offset zero is fixed so a caller that knows nothing about the component can
// still read out which entity an intent names; the row's offset is not, because a
// folder writes the struct it wants and hands back offsetof rather than shaping
// the struct to suit this file.
//
// A TYPE WITHOUT ONE IS SHOWN AND NOT EDITED. _replace reports `set` false, and a
// tool walking the types is expected to display such a component and offer no way
// to change it — which is the honest answer, because there is no intent to submit
// and calling voe_ecs_component_set from outside the owning module would be the
// rule this folder exists to hold.
//
// IT IS ITS OWN CALL AND NOT A PARAMETER TO _register. A folder registers its
// component before it registers the intent that writes it, so the intent type
// does not exist yet at the moment of registration; and a component that is never
// edited says nothing rather than passing a zero nobody can tell from a real one.
//
// Names the intent a whole row of this type is replaced through, and says where
// in that intent's value the row sits — `offsetof` of the row's field, given by
// the folder that declared both. Once per type: a second call is the caller's bug
// and asserts, as does a `row_offset` that would put the row over the entity at
// offset zero or past the end of the intent's value.
void voe_ecs_component_replace_set(voe_ecs_world *world, voe_ecs_type type,
				   voe_ecs_intent intent, size_t row_offset);

// What _replace hands back. `row_size` is the component's row and `value_size`
// the whole intent, both as they were registered, so a caller can zero a buffer
// of one and copy a row into the middle of it without naming either type. When
// `set` is false the type has no replace intent and every other field is zero.
typedef struct {
	bool set;
	voe_ecs_intent intent;
	size_t row_offset;
	size_t row_size;
	size_t value_size;
} voe_ecs_replace;

voe_ecs_replace voe_ecs_component_replace(const voe_ecs_world *world,
					  voe_ecs_type type);

// A DESCRIBED TYPE HAS A DEFAULT ROW, SET BY THE FOLDER THAT DECLARES IT, the way
// its replace intent is. "Add at default" is a structural add with those bytes;
// the world copies them once and hands them back, and never reads them.
//
// Copies `row` — the type's registered size — as the type's default, into
// memory the world pushes for it. Once per type: a second call asserts.
void voe_ecs_component_default_set(voe_ecs_world *world, voe_ecs_type type,
				   const void *row);

// The default row, or NULL when none was set. Valid as long as the world.
const void *voe_ecs_component_default(const voe_ecs_world *world,
				      voe_ecs_type type);

// A DESCRIBED TYPE MAY ALSO HAVE AN UNSAID ROW (0324): what a field a file does
// not mention stands for. It differs from the default row only where what a new
// row is and what an old file meant part ways — a light added today casts no
// shadow, a light saved before the field existed did. Optional; a type without
// one reads its default row. ecs never reads it; the reader of files does.
//
// Copies `row` — the type's registered size — as the type's unsaid row, into
// memory the world pushes for it. Once per type: a second call asserts.
void voe_ecs_component_unsaid_set(voe_ecs_world *world, voe_ecs_type type,
				  const void *row);

// The unsaid row, or NULL when none was set. Valid as long as the world.
const void *voe_ecs_component_unsaid(const voe_ecs_world *world,
				     voe_ecs_type type);

// A DESCRIBED TYPE MAY ALSO NAME ONE TYPE ITS ROWS NEED (0193): a shape does
// nothing without a transform on the same entity. Stored and handed back like the
// rest, never read by ecs, never enforced — a tool shows it; nothing here refuses
// an add over it.
//
// Says rows of `type` do nothing without a row of `needed` on the same entity.
// Once per type: a second call asserts.
void voe_ecs_component_needs_set(voe_ecs_world *world, voe_ecs_type type,
				 voe_ecs_type needed);

// Writes the needed type to `out` and returns true, or returns false when none
// was set and leaves `out` alone.
bool voe_ecs_component_needs(const voe_ecs_world *world, voe_ecs_type type,
			     voe_ecs_type *out);

// A DESCRIBED TYPE MAY ALSO HAVE A MENU PATH (0217, 0221): where a tool offers
// the type, as parts split on `/` — "Rendering / Shape" is the entry "Shape" in
// the group "Rendering". Stored and handed back like the rest; ecs never parses
// or reads it. The string is the declaring folder's and must outlive the world.
//
// Sets the path of `type`. Once per type: a second call, or a NULL or empty
// `path`, asserts.
void voe_ecs_component_menu_set(voe_ecs_world *world, voe_ecs_type type,
				const char *path);

// The menu path, or NULL when none was set.
const char *voe_ecs_component_menu(const voe_ecs_world *world,
				   voe_ecs_type type);

// The type registered against that key. Asserts if nothing was — a module asking
// for its own type before it registered it is a bug in the order the program
// starts up, not a thing to handle.
voe_ecs_type voe_ecs_component_type(const voe_ecs_world *world,
				    const struct voe_ecs_key *key);

// Copies `value` in as the entity's row. False when the table is full or the
// entity is not alive; a caller cannot tell those apart and does not need to,
// because neither is something to retry. Giving an entity a second component of
// a type it already has is the caller's bug and asserts.
[[nodiscard]] bool voe_ecs_component_add(voe_ecs_world *world, voe_ecs_type type,
					 voe_ecs_entity entity,
					 const void *value);

// Overwrites the row the entity already has. False when it has none — which is
// the ordinary answer for an intent that named an entity that has since been
// destroyed, and the reason this returns rather than asserts.
[[nodiscard]] bool voe_ecs_component_set(voe_ecs_world *world, voe_ecs_type type,
					 voe_ecs_entity entity,
					 const void *value);

// NULL when the entity has no row of this type, is not alive, or never was. The
// pointer is into the table, so it is valid until the next add or remove of this
// type and no longer.
const void *voe_ecs_component_get(const voe_ecs_world *world, voe_ecs_type type,
				  voe_ecs_entity entity);

// REMOVING SWAPS THE LAST ROW INTO THE HOLE, so row order is not insertion order
// and does not survive a removal. Anything that depends on the order rows come
// out in is depending on a thing this table does not promise; what it does
// promise is that every live row is visited exactly once.
//
// True when there was one to remove.
bool voe_ecs_component_remove(voe_ecs_world *world, voe_ecs_type type,
			      voe_ecs_entity entity);

uint32_t voe_ecs_component_count(const voe_ecs_world *world, voe_ecs_type type);

// The rows, packed, `count` of them. Cast it to the component's own type; the
// module that registered the type is the one that knows what it is.
const void *voe_ecs_component_rows(const voe_ecs_world *world,
				   voe_ecs_type type);

// Who each row belongs to, same order, same count. This is how a system that has
// walked one table looks the second component up.
const voe_ecs_entity *voe_ecs_component_entities(const voe_ecs_world *world,
						 voe_ecs_type type);
