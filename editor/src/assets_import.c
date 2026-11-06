// Import (0277 point 7, 0399 point 8): the file at `source` copied into the
// Assets panel's shown folder under its own name, read whole and written
// atomically over any file of that name, then the panel listed again. session.c
// calls it with the file the browser's IMPORT mode chose:
//
//     voe_editor_assets_import(&scene->assets, project->folder, source,
//                              &session->notice, imported);
//
// READ BEFORE ANYTHING IS MADE, so a source that fails, or is empty, leaves the
// project as it was; `Assets/` is made only then, when the project has none.
// The shown folder is the panel's only when it was listed for this project,
// else the copy goes to `Assets/` itself.
//
// A `.glb` COPIED has its materials and parts written beside it
// (model_import.h). A refusal of theirs is said in why once and the `.glb`
// stays copied and listed, `imported` left "" (0445 point 4).
//
// Constraints: the whole file is held in one scratch arena while it is copied,
// so a file larger than memory cannot be imported; a streamed copy would lift
// it. The listing calls and the panel's state are assets_panel.c's.
#include "assets_panel.h"

#include "model_import.h"

#include <base/arena.h>
#include <base/assert.h>
#include <base/report.h>

#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <stdio.h>

// A block size, not a limit, as assets_panel.c's scratch is.
#define IMPORT_SCRATCH (64u * 1024u)
#define ASSETS_FOLDER "Assets"

// Makes `<project>/Assets/` when the project's listing has none. False with
// why naming the folder when the listing or the make fails.
static bool assets_made(const char *project, voe_base_arena *scratch,
			voe_editor_notice *why)
{
	voe_platform_folder_listing listing;
	const char *folder;
	bool missing;

	VOE_BASE_ASSERT(project != NULL && scratch != NULL, "making no Assets/");
	VOE_BASE_ASSERT(why != NULL, "making Assets/ with nowhere to say why");
	if (!voe_editor_assets_list_shown(project, "", scratch, &listing,
					  &missing)) {
		voe_editor_notice_from_report(why, project);
		return false;
	}
	if (!missing)
		return true;
	folder = voe_platform_path_join(scratch, project, ASSETS_FOLDER);
	voe_base_report_error_clear();
	if (!voe_platform_folder_create(folder, NULL)) {
		voe_editor_notice_from_report(why, folder);
		return false;
	}
	return true;
}

void voe_editor_assets_import(voe_editor_assets *assets,
			      const char *project_folder, const char *source,
			      voe_editor_notice *why,
			      char imported[VOE_3D_MODEL_PATH])
{
	voe_base_arena *scratch;
	const char *shown;
	const char *target;
	const char *relative;
	const uint8_t *bytes;
	size_t count;
	int length;

	VOE_BASE_ASSERT(assets != NULL && assets->arena != NULL,
			"importing into an Assets panel never listed");
	VOE_BASE_ASSERT(project_folder != NULL && source != NULL,
			"importing with no project folder or no file");
	VOE_BASE_ASSERT(why != NULL && imported != NULL,
			"importing with nowhere to say why or where");
	imported[0] = '\0';

	// The shown folder is only this project's when it was listed for it.
	shown = voe_editor_assets_same_folder(project_folder,
					      assets->project) &&
				!assets->missing ?
			assets->shown :
			"";
	// Read before anything is made, so a source that fails leaves the
	// project as it was.
	scratch = voe_base_arena_new(IMPORT_SCRATCH);
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(source, scratch, &count, NULL);
	if (bytes == NULL) {
		voe_editor_notice_from_report(why, source);
		goto destroy_scratch;
	}
	// A write refuses no bytes (platform/file.h), so an empty file is
	// refused here, naming it, rather than asserting there.
	if (count == 0) {
		voe_editor_notice_set(why, "%s: is empty", source);
		goto destroy_scratch;
	}
	if (!assets_made(project_folder, scratch, why))
		goto destroy_scratch;
	target = voe_platform_path_join(scratch, project_folder, ASSETS_FOLDER);
	if (shown[0] != '\0')
		target = voe_platform_path_join(scratch, target, shown);
	target = voe_platform_path_join(scratch, target,
					voe_platform_path_name(source));

	voe_base_report_error_clear();
	if (!voe_platform_file_write(target, bytes, count, NULL)) {
		voe_editor_notice_from_report(why, target);
		goto destroy_scratch;
	}
	relative = voe_editor_assets_join(
		scratch, ASSETS_FOLDER,
		voe_editor_assets_join(scratch, shown,
				       voe_platform_path_name(source)));
	length = snprintf(imported, VOE_3D_MODEL_PATH, "%s", relative);
	if (length < 0 || length >= VOE_3D_MODEL_PATH)
		imported[0] = '\0';
	// A refusal has said why once and the `.glb` stays copied and listed;
	// `imported` left "" so no LODs question, prepare or reload reads it
	// again to say the same (0445 point 4).
	if (voe_editor_assets_name_ends(source, ".glb") &&
	    !voe_editor_model_import(project_folder, relative, scratch, why))
		imported[0] = '\0';
	voe_editor_assets_relist(assets, project_folder, shown);
destroy_scratch:
	voe_base_arena_destroy(scratch);
}
