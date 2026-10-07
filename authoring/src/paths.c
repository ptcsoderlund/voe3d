// Paths in scene text followed and looked for. See authoring/paths.h for what
// matches and what is never touched.
//
// ONE WALK SERVES BOTH CALLS. It goes line by line and, on a `key = value` line
// whose value opens with `"` or `[`, string by string; everything else is
// copied through. With no output buffer it only counts, which is all
// voe_authoring_paths_named needs and what lets a follow push its text once,
// exactly sized: it walks to count the bytes, pushes, and walks again to write.
// Nothing recurses and every loop is bounded by the text's size.
//
// A STRING IS MATCHED AS IT IS READ: each source byte, an escape's two counting
// as one, is compared against `from` while the source index where `from` ends
// is remembered, so a match replaces exactly the source bytes that spelled it
// and copies the rest of the string as it was, escapes and all.
#include <authoring/paths.h>

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

#define MODULE "authoring"

typedef struct paths_walk {
	const char *text;
	size_t size;
	const char *from;
	size_t from_size;
	const char *to;
	size_t to_size;
	char *out; // NULL while only counting
	size_t written;
	size_t changed;
	size_t longest;
	size_t line;
} paths_walk;

static void emit(paths_walk *walk, const char *bytes, size_t count)
{
	VOE_BASE_ASSERT(bytes != NULL || count == 0, "bytes to emit");
	if (walk->out != NULL && count > 0)
		memcpy(walk->out + walk->written, bytes, count);
	walk->written += count;
	VOE_BASE_ASSERT(walk->written >= count, "written wrapped");
}

// The string whose opening quote is at `open`, closed before `end`, emitted
// with its `from` start replaced when it matches. False when it never closes.
static bool walk_string(paths_walk *walk, size_t open, size_t end, size_t *next)
{
	const char *text = walk->text;
	size_t at = open + 1;
	size_t unescaped = 0;
	size_t prefix_end = 0;
	bool same = true;
	char after = '\0';

	VOE_BASE_ASSERT(text[open] == '"', "a string opens with a quote");
	VOE_BASE_ASSERT(end <= walk->size, "the line is inside the text");
	while (at < end && text[at] != '"') {
		size_t step = text[at] == '\\' && at + 1 < end ? 2 : 1;
		char byte = text[at + step - 1];

		if (unescaped < walk->from_size)
			same = same && byte == walk->from[unescaped];
		else if (unescaped == walk->from_size)
			after = byte;
		unescaped++;
		at += step;
		if (unescaped == walk->from_size)
			prefix_end = at;
	}
	if (at >= end)
		return false;
	emit(walk, "\"", 1);
	if (same && unescaped >= walk->from_size &&
	    (unescaped == walk->from_size || after == '/')) {
		size_t length = unescaped - walk->from_size + walk->to_size;

		walk->changed++;
		if (length > walk->longest)
			walk->longest = length;
		emit(walk, walk->to, walk->to_size);
		emit(walk, text + prefix_end, at - prefix_end);
	} else {
		emit(walk, text + open + 1, at - open - 1);
	}
	emit(walk, "\"", 1);
	*next = at + 1;
	return true;
}

// Where the value of the line [start, end) begins, when it is a quoted string
// or an array; `end` when the line is anything else and is copied whole.
static size_t value_start(const paths_walk *walk, size_t start, size_t end)
{
	const char *text = walk->text;
	size_t at = start;

	VOE_BASE_ASSERT(start <= end && end <= walk->size, "a line in the text");
	while (at < end && (text[at] == ' ' || text[at] == '\t'))
		at++;
	if (at == end || text[at] == '[' ||
	    (text[at] == '/' && at + 1 < end && text[at + 1] == '/'))
		return end;
	const char *equals = memchr(text + at, '=', end - at);

	if (equals == NULL)
		return end;
	at = (size_t)(equals - text) + 1;
	while (at < end && (text[at] == ' ' || text[at] == '\t'))
		at++;
	return at < end && (text[at] == '"' || text[at] == '[') ? at : end;
}

static bool walk_text(paths_walk *walk)
{
	const char *text = walk->text;
	size_t start = 0;

	VOE_BASE_ASSERT(walk->from_size > 0, "a path to follow");
	walk->line = 0;
	while (start < walk->size) {
		const char *newline = memchr(text + start, '\n', walk->size - start);
		size_t end = newline != NULL ? (size_t)(newline - text) : walk->size;
		size_t at = value_start(walk, start, end);

		walk->line++;
		emit(walk, text + start, at - start);
		while (at < end) {
			if (text[at] != '"') {
				emit(walk, text + at, 1);
				at++;
			} else if (!walk_string(walk, at, end, &at)) {
				return false;
			}
		}
		start = newline != NULL ? end + 1 : end;
		emit(walk, newline != NULL ? "\n" : "", newline != NULL ? 1 : 0);
	}
	VOE_BASE_ASSERT(start == walk->size, "the whole text walked");
	return true;
}

bool voe_authoring_paths_follow(const char *text, size_t size,
				const char *from, const char *to,
				voe_base_arena *arena,
				voe_authoring_paths_followed *out)
{
	VOE_BASE_ASSERT(text != NULL && from != NULL && to != NULL, "inputs");
	VOE_BASE_ASSERT(arena != NULL && out != NULL, "an arena and an out");
	VOE_BASE_ASSERT(strpbrk(to, "\"\\") == NULL,
			"`to` holds no quote or backslash");
	paths_walk walk = { .text = text, .size = size,
			    .from = from, .from_size = strlen(from),
			    .to = to, .to_size = strlen(to) };

	if (!walk_text(&walk)) {
		VOE_BASE_ERROR(MODULE, "line %zu: a string is never closed",
			       walk.line);
		return false;
	}
	size_t total = walk.written;

	walk.out = voe_base_arena_push(arena, total + 1);
	walk.written = 0;
	walk.changed = 0;
	walk.longest = 0;
	bool written = walk_text(&walk);

	VOE_BASE_ASSERT(written && walk.written == total, "the same walk twice");
	walk.out[total] = '\0';
	out->text = (voe_authoring_text){ .text = walk.out, .size = total };
	out->changed = walk.changed;
	out->longest = walk.longest;
	return true;
}

bool voe_authoring_paths_named(const char *text, size_t size, const char *path)
{
	VOE_BASE_ASSERT(text != NULL && path != NULL, "inputs");
	paths_walk walk = { .text = text, .size = size,
			    .from = path, .from_size = strlen(path), .to = "" };
	bool closed = walk_text(&walk);

	VOE_BASE_ASSERT(closed || walk.line > 0, "an open string is on a line");
	return walk.changed > 0;
}
