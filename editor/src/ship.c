// The ship's stages carried out: the tree written and the first step started
// at the start, each ended step polled on to the next or to SHIPPED or FAILED,
// and the arena destroyed whenever the ship goes idle. See ship.h for the
// stages and where a failed step's words are.
#include "ship.h"

#include "game_tree.h"

#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>

#include <stdio.h>
#include <string.h>

// Room for the folder, the argument lists and the cook's working memory; a
// larger push gets a block of its own (base/arena.h).
#define VOE_EDITOR_SHIP_ARENA (64u * 1024u)

// A ship builds only the release; the game is Play's, the library Refresh's.
#define RELEASE VOE_EDITOR_GAME_TREE_RELEASE

// Destroys the arena and zeroes ship. The process is already ended or zeroed.
static void ship_idle(voe_editor_ship *ship)
{
	voe_base_arena_destroy(ship->arena);
	*ship = (voe_editor_ship){ 0 };
}

// What a stage does, for the stderr line that names it.
static const char *stage_doing(voe_editor_ship_stage stage)
{
	switch (stage) {
	case VOE_EDITOR_SHIP_CONFIGURING:
		return "configuring the release";
	case VOE_EDITOR_SHIP_BUILDING:
		return "building the release";
	case VOE_EDITOR_SHIP_CLEARING:
		return "clearing the shipped folder";
	case VOE_EDITOR_SHIP_INSTALLING:
		return "installing the shipped game";
	case VOE_EDITOR_SHIP_IDLE:
		break;
	}
	VOE_BASE_ASSERT(false, "naming an idle ship step");
	return "shipping";
}

// Starts argv as stage, its output appended to Build/build.log. False,
// reported by platform/process.h, when it cannot start.
static bool ship_step(voe_editor_ship *ship, voe_editor_ship_stage stage,
		      const char *const *argv)
{
	const char *log;

	VOE_BASE_ASSERT(argv != NULL && argv[0] != NULL,
			"a ship step with no program");
	VOE_BASE_ASSERT(stage != VOE_EDITOR_SHIP_IDLE, "a ship step that is idle");

	log = voe_editor_game_tree_log(ship->folder, ship->arena);
	if (!voe_platform_process_start(argv, log, ship->arena, &ship->process))
		return false;
	ship->stage = stage;
	return true;
}

// The step after an ended one, started. CLEARING only ever follows a build
// that ended 0, so a failed build keeps the old shipped folder. False when it
// cannot start.
static bool ship_next(voe_editor_ship *ship)
{
	const char *folder = ship->folder;

	VOE_BASE_ASSERT(ship->stage != VOE_EDITOR_SHIP_IDLE &&
				ship->stage != VOE_EDITOR_SHIP_INSTALLING,
			"no step after this one");

	switch (ship->stage) {
	case VOE_EDITOR_SHIP_CONFIGURING:
		return ship_step(ship, VOE_EDITOR_SHIP_BUILDING,
				 voe_editor_game_tree_build(folder, RELEASE,
							    ship->arena));
	case VOE_EDITOR_SHIP_BUILDING:
		return ship_step(ship, VOE_EDITOR_SHIP_CLEARING,
				 voe_editor_game_tree_ship_clear(folder,
								 ship->arena));
	default:
		return ship_step(ship, VOE_EDITOR_SHIP_INSTALLING,
				 voe_editor_game_tree_install(folder,
							      ship->arena));
	}
}

// Build/build.log holding only its first line, so it tells of this ship
// alone; platform/file.h asserts on a write of no bytes. False with why
// naming the log on a refused write.
static bool log_empty(const voe_editor_ship *ship, voe_editor_notice *why)
{
	static const char first[] = "voe_editor: shipping the game\n";
	const char *log = voe_editor_game_tree_log(ship->folder, ship->arena);

	VOE_BASE_ASSERT(why != NULL, "emptying the log with nowhere to say why");
	if (voe_platform_file_write(log, (const uint8_t *)first, sizeof first - 1,
				    NULL))
		return true;
	voe_editor_notice_from_report(why, log);
	return false;
}

void voe_editor_ship_start(voe_editor_ship *ship,
			   const voe_editor_project *project,
			   voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	const char *const *argv;
	voe_editor_ship_stage stage;
	size_t length;
	char *folder;

	VOE_BASE_ASSERT(ship != NULL, "starting no ship");
	VOE_BASE_ASSERT(ship->stage == VOE_EDITOR_SHIP_IDLE,
			"starting a ship that is not idle");
	VOE_BASE_ASSERT(project != NULL, "shipping no project");
	VOE_BASE_ASSERT(why != NULL, "shipping with nowhere to say why");

	if (project->folder == NULL) {
		voe_editor_notice_set(why,
				      "Save the project once before shipping it");
		return;
	}

	ship->arena = voe_base_arena_new(VOE_EDITOR_SHIP_ARENA);
	length = strlen(project->folder);
	folder = voe_base_arena_push(ship->arena, length + 1);
	memcpy(folder, project->folder, length + 1);
	ship->folder = folder;
	mark = voe_base_arena_mark(ship->arena);

	if (!voe_editor_game_tree_write(project, ship->arena, why) ||
	    !log_empty(ship, why)) {
		ship_idle(ship);
		return;
	}
	voe_base_arena_rewind(ship->arena, mark);

	if (voe_editor_game_tree_configured(ship->folder, RELEASE, ship->arena)) {
		stage = VOE_EDITOR_SHIP_BUILDING;
		argv = voe_editor_game_tree_build(ship->folder, RELEASE,
						  ship->arena);
	} else {
		stage = VOE_EDITOR_SHIP_CONFIGURING;
		argv = voe_editor_game_tree_configure(ship->folder, RELEASE,
						      ship->arena);
	}

	voe_base_report_error_clear();
	if (!ship_step(ship, stage, argv)) {
		voe_editor_notice_from_report(why, argv[0]);
		ship_idle(ship);
	}
}

voe_editor_ship_result voe_editor_ship_poll(voe_editor_ship *ship,
					    voe_editor_notice *told)
{
	const char *log;
	int code;

	VOE_BASE_ASSERT(ship != NULL, "polling no ship");
	VOE_BASE_ASSERT(ship->stage != VOE_EDITOR_SHIP_IDLE,
			"polling a ship that is idle");
	VOE_BASE_ASSERT(told != NULL, "polling a ship with nowhere to tell");

	if (voe_platform_process_poll(&ship->process, &code) ==
	    VOE_PLATFORM_PROCESS_RUNNING)
		return VOE_EDITOR_SHIP_RUNNING;

	log = voe_editor_game_tree_log(ship->folder, ship->arena);
	if (code != 0) {
		fprintf(stderr, "voe_editor: %s failed (exit %d), see %s\n",
			stage_doing(ship->stage), code, log);
		ship_idle(ship);
		return VOE_EDITOR_SHIP_FAILED;
	}
	if (ship->stage == VOE_EDITOR_SHIP_INSTALLING) {
		voe_editor_notice_set(told, "Shipped to %s",
				      voe_editor_game_tree_shipped(ship->folder,
								   ship->arena));
		ship_idle(ship);
		return VOE_EDITOR_SHIP_SHIPPED;
	}
	if (ship_next(ship))
		return VOE_EDITOR_SHIP_RUNNING;
	// ship_next failed to start, so stage is still the ended one; name the
	// step that could not start.
	fprintf(stderr, "voe_editor: %s could not start, see %s\n",
		stage_doing((voe_editor_ship_stage)(ship->stage + 1)), log);
	ship_idle(ship);
	VOE_BASE_ASSERT(ship->arena == NULL, "a failed ship kept its arena");
	return VOE_EDITOR_SHIP_FAILED;
}

void voe_editor_ship_end(voe_editor_ship *ship)
{
	VOE_BASE_ASSERT(ship != NULL, "ending no ship");

	if (ship->stage == VOE_EDITOR_SHIP_IDLE)
		return;
	voe_platform_process_end(&ship->process);
	ship_idle(ship);
	VOE_BASE_ASSERT(ship->arena == NULL, "an ended ship kept its arena");
}

const char *voe_editor_ship_label(const voe_editor_ship *ship)
{
	VOE_BASE_ASSERT(ship != NULL, "labelling no ship");

	return ship->stage == VOE_EDITOR_SHIP_IDLE ? "Ship" : "Shipping";
}
