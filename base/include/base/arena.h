// The arena. Working memory in this engine is handed to a function as a
// voe_base_arena parameter, used, and then released all at once by rewinding or
// destroying the arena. There is no global arena and no default one: memory that
// appears without a parameter saying where it came from is the thing this design
// exists to prevent.
//
//     voe_base_arena *a = voe_base_arena_new(64 * 1024);
//     struct voe_base_arena_mark m = voe_base_arena_mark(a);
//     void *scratch = voe_base_arena_push(a, 4096);
//     voe_base_arena_rewind(a, m);          // scratch is gone
//     voe_base_arena_destroy(a);
//
// There is no voe_base_arena_free. A single allocation is never given back; that
// is the trade the arena makes, and it is why push can be a pointer bump.
//
// THE SHARP EDGE: two pushes are not guaranteed to be next to each other. The
// arena is a chain of malloc'd blocks, so a push that does not fit in the
// current block starts a new one, which may sit anywhere in memory. Do not push
// n times and index across the results as if they were an array — push the array
// once. This is the one thing a caller can get wrong and the compiler will not
// say a word.
//
// A push either succeeds or aborts. It never returns NULL, so a caller never
// writes a null check after one, and "what if this failed" stays off every
// function in the engine. Failing to allocate is fatal here, not recoverable —
// see base/assert.h for the line between the two.
//
// Single-threaded. No locking, no atomics, no thread-local anything. An arena
// belongs to whoever holds the pointer, and two threads pushing on one arena is
// a bug this code will not detect.
//
// Rewinding and clearing keep the blocks. Addresses handed out before a rewind
// come back after it, in the same order, and only destroy returns memory to the
// C library.
#pragma once

#include <stddef.h>

// Every push is aligned to this, and so is the block a push comes out of.
#define VOE_BASE_ARENA_ALIGNMENT 16

typedef struct voe_base_arena voe_base_arena;

// Opaque. A mark names a position inside one block, and only the arena that
// produced it can read it.
struct voe_base_arena_block;

// A position to rewind back to. It is a struct tag rather than a typedef because
// voe_base_arena_mark is already the name of the function that makes one, and
// one concept deserves one name; C keeps tags and function names apart, so both
// spellings can be the word the card uses.
struct voe_base_arena_mark {
	struct voe_base_arena_block *block;
	size_t used;
};

// block_size is the size of each block, rounded up to VOE_BASE_ARENA_ALIGNMENT.
// A push larger than it gets a block of its own, so the number is a default and
// not a limit.
voe_base_arena *voe_base_arena_new(size_t block_size);
void voe_base_arena_destroy(voe_base_arena *arena);

// Aligned to VOE_BASE_ARENA_ALIGNMENT, zeroed, and never NULL.
void *voe_base_arena_push(voe_base_arena *arena, size_t size);

struct voe_base_arena_mark voe_base_arena_mark(voe_base_arena *arena);
void voe_base_arena_rewind(voe_base_arena *arena, struct voe_base_arena_mark mark);

// Rewind to the beginning. Equivalent to rewinding to a mark taken before the
// first push, without having had to take one.
void voe_base_arena_clear(voe_base_arena *arena);
