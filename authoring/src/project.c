// The project file. See the header for what it holds and what it refuses.
//
// THE READER GOES THROUGH assets/sectioned.h AND NOTHING ELSE. A malformed file
// never reaches this code: voe_assets_sectioned_parse has already reported it,
// with a line, under its own module. Past that, this file's own refusals name
// the line the parser put on each section and key.
//
// A "SCENE PATH" IS CHECKED THE SAME WAY ON THE WAY IN AND THE WAY OUT, one
// function shared by the reader and the writer's assert, so the writer can
// never produce a path the reader refuses. The window's range is shared the
// same way; the sectioned reader hands back text, and the digits are
// converted here.
#include <authoring/project.h>


#include <assets/sectioned.h>
#include <base/assert.h>
#include <base/report.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MODULE "authoring"

// Relative to the project folder, `/`-separated, and unable to name anything
// outside it: not empty, not starting with `/`, no drive letter, no `\` or
// control character, and no `/`-separated segment that is empty, `.` or `..`.
static bool valid_scene_path(const char *scene)
{
	size_t length = strlen(scene);

	if (length == 0 || scene[0] == '/')
		return false;
	if (length >= 2 &&
	    ((scene[0] >= 'a' && scene[0] <= 'z') ||
	     (scene[0] >= 'A' && scene[0] <= 'Z')) &&
	    scene[1] == ':')
		return false;
	for (size_t i = 0; i < length; i++) {
		unsigned char c = (unsigned char)scene[i];

		if (c == '\\' || c < 0x20 || c == 0x7f)
			return false;
	}

	size_t start = 0;

	for (size_t i = 0; i <= length; i++) {
		if (i != length && scene[i] != '/')
			continue;

		size_t segment = i - start;

		if (segment == 0)
			return false;
		if (segment == 1 && scene[start] == '.')
			return false;
		if (segment == 2 && scene[start] == '.' && scene[start + 1] == '.')
			return false;
		start = i + 1;
	}
	return true;
}

static bool window_size_in_range(int value)
{
	return value >= VOE_AUTHORING_PROJECT_WINDOW_MIN &&
	       value <= VOE_AUTHORING_PROJECT_WINDOW_MAX;
}

// Whole decimal digits only, no sign, no space; more than five digits is out
// of range anyway, so the loop stops there and never overflows.
static bool parse_window_size(const char *text, int *out)
{
	size_t length = strlen(text);
	int value = 0;

	if (length == 0 || length > 5)
		return false;
	for (size_t i = 0; i < length; i++) {
		if (text[i] < '0' || text[i] > '9')
			return false;
		value = value * 10 + (text[i] - '0');
	}
	if (!window_size_in_range(value))
		return false;
	*out = value;
	return true;
}

// One `[window]` key onto `*window`. False, reported with the key's line,
// for an unknown key or a value not in its form.
static bool read_window_key(const voe_assets_sectioned_key *key,
			    voe_authoring_project_window *window)
{
	if (strcmp(key->name, "width") == 0 &&
	    parse_window_size(key->value, &window->width))
		return true;
	if (strcmp(key->name, "height") == 0 &&
	    parse_window_size(key->value, &window->height))
		return true;
	if (strcmp(key->name, "fullscreen") == 0) {
		if (strcmp(key->value, "true") == 0 ||
		    strcmp(key->value, "false") == 0) {
			window->fullscreen = key->value[0] == 't';
			return true;
		}
		VOE_BASE_ERROR(MODULE,
			       "line %u: fullscreen = %s is not true or false; "
			       "nothing was loaded",
			       key->line, key->value);
		return false;
	}
	if (strcmp(key->name, "width") == 0 ||
	    strcmp(key->name, "height") == 0) {
		VOE_BASE_ERROR(MODULE,
			       "line %u: %s = %s is not a whole number from %d "
			       "to %d; nothing was loaded",
			       key->line, key->name, key->value,
			       VOE_AUTHORING_PROJECT_WINDOW_MIN,
			       VOE_AUTHORING_PROJECT_WINDOW_MAX);
		return false;
	}
	VOE_BASE_ERROR(MODULE,
		       "line %u: [window] has no key %s; nothing was loaded",
		       key->line, key->name);
	return false;
}

bool voe_authoring_project_read(const char *text, size_t size,
				voe_base_arena *arena,
				voe_authoring_project *out)
{
	voe_assets_sectioned doc;

	VOE_BASE_ASSERT(text != NULL || size == 0, "reading NULL text");
	VOE_BASE_ASSERT(arena != NULL, "reading with a NULL arena");
	VOE_BASE_ASSERT(out != NULL, "nowhere to put the project");

	if (!voe_assets_sectioned_parse(text, size, arena, &doc))
		return false;

	// A section twice in one file is refused by
	// voe_assets_sectioned_parse, so each name is found at most once.
	const voe_assets_sectioned_section *project = NULL;
	voe_authoring_project_window window = {
		.width = VOE_AUTHORING_PROJECT_WINDOW_DEFAULT_WIDTH,
		.height = VOE_AUTHORING_PROJECT_WINDOW_DEFAULT_HEIGHT,
		.fullscreen = false,
	};

	for (uint32_t s = 0; s < doc.section_count; s++) {
		const voe_assets_sectioned_section *section = &doc.sections[s];

		if (strcmp(section->name, "project") == 0) {
			project = section;
			continue;
		}
		if (strcmp(section->name, "window") == 0) {
			for (uint32_t k = 0; k < section->key_count; k++) {
				if (!read_window_key(
					    &doc.keys[section->first_key + k],
					    &window))
					return false;
			}
			continue;
		}
		VOE_BASE_ERROR(MODULE,
			       "line %u: [%s] is not [project] or [window], "
			       "the only sections a project file holds; "
			       "nothing was loaded",
			       section->line, section->name);
		return false;
	}

	if (project == NULL) {
		VOE_BASE_ERROR(MODULE,
			       "no [project] section; this is not a project");
		return false;
	}

	const char *scene = NULL;
	uint32_t scene_line = 0;

	for (uint32_t k = 0; k < project->key_count; k++) {
		uint32_t index = project->first_key + k;
		const voe_assets_sectioned_key *key = &doc.keys[index];

		if (strcmp(key->name, "scene") == 0) {
			scene = key->value;
			scene_line = key->line;
			continue;
		}
		VOE_BASE_WARNING(MODULE,
				 "line %u: [project] has no key %s; the line "
				 "is ignored",
				 key->line, key->name);
	}

	if (scene == NULL) {
		VOE_BASE_ERROR(MODULE,
			       "line %u: [project] has no scene key; nothing "
			       "was loaded",
			       project->line);
		return false;
	}
	if (!valid_scene_path(scene)) {
		VOE_BASE_ERROR(MODULE,
			       "line %u: scene = \"%s\" is not a path relative "
			       "to the project folder, /-separated, and never "
			       "absolute or climbing out of it; nothing was "
			       "loaded",
			       scene_line, scene);
		return false;
	}

	*out = (voe_authoring_project){ .scene = scene, .window = window };
	return true;
}

const char *voe_authoring_project_write(const voe_authoring_project *project,
					voe_base_arena *arena,
					size_t *out_size)
{
	static const char prefix[] = "[project]\nscene = \"";
	static const char suffix[] = "\"\n";
	// Two five-digit numbers and "false" fit well inside this.
	enum { WINDOW_TEXT_MAX = 96 };

	VOE_BASE_ASSERT(project != NULL, "writing a NULL project");
	VOE_BASE_ASSERT(project->scene != NULL, "a project with no scene");
	VOE_BASE_ASSERT(valid_scene_path(project->scene),
			"a scene path voe_authoring_project_read would refuse");
	VOE_BASE_ASSERT(window_size_in_range(project->window.width) &&
				window_size_in_range(project->window.height),
			"a window size voe_authoring_project_read would refuse");
	VOE_BASE_ASSERT(out_size != NULL, "nowhere to put the size");

	size_t length = strlen(project->scene);
	size_t max = sizeof(prefix) - 1 + length * 2 + sizeof(suffix) - 1 +
		     WINDOW_TEXT_MAX + 1;
	char *text = voe_base_arena_push(arena, max);
	size_t size = 0;

	memcpy(text + size, prefix, sizeof(prefix) - 1);
	size += sizeof(prefix) - 1;
	for (size_t i = 0; i < length; i++) {
		char c = project->scene[i];

		if (c == '"' || c == '\\')
			text[size++] = '\\';
		text[size++] = c;
	}
	memcpy(text + size, suffix, sizeof(suffix) - 1);
	size += sizeof(suffix) - 1;

	int written = snprintf(text + size, WINDOW_TEXT_MAX + 1,
			       "\n[window]\nwidth = %d\nheight = %d\n"
			       "fullscreen = %s\n",
			       project->window.width, project->window.height,
			       project->window.fullscreen ? "true" : "false");

	VOE_BASE_ASSERT(written > 0 && written <= WINDOW_TEXT_MAX,
			"the [window] section outgrew its room");
	size += (size_t)written;

	*out_size = size;
	return text;
}
