// The arena's blocks and the pointer bump. See include/base/arena.h for what an
// arena is for and for the one thing a caller can get wrong.
//
// A block is one malloc: the header, then the payload as a flexible array member
// aligned to VOE_BASE_ARENA_ALIGNMENT. malloc returns memory aligned for every
// fundamental type, and alignas pushes the payload to the next multiple of the
// alignment, so data is aligned without any pointer arithmetic here.
//
// Blocks are only ever freed by destroy. Rewind and clear move the write
// position back and leave the chain alone, which is what makes the addresses
// come back in the same order. A block is zeroed on the way out of push rather
// than on the way in, because a reused block is dirty and push promises zeroed.
#include <base/arena.h>

#include <base/assert.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct voe_base_arena_block {
	struct voe_base_arena_block *next;
	size_t size;
	size_t used;
	alignas(VOE_BASE_ARENA_ALIGNMENT) unsigned char data[];
};

struct voe_base_arena {
	struct voe_base_arena_block *first;
	struct voe_base_arena_block *current;
	size_t block_size;
};

static size_t align_up(size_t n)
{
	VOE_BASE_ASSERT(n <= SIZE_MAX - (VOE_BASE_ARENA_ALIGNMENT - 1),
			"a size this close to SIZE_MAX cannot be aligned");
	return (n + VOE_BASE_ARENA_ALIGNMENT - 1) &
	       ~(size_t)(VOE_BASE_ARENA_ALIGNMENT - 1);
}

// payload is already aligned. The block is linked by the caller, because where
// it goes in the chain differs between "append" and "insert before a block that
// is too small".
static struct voe_base_arena_block *block_new(size_t payload)
{
	struct voe_base_arena_block *block;

	VOE_BASE_ASSERT(payload <= SIZE_MAX - sizeof(*block),
			"a block of this size overflows its own header");
	block = malloc(sizeof(*block) + payload);
	VOE_BASE_ASSERT(block != NULL, "out of memory allocating an arena block");

	block->next = NULL;
	block->size = payload;
	block->used = 0;
	return block;
}

voe_base_arena *voe_base_arena_new(size_t block_size)
{
	voe_base_arena *arena;

	VOE_BASE_ASSERT(block_size > 0, "an arena needs a block size");

	arena = malloc(sizeof(*arena));
	VOE_BASE_ASSERT(arena != NULL, "out of memory allocating an arena");

	arena->block_size = align_up(block_size);
	arena->first = block_new(arena->block_size);
	arena->current = arena->first;
	return arena;
}

void voe_base_arena_destroy(voe_base_arena *arena)
{
	struct voe_base_arena_block *block;

	VOE_BASE_DEBUG_ASSERT(arena != NULL, "destroying a NULL arena");

	block = arena->first;
	while (block != NULL) {
		struct voe_base_arena_block *next = block->next;

		free(block);
		block = next;
	}
	free(arena);
}

// The block to write the next push into, growing the chain if it has to. A block
// kept by a rewind is reused when it is big enough; when it is not, a new one is
// inserted in front of it so that the order the addresses come back in survives.
static struct voe_base_arena_block *block_for(voe_base_arena *arena, size_t size)
{
	struct voe_base_arena_block *current = arena->current;
	struct voe_base_arena_block *block;

	if (size <= current->size - current->used)
		return current;

	if (current->next != NULL && current->next->size >= size) {
		current->next->used = 0;
		arena->current = current->next;
		return current->next;
	}

	block = block_new(size > arena->block_size ? size : arena->block_size);
	block->next = current->next;
	current->next = block;
	arena->current = block;
	return block;
}

void *voe_base_arena_push(voe_base_arena *arena, size_t size)
{
	struct voe_base_arena_block *block;
	unsigned char *at;
	size_t step;

	VOE_BASE_DEBUG_ASSERT(arena != NULL, "pushing on a NULL arena");
	VOE_BASE_DEBUG_ASSERT(size > 0, "a push of zero bytes has no caller");

	step = align_up(size);
	block = block_for(arena, step);
	at = block->data + block->used;
	block->used += step;

	memset(at, 0, size);
	return at;
}

struct voe_base_arena_mark voe_base_arena_mark(voe_base_arena *arena)
{
	VOE_BASE_DEBUG_ASSERT(arena != NULL, "marking a NULL arena");

	return (struct voe_base_arena_mark){ arena->current,
					     arena->current->used };
}

void voe_base_arena_rewind(voe_base_arena *arena, struct voe_base_arena_mark mark)
{
	VOE_BASE_DEBUG_ASSERT(arena != NULL, "rewinding a NULL arena");
	VOE_BASE_DEBUG_ASSERT(mark.block != NULL,
			      "a mark from no arena at all");

	arena->current = mark.block;
	mark.block->used = mark.used;
}

void voe_base_arena_clear(voe_base_arena *arena)
{
	VOE_BASE_DEBUG_ASSERT(arena != NULL, "clearing a NULL arena");

	arena->current = arena->first;
	arena->first->used = 0;
}
