// The play's stages carried out: the tree written and the first step started at
// the press, each ended step polled on to the next, and the arena destroyed
// whenever play goes idle. See play.h for the stages and where a failed
// build's words are.
#include "play.h"

#include "game_tree.h"

#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>

#include <stdio.h>
#include <string.h>

// Room for the folder, the argument lists and the cook's working memory; a
// larger push gets a block of its own (base/arena.h).
#define VOE_EDITOR_PLAY_ARENA (64u * 1024u)

// Play builds only the game; the library is the refresh's.
#define GAME VOE_EDITOR_GAME_TREE_GAME

// Destroys the arena and zeroes play. The process is already ended or zeroed.
static void play_idle(voe_editor_play *play)
{
	voe_base_arena_destroy(play->arena);
	*play = (voe_editor_play){ 0 };
}

// Starts argv as stage, its output appended to Build/build.log unless it is
// the game, which shares the editor's. False, reported by platform/process.h
// naming argv[0] or the log, when it cannot start. An argument list stays in
// the arena until play goes idle: a few hundred bytes per step, three steps per
// press.
static bool play_step(voe_editor_play *play, voe_editor_play_stage stage,
		      const char *const *argv)
{
	const char *output = NULL;

	VOE_BASE_ASSERT(argv != NULL && argv[0] != NULL, "a play step with no program");
	if (stage != VOE_EDITOR_PLAY_RUNNING)
		output = voe_editor_game_tree_log(play->folder, play->arena);
	if (!voe_platform_process_start(argv, output, play->arena,
					&play->process))
		return false;
	play->stage = stage;
	return true;
}

// Build/build.log holding only its first line, so it tells of this press
// alone. That line is there because platform/file.h asserts on a write of no
// bytes. False with why naming the log on a refused write.
static bool log_empty(const voe_editor_play *play, voe_editor_notice *why)
{
	static const char first[] = "voe_editor: building the game\n";
	const char *log = voe_editor_game_tree_log(play->folder, play->arena);

	VOE_BASE_ASSERT(why != NULL, "emptying the log with nowhere to say why");
	if (voe_platform_file_write(log, (const uint8_t *)first, sizeof first - 1,
				    NULL))
		return true;
	voe_editor_notice_from_report(why, log);
	return false;
}

void voe_editor_play_start(voe_editor_play *play,
			   const voe_editor_project *project,
			   voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	const char *const *argv;
	voe_editor_play_stage stage;
	size_t length;
	char *folder;

	VOE_BASE_ASSERT(play != NULL, "starting no play");
	VOE_BASE_ASSERT(play->stage == VOE_EDITOR_PLAY_IDLE,
			"starting a play that is not idle");
	VOE_BASE_ASSERT(project != NULL, "playing no project");
	VOE_BASE_ASSERT(why != NULL, "playing with nowhere to say why");

	if (project->folder == NULL) {
		voe_editor_notice_set(why,
				      "Save the project once before playing it");
		return;
	}

	play->arena = voe_base_arena_new(VOE_EDITOR_PLAY_ARENA);
	length = strlen(project->folder);
	folder = voe_base_arena_push(play->arena, length + 1);
	memcpy(folder, project->folder, length + 1);
	play->folder = folder;
	mark = voe_base_arena_mark(play->arena);

	if (!voe_editor_game_tree_write(project, play->arena, why) ||
	    !log_empty(play, why)) {
		play_idle(play);
		return;
	}
	voe_base_arena_rewind(play->arena, mark);

	if (voe_editor_game_tree_configured(play->folder, GAME, play->arena)) {
		stage = VOE_EDITOR_PLAY_BUILDING;
		argv = voe_editor_game_tree_build(play->folder, GAME, play->arena);
	} else {
		stage = VOE_EDITOR_PLAY_CONFIGURING;
		argv = voe_editor_game_tree_configure(play->folder, GAME,
						      play->arena);
	}

	voe_base_report_error_clear();
	if (!play_step(play, stage, argv)) {
		voe_editor_notice_from_report(why, argv[0]);
		play_idle(play);
	}
}

void voe_editor_play_poll(voe_editor_play *play)
{
	int code;

	VOE_BASE_ASSERT(play != NULL, "polling no play");

	if (play->stage == VOE_EDITOR_PLAY_IDLE ||
	    voe_platform_process_poll(&play->process, &code) ==
		    VOE_PLATFORM_PROCESS_RUNNING)
		return;

	switch (play->stage) {
	case VOE_EDITOR_PLAY_CONFIGURING:
		if (code != 0) {
			fprintf(stderr,
				"voe_editor: configuring the game failed (exit %d), see %s\n",
				code, voe_editor_game_tree_log(play->folder, play->arena));
			break;
		}
		if (!play_step(play, VOE_EDITOR_PLAY_BUILDING,
			       voe_editor_game_tree_build(play->folder, GAME,
							  play->arena))) {
			fprintf(stderr,
				"voe_editor: building the game could not start\n");
			break;
		}
		return;

	case VOE_EDITOR_PLAY_BUILDING: {
		const char *program;

		if (code != 0) {
			fprintf(stderr,
				"voe_editor: building the game failed (exit %d), see %s\n",
				code, voe_editor_game_tree_log(play->folder, play->arena));
			break;
		}
		program = voe_editor_game_tree_program(play->folder,
						       play->arena);
		if (!play_step(play, VOE_EDITOR_PLAY_RUNNING,
			       (const char *const[]){ program, NULL })) {
			fprintf(stderr, "voe_editor: the game could not start\n");
			break;
		}
		return;
	}

	case VOE_EDITOR_PLAY_RUNNING:
	case VOE_EDITOR_PLAY_IDLE:
		break;
	}
	play_idle(play);
}

void voe_editor_play_end(voe_editor_play *play)
{
	VOE_BASE_ASSERT(play != NULL, "ending no play");

	if (play->stage == VOE_EDITOR_PLAY_IDLE)
		return;
	voe_platform_process_end(&play->process);
	play_idle(play);
}

const char *voe_editor_play_label(const voe_editor_play *play)
{
	VOE_BASE_ASSERT(play != NULL, "labelling no play");

	switch (play->stage) {
	case VOE_EDITOR_PLAY_CONFIGURING:
	case VOE_EDITOR_PLAY_BUILDING:
		return "Building";
	case VOE_EDITOR_PLAY_RUNNING:
		return "Stop";
	case VOE_EDITOR_PLAY_IDLE:
		break;
	}
	return "Play";
}
