// A world cooked into C source: the one cook (ADR-0235, ADR-0237 point 1).
//
//     #include <game/scene.h>
//
//     bool voe_game_scene_build(voe_ecs_world *world)
//     {
//             voe_ecs_entity e[1];
//
//             for (uint32_t i = 0; i < 1; i++)
//                     if (!voe_ecs_entity_create(world, &e[i]))
//                             return false;
//             if (!voe_ecs_component_add(world, voe_ecs_component_type(world,
//                     &voe_scene_identity_key), e[0], &(voe_scene_identity){
//                     .id = 1ull, .name = "Cube" }))
//                     return false;
//             return true;
//     }
//
// C SOURCE, BECAUSE THE GAME COMPILES ITS DATA IN AND NEVER PARSES PROJECT TEXT
// (ADR-0236). The game build compiles this text beside the engine; there is no
// reader in the game to keep in step with the scene format.
//
// WHAT IS COOKED IS WHAT scene_write.h WRITES, IN THE SAME ORDER: entities with an
// identity, ascending by authored id, and beneath each every described type that
// is not runtime-only — identity included — in the byte order of its key name. A
// key's name is also its struct's name. KEPT SECTIONS ARE NOT COOKED: nothing in
// the game reads them. PREFAB PARTS ARE: the cook takes the world as the editor
// holds it, copies expanded (ADR-0283 point 3), unlike the writer.
//
// FLOATS ARE HEX LITERALS, BECAUSE PLAY MUST SHOW THE EDITOR'S EXACT BYTES
// (ADR-0188). `%a` round-trips every finite value bit for bit.
//
// THE COOKED FUNCTION IS RULE 3'S THIRD CREATION EXCEPTION (ADR-0237), beside a
// folder's typed creation call and the scene reader: it adds rows to entities it
// has just made and edits none.
//
// It reads the world and never writes it; nothing here opens a file. The entity
// array of the cooked function is a local, so a scene of millions of entities
// would need it moved off the stack.
#pragma once

#include <authoring/scene_write.h>
#include <base/arena.h>
#include <ecs/world.h>

// Cooks the world into C source that includes `include` (spelled as it goes
// between the angle brackets) and defines `bool <function>(voe_ecs_world *)`.
// Same arena and failure contract as voe_authoring_scene_write: on success
// `*out` holds the text; on failure it returns false, reports why and leaves
// `*out` untouched; what is pushed either way is the caller's to rewind.
//
// A VALUE IS COOKED BY ITS FIELD'S KIND AND SHAPE, as a designated initialiser:
//
//   - integers in decimal with the suffix their width needs (`u`, `ll`, `ull`);
//   - FLOAT32, FLOAT64 and DOUBLE3's elements as `%a` hex literals, `f` for 32;
//     BOOL `true`/`false`;
//     ENUM as its integer;
//   - vector kinds, FLOAT4X4 and each array dimension as nested braces in memory
//     order;
//   - CHAR as a string literal up to the first NUL, `"`, `\` and bytes outside
//     printable ASCII escaped in octal; the innermost dimension is the string;
//   - ENTITY as the cooked entity with that authored id, or a zeroed
//     voe_ecs_entity for a dead one or one with no identity.
//
// IT REFUSES, naming entity, component and field through base/report.h, on a NaN
// or infinite float, two entities with one authored id, and a described type
// whose description this build compiled out.
[[nodiscard]] bool voe_authoring_scene_cook(const voe_ecs_world *world,
					    const char *include,
					    const char *function,
					    voe_base_arena *arena,
					    voe_authoring_text *out);
