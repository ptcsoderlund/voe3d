// The tree species of game/include/game/trees.h grown into the model store:
// each variation a task with an arena of its own, grown, meshed and prepared
// on a burst (3d/tree_grow.h, 3d/model_prepare.h), then put on the caller
// wearing its species' bark and leaf, and the species set over them.
//
// A TASK touches its own record and arena alone, under its variation's path as
// the subject; a refused prepare's text is carried in the record and kept on
// the caller before the put (0490), so the line reads as one thread's. A
// variation that did not prepare is kept failed at stamp 0, its line said
// where the prepare refused it; its arena is destroyed after its put.
//
// _grow's tasks are every row's variations in row order on scratch, one burst
// for them all, finished before the first put. The job holds one row's copy
// and its tasks itself, so the caller's row may go once started.
//
// Constraints: a task's arena chains blocks of TASK_BLOCK; a grown tree at
// every key's maximum takes tens of them. _grow holds all of them at once.
#include <game/trees.h>

#include "models_batch.h"

#include <3d/model_prepare.h>
#include <3d/models.h>
#include <3d/tree_grow.h>

#include <base/assert.h>
#include <base/report.h>

#include <platform/burst.h>

#include <stdlib.h>
#include <string.h>

#define TASK_BLOCK (1024 * 1024)

// One variation grown: what it grows from, its path, and what it gave.
struct tree_task {
	const voe_assets_tree *values;
	uint32_t variation;
	// `<species>#<v>`: a kind path, `#` and up to two digits.
	char path[VOE_ASSETS_LANDSCAPE_KIND_PATH + 4];
	voe_base_arena *arena;
	const voe_3d_model_prepared *prepared;
	const char *carried;
	voe_base_error error;
};

// `task` set for variation `variation` of `values` at `species`, its arena made.
static void task_begin(struct tree_task *task, const voe_assets_tree *values,
		       const char *species, uint32_t variation)
{
	bool fits;

	*task = (struct tree_task){ .values = values, .variation = variation };
	fits = voe_assets_tree_variation_path(task->path, sizeof(task->path),
					      species, variation);
	VOE_BASE_ASSERT(fits, "a species path past a kind path's room");
	task->arena = voe_base_arena_new(TASK_BLOCK);
}

// Burst task: `context` the tasks, `index` the one grown.
static void grow_task(void *context, uint32_t index)
{
	struct tree_task *task = &((struct tree_task *)context)[index];
	voe_3d_tree_skeleton skeleton;
	voe_assets_model *model;

	VOE_BASE_ASSERT(task->arena != NULL, "growing a task never begun");
	voe_base_report_error_clear();
	voe_base_report_subject_push(task->path);
	model = voe_base_arena_push(task->arena, sizeof(*model));
	voe_3d_tree_grow(task->arena, task->values, task->variation, &skeleton);
	voe_3d_tree_mesh(task->arena, &skeleton, task->values, task->variation,
			 model);
	task->prepared =
		voe_3d_model_prepare_grown(task->arena, model, &task->error);
	if (task->prepared == NULL)
		task->carried = voe_game_models_error_carried(task->arena);
	voe_base_report_subject_pop();
}

// `task` into the store on the caller, its carried error kept first, then its
// arena destroyed. Whether it loaded; `*error` why not, else VOE_BASE_OK.
static bool task_put(struct tree_task *task, voe_3d_models *models,
		     voe_render_device *device, voe_base_error *error)
{
	bool put = false;

	VOE_BASE_ASSERT(task->arena != NULL, "putting a task twice");
	if (task->carried != NULL)
		voe_base_report_error_keep(task->carried);
	voe_base_report_subject_push(task->path);
	if (task->prepared != NULL)
		put = voe_3d_models_put_grown(models, device, task->path,
					      task->prepared, task->values->bark,
					      task->values->leaf, &task->error);
	else
		voe_3d_models_fail(models, task->path, 0);
	voe_base_report_subject_pop();
	voe_base_arena_destroy(task->arena);
	task->arena = NULL;
	*error = put ? VOE_BASE_OK : task->error;
	return put;
}

// Asserts `tree` a species _grow and the job can grow.
static void tree_check(const voe_game_tree *tree)
{
	VOE_BASE_ASSERT(tree != NULL && tree->path != NULL,
			"a species with no path");
	VOE_BASE_ASSERT(strlen(tree->path) < VOE_ASSETS_LANDSCAPE_KIND_PATH,
			"a species path past a kind path's room");
	VOE_BASE_ASSERT(tree->values.variations > 0 &&
				tree->values.variations <= VOE_3D_TREE_VARIATIONS,
			"a species of 1 to 16 variations");
}

voe_game_models_failures voe_game_trees_grow(voe_3d_models *models,
					     voe_render_device *device,
					     const voe_game_trees *trees,
					     voe_base_arena *scratch,
					     voe_game_progress *progress)
{
	voe_game_models_failures failures = { 0 };
	struct voe_base_arena_mark mark;
	struct tree_task *tasks;
	voe_base_error error;
	uint32_t total = 0;
	uint32_t next = 0;
	uint32_t row = 0;

	VOE_BASE_ASSERT(models != NULL && device != NULL, "no store or device");
	VOE_BASE_ASSERT(trees != NULL && scratch != NULL, "no trees or scratch");
	VOE_BASE_ASSERT(trees->rows != NULL || trees->count == 0,
			"trees with no rows");
	for (uint32_t r = 0; r < trees->count; r++) {
		tree_check(&trees->rows[r]);
		total += trees->rows[r].values.variations;
	}
	if (total == 0 || voe_game_progress_stopped(progress))
		return failures;
	mark = voe_base_arena_mark(scratch);
	tasks = voe_base_arena_push(scratch, total * sizeof(*tasks));
	for (uint32_t r = 0; r < trees->count; r++)
		for (uint32_t v = 0; v < trees->rows[r].values.variations; v++)
			task_begin(&tasks[next++], &trees->rows[r].values,
				   trees->rows[r].path, v);
	voe_game_progress_set(progress, "Growing trees", 0, total);
	voe_platform_burst_finish(voe_platform_burst_start(
		voe_platform_burst_workers(progress != NULL), total, grow_task,
		tasks));
	next = 0;
	for (; row < trees->count && !voe_game_progress_stopped(progress);
	     row++) {
		const voe_game_tree *tree = &trees->rows[row];

		for (uint32_t v = 0; v < tree->values.variations; v++, next++) {
			if (!task_put(&tasks[next], models, device, &error))
				voe_game_models_failure_count(&failures, models,
							      tasks[next].path);
			voe_game_progress_set(progress, "Growing trees",
					      next + 1, total);
		}
		if (!voe_3d_models_species_set(models, tree->path,
					       tree->values.variations))
			voe_game_models_failure_count(&failures, models,
						      tree->path);
	}
	// A stop leaves the rows after it grown and not put.
	for (; next < total; next++)
		voe_base_arena_destroy(tasks[next].arena);
	voe_base_arena_rewind(scratch, mark);
	return failures;
}

// One species' copy, its tasks, and how far its puts have gone.
struct voe_game_trees_job {
	voe_game_tree row;
	char path[VOE_ASSETS_LANDSCAPE_KIND_PATH];
	struct tree_task tasks[VOE_3D_TREE_VARIATIONS];
	// NULL once finished, or when nothing is started.
	voe_platform_burst *burst;
	// The tasks started, 0 when idle; the next one put.
	uint32_t count;
	uint32_t next;
};

voe_game_trees_job *voe_game_trees_job_new(void)
{
	voe_game_trees_job *job = calloc(1, sizeof(*job));

	VOE_BASE_ASSERT(job != NULL, "no room for a trees job");
	return job;
}

void voe_game_trees_job_destroy(voe_game_trees_job *job)
{
	if (job == NULL)
		return;
	if (job->burst != NULL)
		voe_platform_burst_finish(job->burst);
	for (uint32_t v = job->next; v < job->count; v++)
		voe_base_arena_destroy(job->tasks[v].arena);
	free(job);
}

bool voe_game_trees_job_start(voe_game_trees_job *job,
			      const voe_game_tree *tree)
{
	VOE_BASE_ASSERT(job != NULL, "starting no job");
	tree_check(tree);
	if (job->count != 0)
		return false;
	memcpy(job->path, tree->path, strlen(tree->path) + 1);
	job->row = (voe_game_tree){ .path = job->path, .values = tree->values };
	job->count = tree->values.variations;
	job->next = 0;
	for (uint32_t v = 0; v < job->count; v++)
		task_begin(&job->tasks[v], &job->row.values, job->path, v);
	job->burst = voe_platform_burst_start(voe_platform_burst_workers(false),
					      job->count, grow_task, job->tasks);
	return true;
}

bool voe_game_trees_job_done(const voe_game_trees_job *job)
{
	VOE_BASE_ASSERT(job != NULL, "polling no job");
	return job->count > 0 &&
	       (job->burst == NULL || voe_platform_burst_done(job->burst));
}

bool voe_game_trees_job_put(voe_game_trees_job *job, voe_3d_models *models,
			    voe_render_device *device, voe_base_error *error)
{
	VOE_BASE_ASSERT(models != NULL && device != NULL && error != NULL,
			"no store, device or error");
	VOE_BASE_ASSERT(voe_game_trees_job_done(job), "putting a job not done");
	if (job->burst != NULL) {
		voe_platform_burst_finish(job->burst);
		job->burst = NULL;
	}
	(void)task_put(&job->tasks[job->next++], models, device, error);
	if (job->next < job->count)
		return false;
	if (!voe_3d_models_species_set(models, job->path, job->count) &&
	    *error == VOE_BASE_OK)
		*error = VOE_BASE_ERROR_REFUSED;
	job->count = 0;
	job->next = 0;
	return true;
}
