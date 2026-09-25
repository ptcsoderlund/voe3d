// The refresh's stages carried out: the tree written and the first step
// started at the start, each ended step polled on to the next or to BUILT or
// FAILED, and the arena destroyed whenever the refresh goes idle. See
// refresh.h for the stages and where a failed build's words are.
#include "refresh.h"

#include "game_tree.h"

#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>

#include <stdio.h>
#include <string.h>

// Room for the folder, the argument lists and the cook's working memory; a
// larger push gets a block of its own (base/arena.h).
#define VOE_EDITOR_REFRESH_ARENA (64u * 1024u)

// A refresh builds only the library; the game is Play's.
#define LIBRARY VOE_EDITOR_GAME_TREE_LIBRARY

// Destroys the arena and zeroes refresh. The process is already ended or
// zeroed.
static void refresh_idle(voe_editor_refresh *refresh)
{
	voe_base_arena_destroy(refresh->arena);
	*refresh = (voe_editor_refresh){ 0 };
}

// Starts argv as stage, its output appended to Build/build.log. False,
// reported by platform/process.h, when it cannot start.
static bool refresh_step(voe_editor_refresh *refresh,
			 voe_editor_refresh_stage stage, const char *const *argv)
{
	const char *log;

	VOE_BASE_ASSERT(argv != NULL && argv[0] != NULL,
			"a refresh step with no program");
	VOE_BASE_ASSERT(stage != VOE_EDITOR_REFRESH_IDLE,
			"a refresh step that is idle");

	log = voe_editor_game_tree_log(refresh->folder, refresh->arena);
	if (!voe_platform_process_start(argv, log, refresh->arena,
					&refresh->process))
		return false;
	refresh->stage = stage;
	return true;
}

// Build/build.log holding only its first line, so it tells of this refresh
// alone; platform/file.h asserts on a write of no bytes. False with why
// naming the log on a refused write.
static bool log_empty(const voe_editor_refresh *refresh, voe_editor_notice *why)
{
	static const char first[] = "voe_editor: building the project's code\n";
	const char *log = voe_editor_game_tree_log(refresh->folder, refresh->arena);

	VOE_BASE_ASSERT(why != NULL, "emptying the log with nowhere to say why");
	if (voe_platform_file_write(log, (const uint8_t *)first, sizeof first - 1,
				    NULL))
		return true;
	voe_editor_notice_from_report(why, log);
	return false;
}

void voe_editor_refresh_start(voe_editor_refresh *refresh,
			      const voe_editor_project *project,
			      voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	const char *const *argv;
	voe_editor_refresh_stage stage;
	size_t length;
	char *folder;

	VOE_BASE_ASSERT(refresh != NULL, "starting no refresh");
	VOE_BASE_ASSERT(refresh->stage == VOE_EDITOR_REFRESH_IDLE,
			"starting a refresh that is not idle");
	VOE_BASE_ASSERT(project != NULL, "refreshing no project");
	VOE_BASE_ASSERT(why != NULL, "refreshing with nowhere to say why");

	if (project->folder == NULL) {
		voe_editor_notice_set(
			why, "Save the project once before refreshing its code");
		return;
	}

	refresh->arena = voe_base_arena_new(VOE_EDITOR_REFRESH_ARENA);
	length = strlen(project->folder);
	folder = voe_base_arena_push(refresh->arena, length + 1);
	memcpy(folder, project->folder, length + 1);
	refresh->folder = folder;
	mark = voe_base_arena_mark(refresh->arena);

	if (!voe_editor_game_tree_write(project, refresh->arena, why) ||
	    !log_empty(refresh, why)) {
		refresh_idle(refresh);
		return;
	}
	voe_base_arena_rewind(refresh->arena, mark);

	if (voe_editor_game_tree_configured(refresh->folder, LIBRARY,
					    refresh->arena)) {
		stage = VOE_EDITOR_REFRESH_BUILDING;
		argv = voe_editor_game_tree_build(refresh->folder, LIBRARY,
						  refresh->arena);
	} else {
		stage = VOE_EDITOR_REFRESH_CONFIGURING;
		argv = voe_editor_game_tree_configure(refresh->folder, LIBRARY,
						      refresh->arena);
	}

	voe_base_report_error_clear();
	if (!refresh_step(refresh, stage, argv)) {
		voe_editor_notice_from_report(why, argv[0]);
		refresh_idle(refresh);
	}
}

voe_editor_refresh_result voe_editor_refresh_poll(voe_editor_refresh *refresh)
{
	const char *log;
	int code;

	VOE_BASE_ASSERT(refresh != NULL, "polling no refresh");
	VOE_BASE_ASSERT(refresh->stage != VOE_EDITOR_REFRESH_IDLE,
			"polling a refresh that is idle");

	if (voe_platform_process_poll(&refresh->process, &code) ==
	    VOE_PLATFORM_PROCESS_RUNNING)
		return VOE_EDITOR_REFRESH_RUNNING;

	log = voe_editor_game_tree_log(refresh->folder, refresh->arena);
	if (code != 0) {
		fprintf(stderr, "voe_editor: %s the project's code failed (exit %d), see %s\n",
			refresh->stage == VOE_EDITOR_REFRESH_CONFIGURING ?
				"configuring" : "building",
			code, log);
		refresh_idle(refresh);
		return VOE_EDITOR_REFRESH_FAILED;
	}
	if (refresh->stage == VOE_EDITOR_REFRESH_BUILDING) {
		refresh_idle(refresh);
		return VOE_EDITOR_REFRESH_BUILT;
	}
	if (refresh_step(refresh, VOE_EDITOR_REFRESH_BUILDING,
			 voe_editor_game_tree_build(refresh->folder, LIBRARY,
						    refresh->arena)))
		return VOE_EDITOR_REFRESH_RUNNING;
	fprintf(stderr,
		"voe_editor: building the project's code could not start, see %s\n",
		log);
	refresh_idle(refresh);
	return VOE_EDITOR_REFRESH_FAILED;
}

void voe_editor_refresh_end(voe_editor_refresh *refresh)
{
	VOE_BASE_ASSERT(refresh != NULL, "ending no refresh");

	if (refresh->stage == VOE_EDITOR_REFRESH_IDLE)
		return;
	voe_platform_process_end(&refresh->process);
	refresh_idle(refresh);
	VOE_BASE_ASSERT(refresh->arena == NULL, "an ended refresh kept its arena");
}

const char *voe_editor_refresh_label(const voe_editor_refresh *refresh)
{
	VOE_BASE_ASSERT(refresh != NULL, "labelling no refresh");

	return refresh->stage == VOE_EDITOR_REFRESH_IDLE ? "Refresh" :
							   "Refreshing";
}
