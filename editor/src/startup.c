// The project the editor opens on, argued, remembered or untitled, and the
// descriptions line. What each means is startup.h's; this is the order they
// are tried in and what each failure says.
//
// Constraints: the remembered path is read in a scratch arena of its own,
// destroyed before this returns; nothing here outlives the call but the
// project and the session's notice.
#include "startup.h"

#include "last_project.h"
#include "notice.h"
#include "project.h"

#include <base/arena.h>
#include <base/report.h>

#include <stdio.h>

// The remembered path's working memory, kept by nothing.
#define LAST_SCRATCH (1u * 1024u * 1024u)

bool voe_editor_startup_project(const voe_editor_options *options,
				voe_editor_session *session)
{
	// Only ever this program's own words, for the one project a folder on
	// the command line named and could not open: this program stops right
	// there, before session->notice would ever be read, so a notice
	// destined for the bar would be one nobody could show.
	voe_editor_notice notice;

	// WHICH PROJECT OPENS, BEFORE THE DEVICE DOES: a folder argued on the
	// command line is opened or this program stops right here, on stderr,
	// before a window would ever have shown (criterion 12). With none, the
	// last project remembered (last_project.h) is tried the same way, but
	// its failure is not this program's to stop over — an untitled cube
	// and light is what a lost or broken last project falls back to, and
	// the notice explaining why goes into session->notice as well as onto
	// stderr, because the bar the session shows it in is up from the first
	// frame (criterion 11).
	if (options->folder != NULL) {
		session->project = voe_editor_project_new_opened(options->folder,
								 &notice);
		if (session->project == NULL) {
			fprintf(stderr, "voe_editor: %s\n", notice.text);
			return false;
		}
	} else {
		voe_base_arena *last_scratch = voe_base_arena_new(LAST_SCRATCH);
		const char *last = voe_editor_last_project_read(last_scratch);

		if (last != NULL) {
			session->project = voe_editor_project_new_opened(
				last, &session->notice);
			if (session->project == NULL) {
				fprintf(stderr, "voe_editor: %s\n",
					session->notice.text);
				session->project =
					voe_editor_project_new_untitled();
			}
		} else {
			session->project = voe_editor_project_new_untitled();
		}
		voe_base_arena_destroy(last_scratch);
	}

	// REMEMBERED FOR NEXT TIME, UNLESS THIS IS A CAPTURE OR THERE IS NO
	// FOLDER TO REMEMBER. An untitled project — whether this is a first
	// start or a last project that could not be opened — writes nothing
	// (criterion 11), and a capture draws what a start would have opened
	// without ever being the thing that decides what a start opens next.
	if (options->capture == NULL && session->project->folder != NULL &&
	    !voe_editor_last_project_write(session->project->folder))
		VOE_BASE_WARNING(
			"editor",
			"could not remember %s as the last project opened",
			session->project->folder);

	// A project with a folder may have code, built and loaded at the
	// first step (session.h).
	session->refresh_due = session->project->folder != NULL;
	return true;
}

// With descriptions off every inspector has nothing to expand and the program
// would simply look empty, so it says so at startup.
void voe_editor_startup_say_descriptions(void)
{
#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS
	printf("descriptions  compiled in\n");
#else
	printf("descriptions  off — nothing will be expandable; build with `cmake --preset editor` to turn them on\n");
#endif
}
