// Import's materials and parts for a `.glb`: the file read and parsed, every
// material's path named and checked first, then each `.material` and the
// `.parts` written. See the header for the names, why both are overwritten
// and what is refused.
#include "model_import.h"

#include <assets/model.h>
#include <assets/model_material.h>
#include <assets/parts.h>
#include <base/assert.h>
#include <base/report.h>
#include <platform/file.h>
#include <platform/path.h>

#include <stdio.h>
#include <string.h>

#define GLB_ENDING ".glb"
#define MATERIAL_ENDING ".material"
#define PARTS_ENDING ".parts"
// One material's cleaned name: glTF's room, then a `-<N>` of two digits.
#define NAME_ROOM (VOE_ASSETS_MODEL_NAME + 4)

// The cleaned names given so far, in glTF order.
struct names {
	char text[VOE_ASSETS_PARTS][NAME_ROOM];
	uint32_t count;
};

// `name` cleaned into `out` of NAME_ROOM bytes: every byte outside
// `A–Z a–z 0–9 - _` made `_`, `material<index>` for an empty one.
static void name_clean(const voe_assets_model_name *name, uint32_t index,
		       char *out)
{
	size_t i = 0;

	VOE_BASE_ASSERT(name != NULL && out != NULL, "cleaning no name");
	if (name->text[0] == '\0') {
		snprintf(out, NAME_ROOM, "material%u", index);
		return;
	}
	for (; i < VOE_ASSETS_MODEL_NAME - 1 && name->text[i] != '\0'; i++) {
		char c = name->text[i];
		bool kept = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			    (c >= '0' && c <= '9') || c == '-' || c == '_';

		out[i] = kept ? c : '_';
	}
	out[i] = '\0';
	VOE_BASE_ASSERT(out[0] != '\0', "a cleaned name left empty");
}

// `c` in lower case when it is an ASCII capital; a cleaned name holds no other.
static char lower(char c)
{
	return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
}

// Whether `name` equals one already given, in any case.
static bool name_taken(const struct names *names, const char *name)
{
	VOE_BASE_ASSERT(names != NULL && name != NULL, "looking for no name");
	for (uint32_t n = 0; n < names->count; n++) {
		const char *a = names->text[n];
		size_t k = 0;

		while (k < NAME_ROOM - 1 && a[k] != '\0' && lower(a[k]) == lower(name[k]))
			k++;
		if (a[k] == '\0' && name[k] == '\0')
			return true;
	}
	return false;
}

// `clean` given as the next name, or `clean-2`, `clean-3`… when taken. Of
// VOE_ASSETS_PARTS + 1 tries at most VOE_ASSETS_PARTS are taken.
static void name_give(struct names *names, const char *clean)
{
	char *out;

	VOE_BASE_ASSERT(names != NULL && clean != NULL, "giving no name");
	VOE_BASE_ASSERT(names->count < VOE_ASSETS_PARTS, "more names than parts");
	out = names->text[names->count];
	snprintf(out, NAME_ROOM, "%s", clean);
	for (uint32_t n = 2; n <= VOE_ASSETS_PARTS + 1 && name_taken(names, out);
	     n++)
		snprintf(out, NAME_ROOM, "%s-%u", clean, n);
	VOE_BASE_ASSERT(!name_taken(names, out), "no free name in reach");
	names->count++;
}

// `glb`'s folder, `/` excluded, and its stem, `.glb` excluded, as lengths
// for `%.*s`.
struct place {
	int folder;
	const char *stem;
	int stem_length;
};

static struct place place_of(const char *glb)
{
	const char *slash = strrchr(glb, '/');
	size_t name;

	VOE_BASE_ASSERT(slash != NULL, "a .glb outside Assets/");
	name = strlen(slash + 1);
	VOE_BASE_ASSERT(name >= sizeof GLB_ENDING - 1, "a .glb with no ending");
	return (struct place){
		.folder = (int)(slash - glb),
		.stem = slash + 1,
		.stem_length = (int)(name - (sizeof GLB_ENDING - 1)),
	};
}

// Every material's `Assets/…` path into `parts`. False with `why` set when
// one does not fit or the path cannot be quoted.
static bool parts_named(const char *glb, const voe_assets_model *model,
			voe_assets_parts *parts, voe_editor_notice *why)
{
	struct place place = place_of(glb);
	struct names names = { 0 };

	VOE_BASE_ASSERT(model != NULL && parts != NULL, "naming no parts");
	VOE_BASE_ASSERT(model->material_count <= VOE_ASSETS_PARTS,
			"naming more parts than a file holds");
	if (strpbrk(glb, "\"\r\n") != NULL) {
		voe_editor_notice_set(why, "%s: a parts file cannot name a path holding \" or a line break",
				      glb);
		return false;
	}
	for (uint32_t i = 0; i < model->material_count; i++) {
		char clean[NAME_ROOM];
		int written;

		name_clean(&model->materials[i].name, i, clean);
		name_give(&names, clean);
		written = snprintf(parts->materials[i], VOE_ASSETS_MATERIAL_PATH,
				   "%.*s/%.*s-%s" MATERIAL_ENDING, place.folder, glb,
				   place.stem_length, place.stem, names.text[i]);
		if (written < 0 || written >= VOE_ASSETS_MATERIAL_PATH) {
			voe_editor_notice_set(why, "%s: material %u's path would be %d bytes, %d or more",
					      glb, i, written, VOE_ASSETS_MATERIAL_PATH);
			return false;
		}
	}
	parts->count = model->material_count;
	return true;
}

// `text` written to `<project_folder>/<path>`. False with `why` naming `path`.
static bool text_written(const char *project_folder, const char *path,
			 voe_assets_material_text text, voe_base_arena *scratch,
			 voe_editor_notice *why)
{
	VOE_BASE_ASSERT(path != NULL && scratch != NULL, "writing to no path");
	VOE_BASE_ASSERT(text.text != NULL && text.size > 0, "writing no text");
	voe_base_report_error_clear();
	if (voe_platform_file_write(voe_platform_path_join(scratch, project_folder, path),
				    (const uint8_t *)text.text, text.size, NULL))
		return true;
	voe_editor_notice_from_report(why, path);
	return false;
}

// `glb` read from the project and parsed into `model` on scratch. False with
// `why` naming it.
static bool model_read(const char *project_folder, const char *glb,
		       voe_base_arena *scratch, voe_assets_model *model,
		       voe_editor_notice *why)
{
	const uint8_t *bytes;
	size_t count = 0;

	VOE_BASE_ASSERT(model != NULL && why != NULL, "reading into no model");
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(voe_platform_path_join(scratch, project_folder, glb),
				       scratch, &count, NULL);
	if (bytes == NULL || !voe_assets_model_read_glb(bytes, count, scratch, model, NULL)) {
		voe_editor_notice_from_report(why, glb);
		return false;
	}
	if (model->material_count <= VOE_ASSETS_PARTS)
		return true;
	voe_editor_notice_set(why, "%s: %u materials, more than the %d a parts file names", glb,
			      model->material_count, VOE_ASSETS_PARTS);
	return false;
}

bool voe_editor_model_import(const char *project_folder, const char *glb,
			     voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	voe_assets_model model;
	voe_assets_parts parts = { 0 };
	struct place place;
	size_t size;
	char *parts_path;
	bool ok = false;

	VOE_BASE_ASSERT(project_folder != NULL && glb != NULL, "importing no .glb");
	VOE_BASE_ASSERT(scratch != NULL && why != NULL, "importing with no scratch or why");
	mark = voe_base_arena_mark(scratch);
	if (!model_read(project_folder, glb, scratch, &model, why) ||
	    !parts_named(glb, &model, &parts, why))
		goto rewind;
	for (uint32_t i = 0; i < parts.count; i++) {
		voe_assets_material_file values =
			voe_assets_model_material_values(&model.materials[i]);

		if (!text_written(project_folder, parts.materials[i],
				  voe_assets_material_write(&values, scratch), scratch, why))
			goto rewind;
	}
	place = place_of(glb);
	size = strlen(glb) + sizeof PARTS_ENDING;
	parts_path = voe_base_arena_push(scratch, size);
	VOE_BASE_ASSERT(parts_path != NULL, "no room for a parts file's path");
	snprintf(parts_path, size, "%.*s/%.*s" PARTS_ENDING, place.folder, glb,
		 place.stem_length, place.stem);
	ok = text_written(project_folder, parts_path, voe_assets_parts_write(&parts, scratch),
			  scratch, why);
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}
