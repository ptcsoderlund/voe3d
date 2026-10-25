// The compressing queue of game/compressing.h: up to sixteen jobs, oldest
// first, each with an arena of its own holding its folder, path, RGBA8 chain
// and the blocks being made; a step encodes the first job's next runs and,
// once its last block is made, writes its copy through models_cooked.h,
// keeps its path in the queue's `ended` arena and destroys its own.
//
// Constraints: a job leaves by shifting the ones behind it down, sixteen at
// most. The returned path lives in `ended`, cleared at the next step.
#include <game/compressing.h>

#include "models_cooked.h"

#include <base/assert.h>
#include <base/report.h>

#include <platform/clock.h>

#include <stdlib.h>
#include <string.h>

// Blocks encoded between two readings of the clock.
#define RUN 64

// A job's arena's block size: its strings; the chain and blocks get their own.
#define JOB_BLOCK 1024

struct job {
	voe_base_arena *memory;
	const char *folder;
	const char *path;
	voe_assets_image_kind kind;
	uint64_t stamp;
	voe_assets_image chain;
	voe_assets_image blocks;
	uint32_t next;
	uint32_t total;
};

struct voe_game_compressing {
	struct job jobs[VOE_GAME_COMPRESSING];
	uint32_t count;
	voe_base_arena *ended;
};

voe_game_compressing *voe_game_compressing_new(void)
{
	voe_game_compressing *queue = calloc(1, sizeof(*queue));

	VOE_BASE_ASSERT(queue != NULL, "no room for a compressing queue");
	queue->ended = voe_base_arena_new(JOB_BLOCK);
	return queue;
}

void voe_game_compressing_destroy(voe_game_compressing *queue)
{
	if (queue == NULL)
		return;
	voe_game_compressing_clear(queue);
	voe_base_arena_destroy(queue->ended);
	free(queue);
}

// `text` copied onto `arena`.
static const char *text_copy(voe_base_arena *arena, const char *text)
{
	size_t size = strlen(text) + 1;
	char *copy = voe_base_arena_push(arena, size);

	memcpy(copy, text, size);
	return copy;
}

// `job` made afresh: a new arena with the strings, the chain and the room for
// its blocks, none encoded.
static void job_fill(struct job *job, const char *folder, const char *path,
		     voe_assets_image_kind kind, uint64_t stamp,
		     const voe_assets_image *chain)
{
	size_t size = voe_assets_image_size(chain);

	VOE_BASE_ASSERT(job->memory == NULL, "filling a job still held");
	job->memory = voe_base_arena_new(JOB_BLOCK);
	job->folder = text_copy(job->memory, folder);
	job->path = text_copy(job->memory, path);
	job->kind = kind;
	job->stamp = stamp;
	job->chain = *chain;
	job->chain.pixels = voe_base_arena_push(job->memory, size);
	memcpy(job->chain.pixels, chain->pixels, size);
	job->blocks = voe_assets_image_blocks_new(job->memory, &job->chain,
						  kind);
	job->next = 0;
	job->total = voe_assets_image_block_count(&job->blocks);
	VOE_BASE_ASSERT(job->total > 0, "a job with no blocks");
}

// `job`'s memory given back, the job empty.
static void job_free(struct job *job)
{
	voe_base_arena_destroy(job->memory);
	*job = (struct job){ 0 };
}

void voe_game_compressing_add(voe_game_compressing *queue, const char *folder,
			      const char *path, voe_assets_image_kind kind,
			      uint64_t stamp, const voe_assets_image *chain)
{
	VOE_BASE_ASSERT(queue != NULL && folder != NULL && path != NULL,
			"adding to no queue, or no folder or path");
	VOE_BASE_ASSERT(chain != NULL && chain->pixels != NULL &&
				chain->format == VOE_ASSETS_IMAGE_RGBA8,
			"compressing no RGBA8 chain");
	for (uint32_t i = 0; i < queue->count; i++) {
		struct job *job = &queue->jobs[i];

		if (job->kind != kind || strcmp(job->path, path) != 0)
			continue;
		job_free(job);
		job_fill(job, folder, path, kind, stamp, chain);
		return;
	}
	if (queue->count == VOE_GAME_COMPRESSING) {
		VOE_BASE_WARNING("game",
				 "the map `%s` is not compressed now: %d maps "
				 "already wait; it is at the next open, Play or Ship",
				 path, VOE_GAME_COMPRESSING);
		return;
	}
	job_fill(&queue->jobs[queue->count], folder, path, kind, stamp, chain);
	queue->count++;
}

// The first job written as its copy under its path, its path kept in
// `ended`, it freed and the rest shifted down. Scratch rewound.
static const char *job_end(voe_game_compressing *queue,
			   voe_base_arena *scratch)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	struct job *job = &queue->jobs[0];
	const char *path = text_copy(queue->ended, job->path);

	voe_base_report_subject_push(job->path);
	voe_game_models_picture_write(job->folder, job->path, &job->blocks,
				      job->kind, job->stamp, scratch);
	voe_base_report_subject_pop();
	voe_base_arena_rewind(scratch, mark);
	job_free(job);
	queue->count--;
	memmove(&queue->jobs[0], &queue->jobs[1],
		queue->count * sizeof(queue->jobs[0]));
	queue->jobs[queue->count] = (struct job){ 0 };
	return path;
}

const char *voe_game_compressing_step(voe_game_compressing *queue,
				      double seconds, voe_base_arena *scratch)
{
	double start = voe_platform_clock_now();
	struct job *job;

	VOE_BASE_ASSERT(queue != NULL && scratch != NULL,
			"stepping no queue, or with no scratch");
	VOE_BASE_ASSERT(seconds >= 0.0, "a step of negative seconds");
	voe_base_arena_clear(queue->ended);
	if (queue->count == 0)
		return NULL;
	job = &queue->jobs[0];
	// Bounded by the job's blocks: each pass encodes at least one.
	while (job->next < job->total) {
		uint32_t count = job->total - job->next < RUN ?
					 job->total - job->next :
					 RUN;

		voe_assets_image_blocks_encode(&job->chain, job->kind,
					       &job->blocks, job->next, count);
		job->next += count;
		if (voe_platform_clock_now() - start >= seconds)
			break;
	}
	return job->next == job->total ? job_end(queue, scratch) : NULL;
}

uint32_t voe_game_compressing_waiting(const voe_game_compressing *queue)
{
	VOE_BASE_ASSERT(queue != NULL, "asking no queue");
	return queue->count;
}

void voe_game_compressing_clear(voe_game_compressing *queue)
{
	VOE_BASE_ASSERT(queue != NULL, "clearing no queue");
	for (uint32_t i = 0; i < queue->count; i++)
		job_free(&queue->jobs[i]);
	queue->count = 0;
}
