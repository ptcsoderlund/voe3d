// The compressing queue of game/compressing.h: up to sixteen jobs, oldest
// first, each with an arena of its own and a stage; a step moves the first
// job on: a read burst, a GPU start, its readiness, a write burst, its end.
// A burst's task and the caller never touch a job's arena at once: the
// caller waits for the burst's end before it reads the job again.
//
// Constraints: a job leaves by shifting the ones behind it down, sixteen at
// most; only the first runs, so no burst holds a job that moves. A second
// add while the first job runs is kept as `again` and its stamp, taken when
// the running stage ends, since the task may still be using the arena.
#include <game/compressing.h>

#include "models_cooked.h"

#include <3d/model_prepare.h>

#include <base/assert.h>
#include <base/report.h>

#include <platform/burst.h>
#include <platform/file.h>
#include <platform/path.h>

#include <stdlib.h>
#include <string.h>

// A job's arena's block size, not a limit: a push past one gets its own.
#define JOB_BLOCK (1024 * 1024)

enum stage {
	// Added; nothing of it runs.
	WAITING,
	// Its read burst runs.
	READING,
	// Its chain is made; the GPU has not taken it.
	READ,
	// The GPU compresses it.
	COMPRESSING,
	// Its write burst runs.
	WRITING,
};

struct job {
	voe_base_arena *memory;
	// Where its arena stood after its strings: a restart rewinds there.
	struct voe_base_arena_mark strings;
	const char *folder;
	const char *path;
	voe_assets_image_kind kind;
	uint64_t stamp;
	enum stage stage;
	voe_platform_burst *burst;
	voe_render_compressing *gpu;
	// Written by the read task: whether the chain was made.
	bool read;
	voe_assets_image chain;
	voe_assets_image blocks;
	// A second add met while a stage ran: start over at `again_stamp`.
	bool again;
	uint64_t again_stamp;
};

struct voe_game_compressing {
	struct job jobs[VOE_GAME_COMPRESSING];
	uint32_t count;
};

voe_game_compressing *voe_game_compressing_new(void)
{
	voe_game_compressing *queue = calloc(1, sizeof(*queue));

	VOE_BASE_ASSERT(queue != NULL, "no room for a compressing queue");
	VOE_BASE_ASSERT(queue->count == 0, "a new queue holding jobs");
	return queue;
}

void voe_game_compressing_destroy(voe_game_compressing *queue,
				  voe_render_device *device)
{
	if (queue == NULL)
		return;
	voe_game_compressing_clear(queue, device);
	free(queue);
}

// `text` copied onto `arena`.
static const char *text_copy(voe_base_arena *arena, const char *text)
{
	size_t size = strlen(text) + 1;
	char *copy = voe_base_arena_push(arena, size);

	VOE_BASE_ASSERT(copy != NULL, "no room for a path");
	memcpy(copy, text, size);
	return copy;
}

// `job` made afresh, waiting: a new arena with its strings.
static void job_fill(struct job *job, const char *folder, const char *path,
		     voe_assets_image_kind kind, uint64_t stamp)
{
	VOE_BASE_ASSERT(job->memory == NULL, "filling a job still held");
	job->memory = voe_base_arena_new(JOB_BLOCK);
	job->folder = text_copy(job->memory, folder);
	job->path = text_copy(job->memory, path);
	job->strings = voe_base_arena_mark(job->memory);
	job->kind = kind;
	job->stamp = stamp;
	job->stage = WAITING;
	VOE_BASE_ASSERT(job->burst == NULL && job->gpu == NULL,
			"a filled job running");
}

// `job`, nothing of it running, waiting again at the stamp of its second
// add, its arena back to its strings.
static void job_restart(struct job *job)
{
	VOE_BASE_ASSERT(job->burst == NULL && job->gpu == NULL,
			"restarting a running job");
	voe_base_arena_rewind(job->memory, job->strings);
	job->stamp = job->again_stamp;
	job->stage = WAITING;
	job->read = false;
	job->chain = (voe_assets_image){ 0 };
	job->blocks = (voe_assets_image){ 0 };
	job->again = false;
	VOE_BASE_ASSERT(job->memory != NULL, "restarting an empty job");
}

// `job`'s running burst finished, waiting for it, its GPU job dropped and
// its memory given back, the job empty.
static void job_free(struct job *job, voe_render_device *device)
{
	if (job->burst != NULL)
		voe_platform_burst_finish(job->burst);
	if (job->gpu != NULL) {
		VOE_BASE_ASSERT(device != NULL, "dropping a GPU job with no device");
		voe_render_compress_finish(device, job->gpu, NULL);
	}
	voe_base_arena_destroy(job->memory);
	*job = (struct job){ 0 };
	VOE_BASE_ASSERT(job->memory == NULL, "a freed job held");
}

void voe_game_compressing_add(voe_game_compressing *queue, const char *folder,
			      const char *path, voe_assets_image_kind kind,
			      uint64_t stamp)
{
	VOE_BASE_ASSERT(queue != NULL && folder != NULL && path != NULL,
			"adding to no queue, or no folder or path");
	for (uint32_t i = 0; i < queue->count; i++) {
		struct job *job = &queue->jobs[i];

		if (job->kind != kind || strcmp(job->path, path) != 0)
			continue;
		if (job->burst != NULL || job->gpu != NULL) {
			job->again = true;
			job->again_stamp = stamp;
			return;
		}
		job_free(job, NULL);
		job_fill(job, folder, path, kind, stamp);
		return;
	}
	if (queue->count == VOE_GAME_COMPRESSING) {
		VOE_BASE_WARNING("game",
				 "the map `%s` is not compressed now: %d maps "
				 "already wait; it is at the next open, Play or Ship",
				 path, VOE_GAME_COMPRESSING);
		return;
	}
	job_fill(&queue->jobs[queue->count], folder, path, kind, stamp);
	queue->count++;
}

// The read burst's one task, on its worker, under the job's path: the map
// read, decoded and mipped by its kind onto the job's arena, `read` set; one
// that will not read or decode said at its cause, as the cook says it.
static void job_read(void *context, uint32_t index)
{
	struct job *job = context;
	voe_base_error error = VOE_BASE_OK;
	voe_assets_image image = { 0 };
	const uint8_t *bytes;
	size_t size = 0;

	VOE_BASE_ASSERT(job != NULL && index == 0, "reading no job");
	voe_base_report_subject_push(job->path);
	bytes = voe_platform_file_read(
		voe_platform_path_join(job->memory, job->folder, job->path),
		job->memory, &size, &error);
	if (bytes != NULL && !voe_3d_model_picture_is(job->path))
		VOE_BASE_ERROR("game",
			       "the map `%s` is not a PNG or JPEG picture; save it as PNG",
			       job->path);
	else if (bytes != NULL &&
		 voe_3d_model_picture_decode(job->path, bytes, size, job->memory,
					     &image, &error)) {
		job->chain = voe_assets_image_mips(job->memory, &image,
						   job->kind);
		job->read = true;
	}
	voe_base_report_subject_pop();
}

// The write burst's one task, on its worker, under the job's path: its
// blocks written as its copy keyed by its stamp.
static void job_write(void *context, uint32_t index)
{
	struct job *job = context;

	VOE_BASE_ASSERT(job != NULL && index == 0, "writing no job");
	VOE_BASE_ASSERT(job->blocks.pixels != NULL, "writing no blocks");
	voe_base_report_subject_push(job->path);
	voe_game_models_picture_write(job->folder, job->path, &job->blocks,
				      job->kind, job->stamp, job->memory);
	voe_base_report_subject_pop();
}

// The first job freed and the rest shifted down.
static void job_leave(voe_game_compressing *queue)
{
	VOE_BASE_ASSERT(queue->count > 0, "no job to leave");
	job_free(&queue->jobs[0], NULL);
	queue->count--;
	memmove(&queue->jobs[0], &queue->jobs[1],
		queue->count * sizeof(queue->jobs[0]));
	queue->jobs[queue->count] = (struct job){ 0 };
}

// The first job's chain handed to the GPU, its blocks' room made once; a
// start that fails leaves it READ for the next step.
static void job_start(struct job *job, voe_render_device *device)
{
	voe_base_error error = VOE_BASE_OK;

	VOE_BASE_ASSERT(job->stage == READ && job->read, "starting no chain");
	if (job->blocks.pixels == NULL)
		job->blocks = voe_assets_image_blocks_new(job->memory,
							  &job->chain, job->kind);
	job->gpu = voe_render_compress_start(
		device,
		job->kind == VOE_ASSETS_IMAGE_NORMAL ? VOE_RENDER_BLOCKS_BC5 :
						       VOE_RENDER_BLOCKS_BC7,
		job->chain.width, job->chain.height, job->chain.levels,
		job->chain.pixels, &error);
	// Render said the one line.
	if (job->gpu != NULL)
		job->stage = COMPRESSING;
}

// The first job's running stage ended: its burst finished or its GPU job
// taken into its blocks (dropped when it starts over).
static void job_ended(struct job *job, voe_render_device *device)
{
	if (job->burst != NULL)
		voe_platform_burst_finish(job->burst);
	if (job->gpu != NULL)
		voe_render_compress_finish(device, job->gpu,
					   job->again ? NULL : job->blocks.pixels);
	job->burst = NULL;
	job->gpu = NULL;
}

// Whether the first job's running stage has ended, never waiting.
static bool job_done(const struct job *job, voe_render_device *device)
{
	if (job->burst != NULL)
		return voe_platform_burst_done(job->burst);
	if (job->gpu != NULL)
		return voe_render_compress_ready(device, job->gpu);
	return true;
}

const char *voe_game_compressing_step(voe_game_compressing *queue,
				      voe_render_device *device,
				      voe_base_arena *scratch)
{
	const char *written = NULL;
	struct job *job;

	VOE_BASE_ASSERT(queue != NULL && device != NULL && scratch != NULL,
			"stepping no queue, or with no device or scratch");
	if (queue->count == 0 || !job_done(&queue->jobs[0], device))
		return NULL;
	job = &queue->jobs[0];
	job_ended(job, device);
	if (job->again) {
		job_restart(job);
		return NULL;
	}
	switch (job->stage) {
	case WAITING:
		job->stage = READING;
		job->burst = voe_platform_burst_start(1, 1, job_read, job);
		break;
	case READING:
		// The read task said why it made no chain.
		if (!job->read)
			job_leave(queue);
		else
			job->stage = READ;
		break;
	case READ:
		job_start(job, device);
		break;
	case COMPRESSING:
		job->stage = WRITING;
		job->burst = voe_platform_burst_start(1, 1, job_write, job);
		break;
	case WRITING:
		written = text_copy(scratch, job->path);
		job_leave(queue);
		break;
	}
	VOE_BASE_ASSERT(queue->count <= VOE_GAME_COMPRESSING, "too many jobs");
	return written;
}

uint32_t voe_game_compressing_waiting(const voe_game_compressing *queue)
{
	VOE_BASE_ASSERT(queue != NULL, "asking no queue");
	return queue->count;
}

void voe_game_compressing_clear(voe_game_compressing *queue,
				voe_render_device *device)
{
	VOE_BASE_ASSERT(queue != NULL, "clearing no queue");
	for (uint32_t i = 0; i < queue->count; i++)
		job_free(&queue->jobs[i], device);
	queue->count = 0;
}
