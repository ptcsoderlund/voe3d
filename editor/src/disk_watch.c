// The disk watch disk_watch.h describes. The frame owns the known table,
// sorted by path, and the changes held; a look owns its arena, the snapshot of
// the known stamps in it and the list of files it found, and the frame touches
// none of those until the look's burst is finished. Bytes, landscapes and
// error texts are malloced on the worker, so taking a look moves pointers.
#include "disk_watch.h"

#include <base/assert.h>
#include <base/report.h>
#include <platform/burst.h>
#include <platform/file.h>
#include <platform/folder.h>
#include <platform/path.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define LOOK_SECONDS 1.0
// How many folder levels are walked, the project folder one, as assets_walk.c.
#define WALK_DEPTH 16
#define PROJECT_FILE "project.voe3d"
#define LANDSCAPE_ENDING ".landscape"
// A look's arena block and a landscape's.
#define BLOCK_SIZE (64 * 1024)
// The frame's own scratch block, which a re-read file needs.
#define SCRATCH_SIZE (256 * 1024)

// assets_walk.c's list of walked extensions, and its skipped top-level folders.
static const char *const watched_extensions[] = { ".scene", ".prefab", ".material",
						   LANDSCAPE_ENDING, ".parts" };
static const char *const skipped_folders[] = { "Build", "Cache", "Code" };

// A path known: its stamp at the last look, settle or write, and but for a
// landscape its bytes then, malloced. `seen` and `wrote` are a look's marks.
struct known {
	char *path;
	uint64_t stamp;
	uint8_t *bytes;
	size_t size;
	bool seen;
	bool wrote;
};

// A known path and stamp as the look began. The path is the known row's own,
// which nothing frees or writes until the look is taken.
struct snapshot {
	const char *path;
	uint64_t stamp;
};

// One file the look stamped: whether its stamp is unlike the known one (or it
// is new), and what was read when so, all but `path` malloced.
struct found {
	struct found *next;
	const char *path;
	uint64_t stamp;
	bool changed;
	bool read;
	uint8_t *bytes;
	size_t size;
	voe_base_arena *memory;
	voe_assets_landscape *landscape;
	char *error;
};

struct look {
	voe_base_arena *memory;
	const char *folder;
	const struct snapshot *known;
	uint32_t known_count;
	struct found *first;
	struct found *last;
	bool complete;
	bool deep_said;
};

struct voe_editor_disk_watch {
	char *folder;
	double looked;
	struct known *known;
	uint32_t known_count;
	uint32_t known_room;
	voe_platform_burst *burst;
	struct look look;
	voe_editor_disk_change *changes;
	uint32_t change_count;
	uint32_t change_room;
	voe_base_arena *scratch;
};

struct walk_level {
	const char *relative;
	voe_platform_folder_listing listing;
	uint32_t next;
};

// Whether name ends in ending, any case, and is longer.
static bool ends_in(const char *name, const char *ending)
{
	size_t length = strlen(name);
	size_t tail = strlen(ending);

	VOE_BASE_ASSERT(tail > 1 && ending[0] == '.', "an ending without its dot");
	if (length <= tail)
		return false;
	for (size_t i = 0; i < tail; i++)
		if (tolower((unsigned char)name[length - tail + i]) != ending[i])
			return false;
	return true;
}

// Whether a file named `name` is watched, `top` when in the project folder.
static bool watched_named(const char *name, bool top)
{
	VOE_BASE_ASSERT(name != NULL, "no file name");
	if (top && strcmp(name, PROJECT_FILE) == 0)
		return true;
	for (size_t e = 0; e < sizeof watched_extensions / sizeof *watched_extensions; e++)
		if (ends_in(name, watched_extensions[e]))
			return true;
	return false;
}

static bool skipped_named(const char *name)
{
	VOE_BASE_ASSERT(name != NULL, "no folder name");
	for (size_t i = 0; i < sizeof skipped_folders / sizeof *skipped_folders; i++)
		if (strcmp(name, skipped_folders[i]) == 0)
			return true;
	return false;
}

static char *text_copy(const char *text)
{
	size_t size = strlen(text) + 1;
	char *out = malloc(size);

	VOE_BASE_ASSERT(out != NULL, "no memory for a watched text");
	memcpy(out, text, size);
	return out;
}

// `size` bytes and the NUL after them, malloced.
static uint8_t *bytes_copy(const uint8_t *bytes, size_t size)
{
	uint8_t *out = malloc(size + 1);

	VOE_BASE_ASSERT(out != NULL && bytes != NULL, "no memory for a watched file");
	memcpy(out, bytes, size);
	out[size] = '\0';
	return out;
}

// "<folder>/<name>", or name alone under the project folder, into arena.
static const char *relative_join(voe_base_arena *arena, const char *folder, const char *name)
{
	size_t head = strlen(folder);
	size_t tail = strlen(name);
	char *out;

	VOE_BASE_ASSERT(tail > 0, "joining no name");
	if (head == 0)
		return name;
	out = voe_base_arena_push(arena, head + 1 + tail + 1);
	memcpy(out, folder, head);
	out[head] = '/';
	memcpy(out + head + 1, name, tail + 1);
	return out;
}

static int known_order(const void *a, const void *b)
{
	return strcmp(((const struct known *)a)->path, ((const struct known *)b)->path);
}

static int snapshot_order(const void *key, const void *row)
{
	return strcmp(key, ((const struct snapshot *)row)->path);
}

static int known_key_order(const void *key, const void *row)
{
	return strcmp(key, ((const struct known *)row)->path);
}

// The known row of `path` among the first `count`, sorted, or NULL.
static struct known *known_find(const voe_editor_disk_watch *watch, const char *path,
				uint32_t count)
{
	VOE_BASE_ASSERT(path != NULL && count <= watch->known_count, "finding past the table");
	if (count == 0)
		return NULL;
	return bsearch(path, watch->known, count, sizeof *watch->known, known_key_order);
}

// A new row for `path`, stamp 0 and no bytes, at the table's end: the caller
// sorts.
static struct known *known_append(voe_editor_disk_watch *watch, const char *path)
{
	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "knowing no path");
	if (watch->known_count == watch->known_room) {
		watch->known_room = watch->known_room == 0 ? 64 : watch->known_room * 2;
		watch->known = realloc(watch->known, watch->known_room * sizeof *watch->known);
		VOE_BASE_ASSERT(watch->known != NULL, "no memory for the known paths");
	}
	watch->known[watch->known_count] = (struct known){ .path = text_copy(path) };
	return &watch->known[watch->known_count++];
}

// ---- The look, on its worker ----

// project/<relative> read, and strict-read when a landscape it knew; bytes,
// landscape and error malloced. Nothing read is said at its cause and tried
// again next look.
static void found_read(struct look *look, struct found *found, const char *path, bool known)
{
	struct voe_base_arena_mark mark = voe_base_arena_mark(look->memory);
	const uint8_t *bytes;
	size_t size = 0;

	VOE_BASE_ASSERT(found != NULL && path != NULL, "reading no found file");
	bytes = voe_platform_file_read(path, look->memory, &size, NULL);
	if (bytes != NULL && known && ends_in(found->path, LANDSCAPE_ENDING)) {
		found->memory = voe_base_arena_new(BLOCK_SIZE);
		found->landscape = voe_base_arena_push(found->memory, sizeof *found->landscape);
		voe_base_report_error_clear();
		voe_base_report_subject_push(found->path);
		if (!voe_assets_landscape_read_strict((const char *)bytes, size, found->memory,
						      found->landscape, NULL)) {
			const char *first = voe_base_report_error_first();

			found->error = text_copy(first != NULL ? first : "will not read");
			voe_base_arena_destroy(found->memory);
			found->memory = NULL;
			found->landscape = NULL;
		}
		voe_base_report_subject_pop();
	}
	if (bytes != NULL) {
		found->bytes = bytes_copy(bytes, size);
		found->size = size;
		found->read = true;
	}
	voe_base_arena_rewind(look->memory, mark);
	VOE_BASE_ASSERT(found->landscape == NULL || found->memory != NULL,
			"a landscape in no memory");
}

// One watched file stamped, read when its stamp is unlike the known one; a new
// landscape is only noted, so never read. One gone since its listing is skipped.
static void look_file(struct look *look, const char *relative)
{
	const struct snapshot *was = look->known_count == 0
		? NULL
		: bsearch(relative, look->known, look->known_count, sizeof *look->known,
			  snapshot_order);
	struct found *found = voe_base_arena_push(look->memory, sizeof *found);
	struct voe_base_arena_mark mark = voe_base_arena_mark(look->memory);
	const char *path = voe_platform_path_join(look->memory, look->folder, relative);
	uint64_t stamp = 0;

	VOE_BASE_ASSERT(relative != NULL && relative[0] != '\0', "stamping no file");
	if (!voe_platform_file_stamp(path, &stamp)) {
		voe_base_arena_rewind(look->memory, mark);
		return;
	}
	*found = (struct found){ .path = relative, .stamp = stamp };
	found->changed = was == NULL || was->stamp != stamp;
	if (found->changed && (was != NULL || !ends_in(relative, LANDSCAPE_ENDING)))
		found_read(look, found, path, was != NULL);
	voe_base_arena_rewind(look->memory, mark);
	if (look->last != NULL)
		look->last->next = found;
	else
		look->first = found;
	look->last = found;
}

// project/<relative> listed onto the stack. False when it will not list, said.
static bool level_push(struct look *look, struct walk_level *stack, uint32_t *depth,
		       const char *relative)
{
	const char *path = relative[0] == '\0'
		? look->folder
		: voe_platform_path_join(look->memory, look->folder, relative);

	VOE_BASE_ASSERT(*depth < WALK_DEPTH, "walking past the stack");
	stack[*depth] = (struct walk_level){ .relative = relative };
	if (!voe_platform_folder_list(path, look->memory, &stack[*depth].listing, NULL))
		return false;
	(*depth)++;
	return true;
}

// The burst's one task: the project walked as assets_walk.c walks it, each
// watched file looked at. Every entry is taken once, so the walk ends.
static void look_task(void *context, uint32_t index)
{
	struct look *look = context;
	struct walk_level stack[WALK_DEPTH];
	uint32_t depth = 0;

	VOE_BASE_ASSERT(look != NULL && index == 0, "looking with no look");
	look->complete = level_push(look, stack, &depth, "");
	while (depth > 0) {
		struct walk_level *top = &stack[depth - 1];
		const voe_platform_folder_entry *entry;
		const char *name;

		if (top->next == top->listing.count) {
			depth--;
			continue;
		}
		entry = &top->listing.entries[top->next++];
		if (entry->hidden || (entry->folder ? depth == 1 && skipped_named(entry->name)
						    : !watched_named(entry->name, depth == 1)))
			continue;
		name = relative_join(look->memory, top->relative, entry->name);
		if (!entry->folder) {
			look_file(look, name);
		} else if (depth == WALK_DEPTH) {
			if (!look->deep_said)
				VOE_BASE_WARNING("editor", "%s is deeper than %d folders; not watched",
						 name, WALK_DEPTH);
			look->deep_said = true;
		} else if (!level_push(look, stack, &depth, name)) {
			look->complete = false;
		}
	}
	VOE_BASE_ASSERT(depth == 0, "a walk left folders on its stack");
}

// ---- The frame's side ----

static void found_free(struct found *found)
{
	VOE_BASE_ASSERT(found != NULL, "freeing no found file");
	free(found->bytes);
	free(found->error);
	if (found->memory != NULL)
		voe_base_arena_destroy(found->memory);
	*found = (struct found){ .next = found->next };
	VOE_BASE_ASSERT(found->bytes == NULL, "a freed file kept its bytes");
}

static void changes_free(voe_editor_disk_watch *watch)
{
	VOE_BASE_ASSERT(watch != NULL, "freeing no watch's changes");
	for (uint32_t i = 0; i < watch->change_count; i++) {
		voe_editor_disk_change *change = &watch->changes[i];

		free(change->path);
		free(change->bytes);
		free(change->error);
		if (change->memory != NULL)
			voe_base_arena_destroy(change->memory);
	}
	watch->change_count = 0;
	VOE_BASE_ASSERT(watch->change_count == 0, "changes kept past their free");
}

// `found` held as a change, its bytes, landscape and error moved into it.
static void change_push(voe_editor_disk_watch *watch, struct found *found)
{
	VOE_BASE_ASSERT(found != NULL && found->read, "holding a change never read");
	if (watch->change_count == watch->change_room) {
		watch->change_room = watch->change_room == 0 ? 8 : watch->change_room * 2;
		watch->changes = realloc(watch->changes,
					 watch->change_room * sizeof *watch->changes);
		VOE_BASE_ASSERT(watch->changes != NULL, "no memory for the changes");
	}
	watch->changes[watch->change_count++] = (voe_editor_disk_change){
		.path = text_copy(found->path), .stamp = found->stamp,
		.bytes = found->bytes, .size = found->size, .landscape = found->landscape,
		.memory = found->memory, .error = found->error,
	};
	*found = (struct found){ .next = found->next };
}

// One file the finished look found, against the table's first `sorted` rows:
// skipped when written meanwhile, noted when new, held when changed and read.
static void take_found(voe_editor_disk_watch *watch, struct found *found, uint32_t sorted)
{
	struct known *known = known_find(watch, found->path, sorted);

	if (known != NULL)
		known->seen = true;
	if (known == NULL && (found->read || ends_in(found->path, LANDSCAPE_ENDING))) {
		known = known_append(watch, found->path);
		known->stamp = found->stamp;
		known->seen = true;
		if (!ends_in(found->path, LANDSCAPE_ENDING)) {
			known->bytes = found->bytes;
			known->size = found->size;
			found->bytes = NULL;
		}
	} else if (known != NULL && !known->wrote && found->changed && found->read) {
		change_push(watch, found);
	}
	found_free(found);
}

// The look in flight finished, its files taken, the paths it did not see let
// go when it saw everything, the table sorted again and the look's arena gone.
static void look_take(voe_editor_disk_watch *watch)
{
	uint32_t sorted = watch->known_count;
	uint32_t kept = 0;

	VOE_BASE_ASSERT(watch->burst != NULL, "taking no look");
	voe_platform_burst_finish(watch->burst);
	watch->burst = NULL;
	for (uint32_t i = 0; i < watch->known_count; i++)
		watch->known[i].seen = false;
	for (struct found *found = watch->look.first; found != NULL; found = found->next)
		take_found(watch, found, sorted);
	for (uint32_t i = 0; i < watch->known_count; i++) {
		struct known row = watch->known[i];

		if (!watch->look.complete || row.seen || row.wrote) {
			row.wrote = false;
			watch->known[kept++] = row;
			continue;
		}
		free(row.path);
		free(row.bytes);
	}
	watch->known_count = kept;
	qsort(watch->known, watch->known_count, sizeof *watch->known, known_order);
	voe_base_arena_destroy(watch->look.memory);
	watch->look = (struct look){ .deep_said = watch->look.deep_said };
}

// A look started on the standing workers over a snapshot of the known stamps.
static void look_start(voe_editor_disk_watch *watch)
{
	struct look *look = &watch->look;
	struct snapshot *known = NULL;

	VOE_BASE_ASSERT(watch->burst == NULL && watch->folder != NULL,
			"starting a look over another or in no folder");
	*look = (struct look){ .memory = voe_base_arena_new(BLOCK_SIZE),
			       .deep_said = look->deep_said };
	// The watch's own copy, freed only once a look is finished.
	look->folder = watch->folder;
	if (watch->known_count > 0)
		known = voe_base_arena_push(look->memory, watch->known_count * sizeof *known);
	for (uint32_t i = 0; i < watch->known_count; i++)
		known[i] = (struct snapshot){ watch->known[i].path, watch->known[i].stamp };
	look->known = known;
	look->known_count = watch->known_count;
	watch->burst = voe_platform_burst_start(voe_platform_burst_workers(false), 1,
						look_task, look);
}

// Everything known and held forgotten, a look in flight finished first, and
// `folder` the watch's, a look due at once.
static void forget(voe_editor_disk_watch *watch, const char *folder, double now)
{
	if (watch->burst != NULL) {
		voe_platform_burst_finish(watch->burst);
		watch->burst = NULL;
		for (struct found *found = watch->look.first; found != NULL; found = found->next)
			found_free(found);
		voe_base_arena_destroy(watch->look.memory);
	}
	watch->look = (struct look){ 0 };
	changes_free(watch);
	for (uint32_t i = 0; i < watch->known_count; i++) {
		free(watch->known[i].path);
		free(watch->known[i].bytes);
	}
	watch->known_count = 0;
	free(watch->folder);
	watch->folder = folder != NULL ? text_copy(folder) : NULL;
	watch->looked = now - LOOK_SECONDS;
	VOE_BASE_ASSERT(watch->burst == NULL && watch->change_count == 0, "a forget kept work");
}

voe_editor_disk_watch *voe_editor_disk_watch_create(void)
{
	voe_editor_disk_watch *watch = calloc(1, sizeof *watch);

	VOE_BASE_ASSERT(watch != NULL, "no memory for a disk watch");
	watch->scratch = voe_base_arena_new(SCRATCH_SIZE);
	VOE_BASE_ASSERT(watch->folder == NULL && watch->burst == NULL, "a new watch looking");
	return watch;
}

void voe_editor_disk_watch_destroy(voe_editor_disk_watch *watch)
{
	if (watch == NULL)
		return;
	forget(watch, NULL, 0.0);
	VOE_BASE_ASSERT(watch->burst == NULL, "destroying a watch mid-look");
	free(watch->known);
	free(watch->changes);
	voe_base_arena_destroy(watch->scratch);
	free(watch);
}

void voe_editor_disk_watch_look(voe_editor_disk_watch *watch, const char *folder,
				double now)
{
	VOE_BASE_ASSERT(watch != NULL, "looking with no watch");
	VOE_BASE_ASSERT(folder == NULL || folder[0] != '\0', "looking in an empty folder name");
	if ((folder == NULL) != (watch->folder == NULL) ||
	    (folder != NULL && strcmp(folder, watch->folder) != 0))
		forget(watch, folder, now);
	if (folder == NULL || watch->burst != NULL || watch->change_count > 0 ||
	    now - watch->looked < LOOK_SECONDS)
		return;
	watch->looked = now;
	look_start(watch);
}

const voe_editor_disk_change *voe_editor_disk_watch_changes(voe_editor_disk_watch *watch,
							    uint32_t *count)
{
	VOE_BASE_ASSERT(watch != NULL && count != NULL, "changes into nothing");
	if (watch->burst != NULL && voe_platform_burst_done(watch->burst))
		look_take(watch);
	*count = watch->burst == NULL ? watch->change_count : 0;
	return *count > 0 ? watch->changes : NULL;
}

void voe_editor_disk_watch_settle(voe_editor_disk_watch *watch, uint32_t index, bool taken)
{
	voe_editor_disk_change *change;
	struct known *known;
	uint32_t open = 0;

	VOE_BASE_ASSERT(watch != NULL && index < watch->change_count, "settling no change");
	change = &watch->changes[index];
	VOE_BASE_ASSERT(!change->settled, "settling a change twice");
	change->settled = true;
	known = change->written ? NULL : known_find(watch, change->path, watch->known_count);
	if (known != NULL) {
		known->stamp = change->stamp;
		if (taken && !ends_in(change->path, LANDSCAPE_ENDING)) {
			free(known->bytes);
			known->bytes = change->bytes;
			known->size = change->size;
			change->bytes = NULL;
		}
	}
	for (uint32_t i = 0; i < watch->change_count; i++)
		open += watch->changes[i].settled ? 0 : 1;
	if (open == 0)
		changes_free(watch);
}

// `path` project-relative, pointing into it, or NULL when outside the folder.
static const char *relative_of(const voe_editor_disk_watch *watch, const char *path)
{
	size_t head = strlen(watch->folder);

	VOE_BASE_ASSERT(path != NULL && path[0] != '\0', "a path with no name");
	if (strncmp(path, watch->folder, head) == 0 && (path[head] == '/' || path[head] == '\\'))
		return path[head + 1] != '\0' ? path + head + 1 : NULL;
	if (path[0] == '/' || path[0] == '\\' || (isalpha((unsigned char)path[0]) && path[1] == ':'))
		return NULL;
	return path;
}

// Whether a project-relative `path` is of a watched kind.
static bool watched_path(const char *path)
{
	const char *slash = strrchr(path, '/');

	return watched_named(slash != NULL ? slash + 1 : path, slash == NULL);
}

void voe_editor_disk_watch_wrote(voe_editor_disk_watch *watch, const char *path)
{
	const char *relative;
	const char *full;
	struct known *known;
	const uint8_t *bytes;
	size_t size = 0;

	VOE_BASE_ASSERT(watch != NULL && path != NULL, "told of no write");
	if (watch->folder == NULL || (relative = relative_of(watch, path)) == NULL ||
	    !watched_path(relative))
		return;
	known = known_find(watch, relative, watch->known_count);
	if (known == NULL) {
		(void)known_append(watch, relative);
		qsort(watch->known, watch->known_count, sizeof *watch->known, known_order);
		known = known_find(watch, relative, watch->known_count);
	}
	full = voe_platform_path_join(watch->scratch, watch->folder, relative);
	if (!voe_platform_file_stamp(full, &known->stamp))
		known->stamp = 0;
	free(known->bytes);
	known->bytes = NULL;
	known->size = 0;
	bytes = ends_in(relative, LANDSCAPE_ENDING)
		? NULL
		: voe_platform_file_read(full, watch->scratch, &size, NULL);
	if (bytes != NULL) {
		known->bytes = bytes_copy(bytes, size);
		known->size = size;
	}
	known->wrote = watch->burst != NULL;
	for (uint32_t i = 0; i < watch->change_count; i++)
		if (strcmp(watch->changes[i].path, relative) == 0)
			watch->changes[i].written = true;
	voe_base_arena_clear(watch->scratch);
	VOE_BASE_ASSERT(known_find(watch, relative, watch->known_count) != NULL,
			"a written path not known");
}

const uint8_t *voe_editor_disk_watch_known(voe_editor_disk_watch *watch, const char *path,
					   size_t *size)
{
	const char *relative;
	const struct known *known;

	VOE_BASE_ASSERT(watch != NULL && path != NULL && size != NULL, "asking into nothing");
	*size = 0;
	if (watch->folder == NULL || (relative = relative_of(watch, path)) == NULL)
		return NULL;
	known = known_find(watch, relative, watch->known_count);
	if (known == NULL || known->bytes == NULL)
		return NULL;
	*size = known->size;
	return known->bytes;
}
