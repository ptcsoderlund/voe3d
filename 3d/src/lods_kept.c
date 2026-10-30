// A view's kept LODs (3d/lods_kept.h): two tables of stamped slots probed
// linearly from a mixed key, this frame's filled as copies are chosen and last
// frame's read for the LOD each kept; the swap rule over the stateless size
// and level of model_levels.h; and the two kinds of key.
//
// A SLOT IS LIVE WHEN ITS STAMP IS ITS TABLE'S FRAME. Frames are numbered from
// 2 at the first _begin, table `frame & 1` this frame's, so a zeroed slot
// (stamp 0) and a slot of two frames ago are both empty. Within a frame slots
// are only added, so a probe stops at the first empty one.
//
// A SLOT KEEPS THE MOMENT OF ITS LAST SWAP, carried from frame to frame while
// its LOD stays; a first sight keeps −INFINITY, settled at once.
#include <3d/lods_kept.h>

#include <base/assert.h>

#include <math.h>

#define SLOTS (2u * VOE_3D_LODS_KEPT)
#define COPY_BIT (1ull << 63)

static_assert((SLOTS & (SLOTS - 1)) == 0, "slots a power of two");
static_assert(VOE_3D_MODEL_LEVELS <= UINT8_MAX, "a level fits a byte");

struct slot {
	uint64_t key;
	// When the LOD last swapped, on the caller's clock.
	double swapped;
	uint32_t stamp;
	uint8_t level;
	uint8_t from;
};

struct voe_3d_lods_kept {
	struct slot *tables[2];
	// Keys this frame's table holds.
	uint32_t count;
	// This frame's number; 1 before the first _begin.
	uint32_t frame;
	// This frame's clock and last frame's.
	double now;
	double then;
};

// SplitMix64's finaliser: every bit of `h` reaches every bit of the result.
static uint64_t mix(uint64_t h)
{
	h ^= h >> 30;
	h *= 0xbf58476d1ce4e5b9ull;
	h ^= h >> 27;
	h *= 0x94d049bb133111ebull;
	return h ^ (h >> 31);
}

voe_3d_lods_kept *voe_3d_lods_kept_create(voe_base_arena *arena)
{
	voe_3d_lods_kept *kept;

	VOE_BASE_ASSERT(arena != NULL, "a memory on no arena");
	kept = voe_base_arena_push(arena, sizeof(*kept));
	for (int t = 0; t < 2; t++)
		kept->tables[t] =
			voe_base_arena_push(arena, SLOTS * sizeof(struct slot));
	kept->frame = 1;
	kept->now = -INFINITY;
	kept->then = -INFINITY;
	return kept;
}

void voe_3d_lods_kept_begin(voe_3d_lods_kept *kept, double now)
{
	VOE_BASE_ASSERT(kept != NULL, "beginning no memory");
	VOE_BASE_ASSERT(isfinite(now) && now >= kept->now,
			"a clock not finite or going back");
	kept->frame++;
	kept->count = 0;
	kept->then = kept->now;
	kept->now = now;
}

// `key`'s slot in the table of `frame`: its live slot, or the empty one it
// would take; NULL when the table holds neither, which its room prevents.
static struct slot *probe(const voe_3d_lods_kept *kept, uint32_t frame,
			  uint64_t key)
{
	struct slot *table = kept->tables[frame & 1];
	uint32_t at = (uint32_t)mix(key) & (SLOTS - 1);

	for (uint32_t i = 0; i < SLOTS; i++, at = (at + 1) & (SLOTS - 1))
		if (table[at].stamp != frame || table[at].key == key)
			return &table[at];
	return NULL;
}

// A slot's choice at `now`: dissolving from `from` while under the dissolve's
// time since its swap, else whole.
static voe_3d_model_level_choice answer(const struct slot *slot, double now)
{
	const double since = now - slot->swapped;

	if (since < VOE_3D_LODS_DISSOLVE)
		return (voe_3d_model_level_choice){
			.level = slot->level,
			.from = slot->from,
			.shown = (float)(since / VOE_3D_LODS_DISSOLVE),
		};
	return (voe_3d_model_level_choice){ slot->level, slot->level, 1.0f };
}

// The LOD a copy at `s` keeps from `last`, with its from and swap moment
// carried or, on a swap, set to `last`'s and `now`.
static void keep_or_swap(const voe_3d_model_entry *model, float s,
			 const struct slot *last, double now, struct slot *out)
{
	const uint32_t near = voe_3d_model_level_at(model, s);
	const uint32_t back = voe_3d_model_level_at(model, VOE_3D_LODS_BACK * s);
	uint32_t level = last->level;

	if (near > level)
		level = near;
	else if (back < level)
		level = back;
	out->level = (uint8_t)level;
	out->from = level == last->level ? last->from : last->level;
	out->swapped = level == last->level ? last->swapped : now;
}

voe_3d_model_level_choice
voe_3d_lods_kept_choose(voe_3d_lods_kept *kept, uint64_t key,
			const voe_3d_model_entry *model,
			const voe_scene_transform *placed, voe_math_double3 eye,
			const voe_render_view *view)
{
	const float s = voe_3d_model_level_size(model, placed, eye, view);
	const uint32_t level = voe_3d_model_level_at(model, s);
	const voe_3d_model_level_choice whole = { level, level, 1.0f };
	struct slot *slot;
	const struct slot *last;

	if (kept == NULL)
		return whole;
	VOE_BASE_ASSERT(kept->frame >= 2, "choosing before a frame began");
	slot = probe(kept, kept->frame, key);
	VOE_BASE_ASSERT(slot != NULL, "a table past its room");
	if (slot->stamp == kept->frame)
		return answer(slot, kept->now);
	if (kept->count >= VOE_3D_LODS_KEPT)
		return whole;

	last = probe(kept, kept->frame - 1, key);
	*slot = (struct slot){ .key = key, .stamp = kept->frame };
	kept->count++;
	if (last != NULL && last->stamp == kept->frame - 1 &&
	    last->level < model->level_count && last->from < model->level_count) {
		keep_or_swap(model, s, last, kept->now, slot);
		return answer(slot, kept->now);
	}
	slot->level = (uint8_t)level;
	slot->from = (uint8_t)level;
	slot->swapped = -INFINITY;
	return whole;
}

bool voe_3d_lods_kept_find(const voe_3d_lods_kept *kept, uint64_t key,
			   voe_3d_model_level_choice *out)
{
	const struct slot *slot;

	VOE_BASE_ASSERT(kept != NULL && out != NULL, "finding in no memory");
	VOE_BASE_ASSERT(kept->frame >= 2, "finding before a frame began");
	slot = probe(kept, kept->frame, key);
	if (slot != NULL && slot->stamp == kept->frame) {
		*out = answer(slot, kept->now);
		return true;
	}
	slot = probe(kept, kept->frame - 1, key);
	if (slot != NULL && slot->stamp == kept->frame - 1) {
		*out = answer(slot, kept->then);
		return true;
	}
	return false;
}

uint64_t voe_3d_lods_kept_row_key(voe_ecs_entity row)
{
	return ((uint64_t)row.generation << 32 | row.index) & ~COPY_BIT;
}

uint64_t voe_3d_lods_kept_copy_key(voe_ecs_entity landscape, uint32_t kind,
				   int32_t x, int32_t z)
{
	uint64_t h = mix((uint64_t)landscape.generation << 32 | landscape.index);

	h = mix(h ^ kind);
	h = mix(h ^ ((uint64_t)(uint32_t)z << 32 | (uint32_t)x));
	return h | COPY_BIT;
}
