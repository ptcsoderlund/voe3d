// The arena's tests. Every promise in include/base/arena.h that a caller can
// check from outside: alignment, zeroing, a push bigger than a block, and that
// rewind and clear hand the same addresses back.
//
// The assert path is not tested here. It aborts the process by design, and a
// test that aborts is a test that reports nothing.
#include <base/arena.h>

#include <testing/test.h>

#include <stdint.h>

// Small, so that a handful of pushes crosses several blocks and the chaining is
// exercised rather than described.
#define BLOCK 256

static int aligned(const void *p)
{
	return ((uintptr_t)p % VOE_BASE_ARENA_ALIGNMENT) == 0;
}

static int all_zero(const unsigned char *p, size_t n)
{
	for (size_t i = 0; i < n; i++)
		if (p[i] != 0)
			return 0;
	return 1;
}

static int all_equal(const unsigned char *p, size_t n, unsigned char value)
{
	for (size_t i = 0; i < n; i++)
		if (p[i] != value)
			return 0;
	return 1;
}

// A push is writable, zeroed and aligned, whatever the size asked for.
static void push_is_aligned_zeroed_and_writable(void)
{
	voe_base_arena *arena = voe_base_arena_new(BLOCK);
	static const size_t sizes[] = { 1, 3, 16, 17, 64 };

	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
		unsigned char *p = voe_base_arena_push(arena, sizes[i]);

		VOE_TEST_CHECK(p != NULL);
		VOE_TEST_CHECK(aligned(p));
		VOE_TEST_CHECK(all_zero(p, sizes[i]));

		for (size_t b = 0; b < sizes[i]; b++)
			p[b] = 0xAB;
		VOE_TEST_CHECK(all_equal(p, sizes[i], 0xAB));
	}

	voe_base_arena_destroy(arena);
}

// A push no block can hold gets a block of its own, so the block size is a
// default and not a limit.
static void push_larger_than_a_block(void)
{
	voe_base_arena *arena = voe_base_arena_new(BLOCK);
	size_t big = BLOCK * 4 + 7;
	unsigned char *p = voe_base_arena_push(arena, big);

	VOE_TEST_CHECK(p != NULL);
	VOE_TEST_CHECK(aligned(p));
	VOE_TEST_CHECK(all_zero(p, big));

	for (size_t i = 0; i < big; i++)
		p[i] = 0x5A;
	VOE_TEST_CHECK(all_equal(p, big, 0x5A));

	// And the arena still works afterwards.
	VOE_TEST_CHECK(voe_base_arena_push(arena, 32) != NULL);

	voe_base_arena_destroy(arena);
}

// Rewinding to a mark hands the same addresses back, including across the block
// boundary the pushes crossed, and the memory is zeroed again.
static void rewind_replays_the_same_addresses(void)
{
	voe_base_arena *arena = voe_base_arena_new(BLOCK);
	void *before[8];
	void *after[8];
	struct voe_base_arena_mark mark;

	// Get off the start of a block first, so the mark is not the beginning.
	voe_base_arena_push(arena, 48);

	mark = voe_base_arena_mark(arena);
	for (size_t i = 0; i < 8; i++)
		before[i] = voe_base_arena_push(arena, 64);

	voe_base_arena_rewind(arena, mark);
	for (size_t i = 0; i < 8; i++) {
		after[i] = voe_base_arena_push(arena, 64);
		VOE_TEST_CHECK(after[i] == before[i]);
		VOE_TEST_CHECK(all_zero(after[i], 64));
	}

	voe_base_arena_destroy(arena);
}

// Clear goes back to the very start, without a mark having been taken.
static void clear_goes_back_to_the_start(void)
{
	voe_base_arena *arena = voe_base_arena_new(BLOCK);
	void *first = voe_base_arena_push(arena, 32);
	void *again;

	for (size_t i = 0; i < 20; i++)
		voe_base_arena_push(arena, 64);

	voe_base_arena_clear(arena);
	again = voe_base_arena_push(arena, 32);

	VOE_TEST_CHECK(again == first);
	VOE_TEST_CHECK(all_zero(again, 32));

	voe_base_arena_destroy(arena);
}

// The one that catches a bad block boundary: a distinct pattern per allocation,
// enough allocations to force several blocks, and every one of them checked at
// the end rather than only the last.
#define CHUNKS 40
#define CHUNK 40

static void crossing_blocks_does_not_corrupt_earlier_ones(void)
{
	voe_base_arena *arena = voe_base_arena_new(BLOCK);
	unsigned char *chunk[CHUNKS];

	for (size_t i = 0; i < CHUNKS; i++) {
		chunk[i] = voe_base_arena_push(arena, CHUNK);
		VOE_TEST_CHECK(aligned(chunk[i]));
		for (size_t b = 0; b < CHUNK; b++)
			chunk[i][b] = (unsigned char)(i + 1);
	}

	for (size_t i = 0; i < CHUNKS; i++)
		VOE_TEST_CHECK(all_equal(chunk[i], CHUNK,
					 (unsigned char)(i + 1)));

	voe_base_arena_destroy(arena);
}

// Rewinding keeps the blocks it rewound past, and a later push may be too big
// for the one it lands on. The block that is kept has to stay in the chain
// rather than be dropped for the bigger one, or it is never freed.
static void a_kept_block_too_small_is_not_lost(void)
{
	voe_base_arena *arena = voe_base_arena_new(BLOCK);
	struct voe_base_arena_mark mark;
	unsigned char *small;
	unsigned char *large;

	voe_base_arena_push(arena, 32);
	mark = voe_base_arena_mark(arena);

	// Its own block, because it does not fit in BLOCK.
	small = voe_base_arena_push(arena, BLOCK * 4);
	voe_base_arena_rewind(arena, mark);

	// Bigger than the block the rewind kept, so it cannot reuse it.
	large = voe_base_arena_push(arena, BLOCK * 12);
	VOE_TEST_CHECK(aligned(large));
	VOE_TEST_CHECK(all_zero(large, BLOCK * 12));
	VOE_TEST_CHECK(large != small);

	for (size_t i = 0; i < BLOCK * 12; i++)
		large[i] = 0x3C;
	VOE_TEST_CHECK(all_equal(large, BLOCK * 12, 0x3C));

	voe_base_arena_destroy(arena);
}

int main(void)
{
	push_is_aligned_zeroed_and_writable();
	push_larger_than_a_block();
	rewind_replays_the_same_addresses();
	clear_goes_back_to_the_start();
	crossing_blocks_does_not_corrupt_earlier_ones();
	a_kept_block_too_small_is_not_lost();

	return voe_test_result();
}
