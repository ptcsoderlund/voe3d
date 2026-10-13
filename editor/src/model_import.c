// Import's materials and parts for a `.glb`, and Import textures' pictures and
// maps: the file read and parsed, every path named and checked first, then the
// files written. See the header for the names, what each overwrites and what
// is refused.
#include "model_import.h"

#include <assets/image.h>
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
#define PNG_ENDING ".png"
#define ORM_TAIL "-orm"
// One cleaned name's room: glTF's, a material's `-<N>`, `-orm`, then a
// `-<N>` of up to ten digits.
#define NAME_ROOM (VOE_ASSETS_MODEL_NAME + 24)

// The cleaned names given so far, in order, in `room` slots on scratch.
struct names {
	char (*text)[NAME_ROOM];
	uint32_t count;
	uint32_t room;
};

static struct names names_on(voe_base_arena *scratch, uint32_t room)
{
	VOE_BASE_ASSERT(scratch != NULL, "names on no arena");
	return (struct names){
		.text = voe_base_arena_push(scratch, (size_t)room * NAME_ROOM),
		.room = room,
	};
}

// `name` cleaned into `out` of NAME_ROOM bytes: every byte outside
// `A–Z a–z 0–9 - _` made `_`, `<fallback><index>` for an empty one.
static void name_clean(const voe_assets_model_name *name, const char *fallback,
		       uint32_t index, char *out)
{
	size_t i = 0;

	VOE_BASE_ASSERT(name != NULL && out != NULL, "cleaning no name");
	if (name->text[0] == '\0') {
		snprintf(out, NAME_ROOM, "%s%u", fallback, index);
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

// `clean` given as the next name, or `clean-2`, `clean-3`… when taken. Of the
// count + 1 tries at most count are taken.
static const char *name_give(struct names *names, const char *clean)
{
	char *out;

	VOE_BASE_ASSERT(names != NULL && clean != NULL, "giving no name");
	VOE_BASE_ASSERT(names->count < names->room, "more names than room");
	out = names->text[names->count];
	snprintf(out, NAME_ROOM, "%s", clean);
	for (uint32_t n = 2; n <= names->count + 1 && name_taken(names, out); n++)
		snprintf(out, NAME_ROOM, "%s-%u", clean, n);
	VOE_BASE_ASSERT(!name_taken(names, out), "no free name in reach");
	names->count++;
	return out;
}

// Every glTF material's cleaned, unique name, in glTF order.
static struct names materials_named(const voe_assets_model *model,
				    voe_base_arena *scratch)
{
	struct names names = names_on(scratch, model->material_count);

	for (uint32_t i = 0; i < model->material_count; i++) {
		char clean[NAME_ROOM];

		name_clean(&model->materials[i].name, "material", i, clean);
		(void)name_give(&names, clean);
	}
	return names;
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

// `<folder>/<stem>-<name><ending>` into `out` of VOE_ASSETS_MATERIAL_PATH
// bytes. False with `why` set when it does not fit.
static bool beside_named(const char *glb, const char *name, const char *ending,
			 char *out, voe_editor_notice *why)
{
	struct place place = place_of(glb);
	int written = snprintf(out, VOE_ASSETS_MATERIAL_PATH, "%.*s/%.*s-%s%s",
			       place.folder, glb, place.stem_length, place.stem,
			       name, ending);

	if (written >= 0 && written < VOE_ASSETS_MATERIAL_PATH)
		return true;
	voe_editor_notice_set(why, "%s: %s%s's path would be %d bytes, %d or more", glb,
			      name, ending, written, VOE_ASSETS_MATERIAL_PATH);
	return false;
}

// Whether a file can quote `glb`'s folder; false with `why` set.
static bool glb_quotable(const char *glb, voe_editor_notice *why)
{
	if (strpbrk(glb, "\"\r\n") == NULL)
		return true;
	voe_editor_notice_set(why, "%s: a file cannot name a path holding \" or a line break",
			      glb);
	return false;
}

// Every material's `Assets/…` path into `parts`. False with `why` set when
// one does not fit or the path cannot be quoted.
static bool parts_named(const char *glb, const voe_assets_model *model,
			voe_base_arena *scratch, voe_assets_parts *parts,
			voe_editor_notice *why)
{
	struct names names;

	VOE_BASE_ASSERT(model != NULL && parts != NULL, "naming no parts");
	VOE_BASE_ASSERT(model->material_count <= VOE_ASSETS_PARTS,
			"naming more parts than a file holds");
	if (!glb_quotable(glb, why))
		return false;
	names = materials_named(model, scratch);
	for (uint32_t i = 0; i < model->material_count; i++)
		if (!beside_named(glb, names.text[i], MATERIAL_ENDING, parts->materials[i],
				  why))
			return false;
	parts->count = model->material_count;
	return true;
}

// `bytes` written to `<project_folder>/<path>`. False with `why` naming `path`.
static bool bytes_written(const char *project_folder, const char *path,
			  const uint8_t *bytes, size_t count, voe_base_arena *scratch,
			  voe_editor_notice *why)
{
	VOE_BASE_ASSERT(path != NULL && scratch != NULL, "writing to no path");
	VOE_BASE_ASSERT(bytes != NULL && count > 0, "writing no bytes");
	voe_base_report_error_clear();
	if (voe_platform_file_write(voe_platform_path_join(scratch, project_folder, path),
				    bytes, count, NULL))
		return true;
	voe_editor_notice_from_report(why, path);
	return false;
}

static bool text_written(const char *project_folder, const char *path,
			 voe_assets_material_text text, voe_base_arena *scratch,
			 voe_editor_notice *why)
{
	return bytes_written(project_folder, path, (const uint8_t *)text.text, text.size,
			     scratch, why);
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

// `glb`'s `<stem>.parts` path, on scratch.
static const char *parts_path_of(const char *glb, voe_base_arena *scratch)
{
	struct place place = place_of(glb);
	size_t size = strlen(glb) + sizeof PARTS_ENDING;
	char *path = voe_base_arena_push(scratch, size);

	snprintf(path, size, "%.*s/%.*s" PARTS_ENDING, place.folder, glb,
		 place.stem_length, place.stem);
	return path;
}

bool voe_editor_model_import(const char *project_folder, const char *glb,
			     voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	voe_assets_model model;
	voe_assets_parts parts = { 0 };
	bool ok = false;

	VOE_BASE_ASSERT(project_folder != NULL && glb != NULL, "importing no .glb");
	VOE_BASE_ASSERT(scratch != NULL && why != NULL, "importing with no scratch or why");
	mark = voe_base_arena_mark(scratch);
	if (!model_read(project_folder, glb, scratch, &model, why) ||
	    !parts_named(glb, &model, scratch, &parts, why))
		goto rewind;
	for (uint32_t i = 0; i < parts.count; i++) {
		voe_assets_material_file values =
			voe_assets_model_material_values(&model.materials[i]);

		if (!text_written(project_folder, parts.materials[i],
				  voe_assets_material_write(&values, scratch), scratch, why))
			goto rewind;
	}
	ok = text_written(project_folder, parts_path_of(glb, scratch),
			  voe_assets_parts_write(&parts, scratch), scratch, why);
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}

// Whether `name` ends `ending`, exactly.
static bool ends_with(const char *name, const char *ending)
{
	size_t length = strlen(name);
	size_t tail = strlen(ending);

	return length >= tail && strcmp(name + length - tail, ending) == 0;
}

// `glb`'s `.parts` read into `parts`. False with `why` set when there is none,
// it will not read, or it names a file that is not a `.material`.
static bool parts_read(const char *project_folder, const char *glb,
		       voe_base_arena *scratch, voe_assets_parts *parts,
		       voe_editor_notice *why)
{
	const char *path = parts_path_of(glb, scratch);
	const char *full = voe_platform_path_join(scratch, project_folder, path);
	const uint8_t *bytes;
	size_t count = 0;

	if (!voe_platform_file_exists(full)) {
		voe_editor_notice_set(why, "%s: no %s. Import the model first", glb, path);
		return false;
	}
	voe_base_report_error_clear();
	bytes = voe_platform_file_read(full, scratch, &count, NULL);
	if (bytes == NULL || !voe_assets_parts_read((const char *)bytes, count, parts, NULL)) {
		voe_editor_notice_from_report(why, path);
		return false;
	}
	for (uint32_t i = 0; i < parts->count; i++)
		if (parts->materials[i][0] != '\0' &&
		    !ends_with(parts->materials[i], MATERIAL_ENDING)) {
			voe_editor_notice_set(why, "%s: material%u names %s, not a .material",
					      path, i, parts->materials[i]);
			return false;
		}
	return true;
}

// Every picture's and every ORM picture's `Assets/…` path, image_count and
// material_count long; "" for a material with no ORM picture.
struct pictures {
	char (*images)[VOE_ASSETS_MATERIAL_PATH];
	char (*orms)[VOE_ASSETS_MATERIAL_PATH];
};

// Whether voe_assets_model_orm makes a picture for `material`.
static bool orm_held(const voe_assets_material *material)
{
	return material->occlusion_image != VOE_ASSETS_MODEL_NONE ||
	       material->metallic_roughness_image != VOE_ASSETS_MODEL_NONE;
}

// Every picture named, images first so each keeps its own name and an ORM
// picture whose name an image took is the one made `-2`. False with `why`
// set when a path does not fit.
static bool pictures_named(const char *glb, const voe_assets_model *model,
			   voe_base_arena *scratch, struct pictures *pictures,
			   voe_editor_notice *why)
{
	struct names materials = materials_named(model, scratch);
	struct names given = names_on(scratch, model->image_count + model->material_count);
	char clean[NAME_ROOM];

	pictures->images = voe_base_arena_push(
		scratch, (size_t)model->image_count * VOE_ASSETS_MATERIAL_PATH);
	pictures->orms = voe_base_arena_push(
		scratch, (size_t)model->material_count * VOE_ASSETS_MATERIAL_PATH);
	for (uint32_t i = 0; i < model->image_count; i++) {
		name_clean(&model->image_names[i], "image", i, clean);
		if (!beside_named(glb, name_give(&given, clean), PNG_ENDING,
				  pictures->images[i], why))
			return false;
	}
	for (uint32_t i = 0; i < model->material_count; i++) {
		pictures->orms[i][0] = '\0';
		if (!orm_held(&model->materials[i]))
			continue;
		snprintf(clean, NAME_ROOM, "%s" ORM_TAIL, materials.text[i]);
		if (!beside_named(glb, name_give(&given, clean), PNG_ENDING,
				  pictures->orms[i], why))
			return false;
	}
	return true;
}

// `image` encoded as a PNG and written to `path`, scratch rewound after.
static bool picture_written(const char *project_folder, const char *path,
			    voe_assets_image image, voe_base_arena *scratch,
			    voe_editor_notice *why)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
	voe_assets_bytes png;
	bool ok = false;

	if (!voe_assets_png_encode(scratch, image, &png, NULL))
		voe_editor_notice_set(why, "%s: a picture too large to write", path);
	else
		ok = bytes_written(project_folder, path, png.bytes, png.count, scratch, why);
	voe_base_arena_rewind(scratch, mark);
	return ok;
}

// Every picture and every ORM picture written. False with `why` naming the
// first that fails.
static bool pictures_written(const char *project_folder, const voe_assets_model *model,
			     const struct pictures *pictures, voe_base_arena *scratch,
			     voe_editor_notice *why)
{
	for (uint32_t i = 0; i < model->image_count; i++)
		if (!picture_written(project_folder, pictures->images[i], model->images[i],
				     scratch, why))
			return false;
	for (uint32_t i = 0; i < model->material_count; i++) {
		struct voe_base_arena_mark mark = voe_base_arena_mark(scratch);
		voe_assets_image orm;
		bool ok;

		if (pictures->orms[i][0] == '\0')
			continue;
		ok = voe_assets_model_orm(model, i, scratch, &orm);
		VOE_BASE_ASSERT(ok, "an ORM picture named but not made");
		ok = picture_written(project_folder, pictures->orms[i], orm, scratch, why);
		voe_base_arena_rewind(scratch, mark);
		if (!ok)
			return false;
	}
	return true;
}

// `map` set to image `image`'s path, kept when the glTF material has none.
static void map_set(char *map, uint32_t image, const voe_assets_model *model,
		    const struct pictures *pictures)
{
	if (image == VOE_ASSETS_MODEL_NONE)
		return;
	VOE_BASE_ASSERT(image < model->image_count, "a material's picture out of range");
	memcpy(map, pictures->images[image], VOE_ASSETS_MATERIAL_PATH);
}

// Material `index`'s file at `path` read, or glTF's values when it is gone,
// its four maps set to the pictures written, and written back.
static bool material_dressed(const char *project_folder, const char *path,
			     const voe_assets_model *model, uint32_t index,
			     const struct pictures *pictures, voe_base_arena *scratch,
			     voe_editor_notice *why)
{
	const voe_assets_material *gltf = &model->materials[index];
	const char *full = voe_platform_path_join(scratch, project_folder, path);
	voe_assets_material_file values = voe_assets_model_material_values(gltf);

	if (voe_platform_file_exists(full)) {
		size_t count = 0;
		const uint8_t *bytes;

		voe_base_report_error_clear();
		bytes = voe_platform_file_read(full, scratch, &count, NULL);
		if (bytes == NULL ||
		    !voe_assets_material_read((const char *)bytes, count, &values, NULL)) {
			voe_editor_notice_from_report(why, path);
			return false;
		}
	}
	map_set(values.colormap, gltf->base_colour_image, model, pictures);
	map_set(values.normalmap, gltf->normal_image, model, pictures);
	map_set(values.emissionmap, gltf->emissive_image, model, pictures);
	if (pictures->orms[index][0] != '\0')
		memcpy(values.ormmap, pictures->orms[index], VOE_ASSETS_MATERIAL_PATH);
	return text_written(project_folder, path, voe_assets_material_write(&values, scratch),
			    scratch, why);
}

bool voe_editor_model_import_textures(const char *project_folder, const char *glb,
				      voe_base_arena *scratch, voe_editor_notice *why)
{
	struct voe_base_arena_mark mark;
	voe_assets_model model;
	voe_assets_parts parts = { 0 };
	struct pictures pictures;
	uint32_t count;
	bool ok = false;

	VOE_BASE_ASSERT(project_folder != NULL && glb != NULL, "dressing no .glb");
	VOE_BASE_ASSERT(scratch != NULL && why != NULL, "dressing with no scratch or why");
	mark = voe_base_arena_mark(scratch);
	if (!glb_quotable(glb, why) || !model_read(project_folder, glb, scratch, &model, why) ||
	    !parts_read(project_folder, glb, scratch, &parts, why) ||
	    !pictures_named(glb, &model, scratch, &pictures, why) ||
	    !pictures_written(project_folder, &model, &pictures, scratch, why))
		goto rewind;
	// A re-export with fewer materials leaves keys past the file's: no
	// glTF material there to dress them from.
	count = parts.count < model.material_count ? parts.count : model.material_count;
	for (uint32_t i = 0; i < count; i++)
		if (parts.materials[i][0] != '\0' &&
		    !material_dressed(project_folder, parts.materials[i], &model, i,
				      &pictures, scratch, why))
			goto rewind;
	ok = true;
rewind:
	voe_base_arena_rewind(scratch, mark);
	return ok;
}
