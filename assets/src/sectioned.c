// The sectioned-format reader. One loop over the lines, twice.
//
// TWO PASSES OVER THE SAME LINE LEXER, AND THAT IS HOW THE OUTPUT IS SIZED
// EXACTLY WITHOUT THE CALLER GUESSING A CAPACITY. The JSON reader makes its
// caller state how many values it will hold, because a nested document can
// hold far more values than it has bytes of structure. This format cannot: a
// line is at most one section or one key, and every name and value is a
// substring of the text or, unescaped, shorter than one, so the output is
// bounded by the input and there is nothing for a hostile file to amplify. So
// the first pass lexes every line, refuses anything malformed and counts; three
// arrays are pushed at exactly those sizes; the second pass lexes the same
// lines again and stores. Both passes go through read_line(), so they cannot
// disagree about what a line is. The only failure the second pass can find is a
// duplicate, because a duplicate needs the earlier names stored to compare
// against.
//
// EVERY NAME AND VALUE IS COPIED INTO ONE TEXT POOL, PUSHED ONCE. Two pushes
// are not guaranteed to be adjacent (base/arena.h), and one push per string
// would round each to sixteen bytes. The spans copied are disjoint substrings
// of the text and unescaping only ever drops a byte, so the pool is the text's
// size plus one NUL per string, and it cannot run out; the assert in intern()
// is there to say so.
//
// A QUOTED VALUE IS CHECKED FOR ESCAPES WHEN IT IS LEXED AND UNESCAPED WHEN IT
// IS STORED. read_key() refuses a backslash that is not `\"` or `\\` on both
// passes, so the counting pass has already refused every bad one and intern()
// can copy a quoted value knowing each backslash is followed by one of the two.
//
// EVERY READ IS BOUNDED BY `size` AND NEVER BY A TERMINATOR. The text is a span
// the caller handed over — an embedded file, a buffer from wherever — and a
// reader that leant on a NUL would pass its tests and fail on the real thing.
// A NUL inside the text is refused for the opposite reason: the strings this
// hands back are NUL-terminated, and one inside a value would end it early and
// silently.
//
// THERE IS NO RECURSION AND NO STACK, and neither was designed out: a dotted
// section name is flat text here, nothing nests, and the loop is over lines.
#include <assets/sectioned.h>

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// The two things a section name may not have more of: parts, and dots between
// them. Two parts is what the sketch shows and all anything needs.
#define NAME_PARTS_MAX 2

enum line_kind {
	// Nothing, or a comment. Skipped.
	LINE_BLANK,
	LINE_SECTION,
	LINE_KEY,
};

// A run of bytes inside the text.
struct span {
	size_t start;
	size_t length;
};

struct line {
	enum line_kind kind;
	// The section's name, or the key's.
	struct span name;
	// The key's value, quotes already stripped. Unused for a section.
	struct span value;
	// The value was quoted, so its span may hold `\"` and `\\`, and nothing
	// else after a backslash.
	bool quoted;
};

struct parser {
	const char *text;
	size_t size;
	size_t pos;
	// One-based, of the line being read. Zero before the first.
	uint32_t line;

	// False on the counting pass, true on the storing pass.
	bool storing;

	voe_assets_sectioned_section *sections;
	voe_assets_sectioned_key *keys;
	uint32_t section_count;
	uint32_t key_count;
	// What the counting pass found, which the storing pass may not exceed.
	uint32_t section_capacity;
	uint32_t key_capacity;

	char *pool;
	size_t pool_used;
	size_t pool_size;
};

static bool malformed(const struct parser *parser, const char *message)
{
	VOE_BASE_ERROR("assets", "sectioned: line %u: %s", parser->line,
		       message);
	return false;
}

static bool is_blank(char c)
{
	return c == ' ' || c == '\t';
}

// The span with the blanks at both ends taken off.
static struct span trimmed(const struct parser *parser, size_t start,
			   size_t end)
{
	while (start < end && is_blank(parser->text[start]))
		start++;
	while (end > start && is_blank(parser->text[end - 1]))
		end--;
	return (struct span){ .start = start, .length = end - start };
}

static bool has_blank(const struct parser *parser, struct span span)
{
	for (size_t i = 0; i < span.length; i++)
		if (is_blank(parser->text[span.start + i]))
			return true;
	return false;
}

// Only blanks between here and the end of the line.
static bool rest_is_blank(const struct parser *parser, size_t start,
			  size_t end)
{
	return trimmed(parser, start, end).length == 0;
}

// The first `c` in [start, end), or `end` when there is none.
static size_t find(const struct parser *parser, size_t start, size_t end,
		   char c)
{
	for (size_t i = start; i < end; i++)
		if (parser->text[i] == c)
			return i;
	return end;
}

// `[Name]` — pos is on the bracket. Blanks inside the brackets are trimmed off
// the name; a name is non-empty, has no blank in it, and has at most
// NAME_PARTS_MAX parts with a non-empty part on either side of every dot.
static bool read_section(struct parser *parser, size_t start, size_t end,
			 struct line *line)
{
	size_t close = find(parser, start + 1, end, ']');
	struct span name;
	uint32_t parts = 1;

	if (close == end)
		return malformed(parser,
				 "a section header with no closing bracket");
	if (!rest_is_blank(parser, close + 1, end))
		return malformed(parser,
				 "something after a section header's closing bracket");

	name = trimmed(parser, start + 1, close);
	if (name.length == 0)
		return malformed(parser, "a section with no name");
	if (has_blank(parser, name))
		return malformed(parser, "a section name with a blank in it");

	for (size_t i = 0; i < name.length; i++) {
		if (parser->text[name.start + i] != '.')
			continue;
		// A dot at either end or two in a row is an empty part.
		if (i == 0 || i == name.length - 1 ||
		    parser->text[name.start + i + 1] == '.')
			return malformed(parser,
					 "a section name with an empty part");
		parts++;
	}
	if (parts > NAME_PARTS_MAX)
		return malformed(parser,
				 "a section name dotted deeper than two parts");

	line->kind = LINE_SECTION;
	line->name = name;
	return true;
}

// `key=value` — a `=` somewhere in [start, end). The key is what is before it,
// trimmed; the value is what is after it, trimmed, and if that starts with `"`
// it is what is between that quote and the next unescaped one, with only blanks
// allowed after the closing quote. Inside the quotes a backslash is `\"` or `\\`
// and nothing else — ADR-0149 — and anything else after one, the end of the
// line included, refuses the line.
static bool read_key(struct parser *parser, size_t start, size_t end,
		     struct line *line)
{
	size_t equals = find(parser, start, end, '=');
	struct span name;
	struct span value;

	if (equals == end)
		return malformed(parser,
				 "a line that is not a section, a key or a comment");

	name = trimmed(parser, start, equals);
	if (name.length == 0)
		return malformed(parser, "a key with no name");
	if (has_blank(parser, name))
		return malformed(parser, "a key name with a blank in it");

	value = trimmed(parser, equals + 1, end);
	line->quoted = value.length > 0 && parser->text[value.start] == '"';
	if (line->quoted) {
		size_t open = value.start;
		size_t close = open + 1;

		for (; close < end && parser->text[close] != '"'; close++) {
			if (parser->text[close] != '\\')
				continue;
			if (close + 1 == end || (parser->text[close + 1] != '"' &&
						 parser->text[close + 1] != '\\'))
				return malformed(parser,
						 "a backslash in a quoted value that is not \\\" or \\\\");
			close++;
		}
		if (close == end)
			return malformed(parser,
					 "a quoted value with no closing quote on its line");
		if (!rest_is_blank(parser, close + 1, end))
			return malformed(parser,
					 "something after a quoted value's closing quote");
		value = (struct span){ .start = open + 1,
				       .length = close - open - 1 };
	}

	line->kind = LINE_KEY;
	line->name = name;
	line->value = value;
	return true;
}

// One line, from pos to the newline or the end of the text, classified and
// lexed. pos is left on the first byte of the next line.
static bool read_line(struct parser *parser, struct line *line)
{
	size_t start = parser->pos;
	size_t end = find(parser, start, parser->size, '\n');
	size_t content_end = end;
	size_t first;

	// pos moves before anything can fail, so a refused line is never read
	// twice.
	parser->pos = end < parser->size ? end + 1 : end;
	parser->line++;

	if (content_end > start && parser->text[content_end - 1] == '\r')
		content_end--;
	if (find(parser, start, content_end, '\0') != content_end)
		return malformed(parser, "a NUL byte, which is not text");

	first = trimmed(parser, start, content_end).start;
	if (first == content_end) {
		line->kind = LINE_BLANK;
		return true;
	}
	if (content_end - first >= 2 && parser->text[first] == '/' &&
	    parser->text[first + 1] == '/') {
		line->kind = LINE_BLANK;
		return true;
	}
	if (parser->text[first] == '[')
		return read_section(parser, first, content_end, line);
	return read_key(parser, first, content_end, line);
}

// A span copied out of the text as a NUL-terminated string in the pool, with
// `\"` and `\\` taken back to one byte when it was quoted. read_key() has
// already refused any other backslash in a quoted span.
static const char *intern(struct parser *parser, struct span span,
			  bool quoted)
{
	char *at = parser->pool + parser->pool_used;
	size_t length = 0;

	VOE_BASE_ASSERT(parser->pool_used + span.length + 1 <=
				parser->pool_size,
			"the text pool was sized from the same lines it now holds");
	for (size_t i = 0; i < span.length; i++) {
		if (quoted && parser->text[span.start + i] == '\\')
			i++;
		at[length++] = parser->text[span.start + i];
	}
	at[length] = '\0';
	parser->pool_used += length + 1;
	return at;
}

static bool same(const struct parser *parser, const char *stored,
		 struct span span)
{
	return strlen(stored) == span.length &&
	       memcmp(stored, parser->text + span.start, span.length) == 0;
}

static bool add_section(struct parser *parser, struct span name)
{
	if (!parser->storing) {
		parser->section_count++;
		return true;
	}

	VOE_BASE_ASSERT(parser->section_count < parser->section_capacity,
			"the storing pass found a section the counting pass did not");
	for (uint32_t i = 0; i < parser->section_count; i++)
		if (same(parser, parser->sections[i].name, name))
			return malformed(parser,
					 "a section this file already has");

	parser->sections[parser->section_count++] =
		(voe_assets_sectioned_section){
			.name = intern(parser, name, false),
			.first_key = parser->key_count,
			.key_count = 0,
		};
	return true;
}

static bool add_key(struct parser *parser, struct span name, struct span value,
		    bool quoted)
{
	voe_assets_sectioned_section *section;

	if (parser->section_count == 0)
		return malformed(parser, "a key before any section");
	if (!parser->storing) {
		parser->key_count++;
		return true;
	}

	VOE_BASE_ASSERT(parser->key_count < parser->key_capacity,
			"the storing pass found a key the counting pass did not");
	section = &parser->sections[parser->section_count - 1];
	for (uint32_t i = 0; i < section->key_count; i++)
		if (same(parser, parser->keys[section->first_key + i].name,
			 name))
			return malformed(parser,
					 "a key this section already has");

	parser->keys[parser->key_count++] = (voe_assets_sectioned_key){
		.name = intern(parser, name, false),
		.value = intern(parser, value, quoted),
	};
	section->key_count++;
	return true;
}

// Every line, start to end. The pass that is being made is parser->storing.
static bool read_lines(struct parser *parser)
{
	parser->pos = 0;
	parser->line = 0;
	parser->section_count = 0;
	parser->key_count = 0;

	while (parser->pos < parser->size) {
		struct line line = { 0 };

		if (!read_line(parser, &line))
			return false;
		switch (line.kind) {
		case LINE_BLANK:
			break;
		case LINE_SECTION:
			if (!add_section(parser, line.name))
				return false;
			break;
		case LINE_KEY:
			if (!add_key(parser, line.name, line.value,
				     line.quoted))
				return false;
			break;
		}
	}
	return true;
}

bool voe_assets_sectioned_parse(const char *text, size_t size,
				voe_base_arena *arena,
				voe_assets_sectioned *out)
{
	struct parser parser = {
		.text = text,
		.size = size,
	};

	VOE_BASE_ASSERT(text != NULL, "reading a sectioned file from nothing");
	VOE_BASE_ASSERT(arena != NULL,
			"reading a sectioned file without an arena");
	VOE_BASE_ASSERT(out != NULL, "reading a sectioned file into nothing");

	// Counts are 32-bit and a line is at least one byte, so a text that
	// does not fit in one cannot have its lines counted. No authored file
	// is within a thousandth of this.
	if (size > UINT32_MAX) {
		VOE_BASE_ERROR("assets",
			       "sectioned: %zu bytes is more text than this reader will hold",
			       size);
		return false;
	}

	if (!read_lines(&parser))
		return false;

	parser.section_capacity = parser.section_count;
	parser.key_capacity = parser.key_count;
	// A push of nothing has no caller (base/arena.c), so an empty file
	// pushes no arrays; the pool gets one byte over so it is never empty.
	if (parser.section_capacity > 0)
		parser.sections = voe_base_arena_push(
			arena, (size_t)parser.section_capacity *
				       sizeof(*parser.sections));
	if (parser.key_capacity > 0)
		parser.keys = voe_base_arena_push(
			arena,
			(size_t)parser.key_capacity * sizeof(*parser.keys));
	parser.pool_size = size + parser.section_capacity +
			   2 * (size_t)parser.key_capacity + 1;
	parser.pool = voe_base_arena_push(arena, parser.pool_size);

	parser.storing = true;
	if (!read_lines(&parser))
		return false;

	VOE_BASE_ASSERT(parser.section_count == parser.section_capacity &&
				parser.key_count == parser.key_capacity,
			"the two passes disagreed about what the lines were");

	out->sections = parser.sections;
	out->section_count = parser.section_count;
	out->keys = parser.keys;
	out->key_count = parser.key_count;
	return true;
}

uint32_t voe_assets_sectioned_find(const voe_assets_sectioned *doc,
				   const char *name)
{
	VOE_BASE_DEBUG_ASSERT(doc != NULL, "searching no document");
	VOE_BASE_DEBUG_ASSERT(name != NULL, "searching for no section");

	for (uint32_t i = 0; i < doc->section_count; i++)
		if (strcmp(doc->sections[i].name, name) == 0)
			return i;
	return VOE_ASSETS_SECTIONED_NONE;
}

const char *voe_assets_sectioned_value(const voe_assets_sectioned *doc,
				       uint32_t section, const char *name)
{
	const voe_assets_sectioned_section *at;

	VOE_BASE_DEBUG_ASSERT(doc != NULL, "reading no document");
	VOE_BASE_DEBUG_ASSERT(name != NULL, "reading no key");
	VOE_BASE_ASSERT(section < doc->section_count,
			"a section index this document does not have");

	at = &doc->sections[section];
	for (uint32_t i = 0; i < at->key_count; i++) {
		const voe_assets_sectioned_key *key =
			&doc->keys[at->first_key + i];

		if (strcmp(key->name, name) == 0)
			return key->value;
	}
	return NULL;
}
