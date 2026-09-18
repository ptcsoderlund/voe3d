// The sectioned-format reader: that the principal's own sketch comes back as
// the text it holds, that every question the format left open is answered the
// way the header says, and that every shape of broken line is refused.
//
// THE PINNED DECISIONS ARE THE POINT OF THIS FILE. Two-part names and no
// deeper, blanks tolerated around the punctuation and refused inside a name,
// two escapes inside quotes and none outside them, duplicates refused, no key
// before a section, no trailing comment — each is one test, so that the day one
// of them changes it changes here on purpose and not in the reader by accident.
//
// EVERYTHING THAT COMES BACK IS COMPARED AS TEXT. The moment a test here calls
// strtod it has started writing layer two, and the reader is being tested for
// something it must not do.
//
// THE TEXT IS PASSED WITH A LENGTH AND NEVER LEANT ON AS A C STRING: one case
// cuts a literal short to prove the reader stops at `size` and not at the
// terminator.
#include <assets/sectioned.h>
#include <base/arena.h>

#include <testing/test.h>

#include <stdio.h>
#include <string.h>

// The principal's sketch, as the card restates it, plus the three sigil forms
// the card quotes from the same file — `400.12345d`, `400_000_000_000`,
// `bool=1` — which exist for layer three and have to come back untouched.
static const char *const SKETCH =
	"[Player1]\n"
	"type=\"Node\"\n"
	"id=1\n"
	"hp=20.0\n"
	"\n"
	"[Player1.Stats]\n"
	"max_hp=200.0\n"
	"some_array=[0,2,3,4,5]\n"
	"\n"
	"//This is a comment\n"
	"\n"
	"[Theme1]\n"
	"accent=\"#2B2B2B\"\n"
	"contrast_strength=1.25\n"
	"error_light=\"#B836BA\"\n"
	"\n"
	"[Sigils]\n"
	"precise=400.12345d\n"
	"big=400_000_000_000\n"
	"bool=1\n";

static bool parse(voe_base_arena *arena, const char *text,
		  voe_assets_sectioned *doc)
{
	return voe_assets_sectioned_parse(text, strlen(text), arena, doc);
}

// Accepted, and the value of the one key in the one section is exactly this.
static void reads_as(voe_base_arena *arena, const char *text,
		     const char *section, const char *key, const char *expected)
{
	voe_assets_sectioned doc;
	uint32_t found;
	const char *value;

	if (!parse(arena, text, &doc)) {
		fprintf(stderr, "      refused: %s\n", text);
		VOE_TEST_CHECK(false);
		return;
	}
	found = voe_assets_sectioned_find(&doc, section);
	if (found == VOE_ASSETS_SECTIONED_NONE) {
		fprintf(stderr, "      no section %s in: %s\n", section, text);
		VOE_TEST_CHECK(false);
		return;
	}
	value = voe_assets_sectioned_value(&doc, found, key);
	if (value == NULL) {
		fprintf(stderr, "      no key %s in: %s\n", key, text);
		VOE_TEST_CHECK(false);
		return;
	}
	if (strcmp(value, expected) != 0) {
		fprintf(stderr,
			"      in: %s\n      actual:   %s\n      expected: %s\n",
			text, value, expected);
		VOE_TEST_CHECK(false);
	}
}

static void refused(voe_base_arena *arena, const char *text)
{
	voe_assets_sectioned doc;
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);

	if (parse(arena, text, &doc)) {
		// The text is printed so that a failure names the case rather
		// than a line number in a list of them.
		fprintf(stderr, "      accepted: %s\n", text);
		VOE_TEST_CHECK(false);
	}
	voe_base_arena_rewind(arena, mark);
}

static void accepted(voe_base_arena *arena, const char *text)
{
	voe_assets_sectioned doc;

	if (!parse(arena, text, &doc)) {
		fprintf(stderr, "      refused: %s\n", text);
		VOE_TEST_CHECK(false);
	}
}

static void the_sketch_comes_back_as_its_text(voe_base_arena *arena)
{
	voe_assets_sectioned doc;
	uint32_t section;

	VOE_TEST_CHECK(parse(arena, SKETCH, &doc));
	VOE_TEST_CHECK_INT(doc.section_count, 4);
	VOE_TEST_CHECK_INT(doc.key_count, 11);

	// In file order, and each section's keys are the run after it.
	VOE_TEST_CHECK(strcmp(doc.sections[0].name, "Player1") == 0);
	VOE_TEST_CHECK_INT(doc.sections[0].first_key, 0);
	VOE_TEST_CHECK_INT(doc.sections[0].key_count, 3);
	VOE_TEST_CHECK(strcmp(doc.sections[1].name, "Player1.Stats") == 0);
	VOE_TEST_CHECK_INT(doc.sections[1].first_key, 3);
	VOE_TEST_CHECK_INT(doc.sections[1].key_count, 2);
	VOE_TEST_CHECK(strcmp(doc.sections[2].name, "Theme1") == 0);
	VOE_TEST_CHECK_INT(doc.sections[2].first_key, 5);
	VOE_TEST_CHECK_INT(doc.sections[2].key_count, 3);
	VOE_TEST_CHECK(strcmp(doc.sections[3].name, "Sigils") == 0);
	VOE_TEST_CHECK_INT(doc.sections[3].first_key, 8);
	VOE_TEST_CHECK_INT(doc.sections[3].key_count, 3);

	// Quotes stripped, numbers left as the characters they were.
	reads_as(arena, SKETCH, "Player1", "type", "Node");
	reads_as(arena, SKETCH, "Player1", "id", "1");
	reads_as(arena, SKETCH, "Player1", "hp", "20.0");
	reads_as(arena, SKETCH, "Player1.Stats", "max_hp", "200.0");
	// The array is the text it was written as, brackets included.
	reads_as(arena, SKETCH, "Player1.Stats", "some_array", "[0,2,3,4,5]");
	// A hex colour: the reason comments are // and not #.
	reads_as(arena, SKETCH, "Theme1", "accent", "#2B2B2B");
	reads_as(arena, SKETCH, "Theme1", "contrast_strength", "1.25");
	reads_as(arena, SKETCH, "Theme1", "error_light", "#B836BA");
	// The sigils are layer three's and come back untouched.
	reads_as(arena, SKETCH, "Sigils", "precise", "400.12345d");
	reads_as(arena, SKETCH, "Sigils", "big", "400_000_000_000");
	reads_as(arena, SKETCH, "Sigils", "bool", "1");

	// Keys are in file order too, through the array and not the lookup.
	VOE_TEST_CHECK(strcmp(doc.keys[3].name, "max_hp") == 0);
	VOE_TEST_CHECK(strcmp(doc.keys[3].value, "200.0") == 0);

	// What is not there: a section, a key, and a key that is in a different
	// section — the dotted child does not see its parent's keys.
	VOE_TEST_CHECK_INT(voe_assets_sectioned_find(&doc, "Player2"),
			   VOE_ASSETS_SECTIONED_NONE);
	VOE_TEST_CHECK_INT(voe_assets_sectioned_find(&doc, "Player1.Stat"),
			   VOE_ASSETS_SECTIONED_NONE);
	section = voe_assets_sectioned_find(&doc, "Player1");
	VOE_TEST_CHECK(voe_assets_sectioned_value(&doc, section, "mp") == NULL);
	VOE_TEST_CHECK(voe_assets_sectioned_value(&doc, section, "max_hp") ==
		       NULL);
}

// Two parts and no more, and a dotted name is flat text: it needs no parent.
static void a_name_has_at_most_two_parts(voe_base_arena *arena)
{
	accepted(arena, "[a]\n");
	accepted(arena, "[a.b]\n");
	accepted(arena, "[Player1.Stats]\nx=1\n");
	refused(arena, "[a.b.c]\n");
	refused(arena, "[.a]\n");
	refused(arena, "[a.]\n");
	refused(arena, "[a..b]\n");
	refused(arena, "[.]\n");
}

// Blanks around the punctuation and at the ends of a line are nothing; blanks
// inside a name are a typo.
static void blanks_are_tolerated_around_and_refused_inside(
	voe_base_arena *arena)
{
	voe_assets_sectioned doc;

	reads_as(arena, "[ Player ]\n  hp   =   20  \n", "Player", "hp", "20");
	reads_as(arena, "\t[Player]\t\n\thp\t=\t20\t\n", "Player", "hp", "20");
	reads_as(arena, "[Player]\nhp = \"20\"  \n", "Player", "hp", "20");
	// Blanks inside the quotes are the value's own.
	reads_as(arena, "[Player]\nname=\"  two words  \"\n", "Player", "name",
		 "  two words  ");
	// And an unquoted value keeps its inside blanks, losing only the ends.
	reads_as(arena, "[Player]\nname=  two words  \n", "Player", "name",
		 "two words");

	refused(arena, "[Pl ayer]\n");
	refused(arena, "[Player]\nmax hp=1\n");

	// The name that came back is the trimmed one.
	VOE_TEST_CHECK(parse(arena, "[ Player ]\n", &doc));
	VOE_TEST_CHECK(strcmp(doc.sections[0].name, "Player") == 0);
}

// Two escapes inside quotes, `\"` and `\\`, and any other backslash there
// refuses the line (ADR-0149). Outside quotes a backslash is a byte.
static void a_quoted_value_has_two_escapes(voe_base_arena *arena)
{
	reads_as(arena, "[P]\nname = \"say \\\"hi\\\"\"\n", "P", "name",
		 "say \"hi\"");
	reads_as(arena, "[P]\npath = \"a\\\\b\"\n", "P", "path", "a\\b");
	// An escaped backslash right before the closing quote does not escape
	// the quote.
	reads_as(arena, "[P]\nx = \"ends in \\\\\"\n", "P", "x", "ends in \\");
	// A Windows path in quotes is refused, not read as something else. The
	// refusal is on line 3, which the report names; ctest shows it.
	refused(arena, "[P]\n// a path\npath = \"C:\\Assets\"\n");
	// A backslash at the end of the line escapes nothing.
	refused(arena, "[P]\nx = \"a\\\n");
	// The escaped quote does not close the value, so nothing does.
	refused(arena, "[P]\nx = \"open \\\"\n");
	// Unquoted, a backslash is a byte and a quote is text.
	reads_as(arena, "[P]\nurl = http://a\\b\n", "P", "url", "http://a\\b");
	reads_as(arena, "[P]\nodd=x\"y\n", "P", "odd", "x\"y");
	// Quoted and unquoted say the same thing.
	reads_as(arena, "[P]\na=\"1.25\"\n", "P", "a", "1.25");
	reads_as(arena, "[P]\na=1.25\n", "P", "a", "1.25");
}

static void duplicates_are_refused(voe_base_arena *arena)
{
	refused(arena, "[A]\nx=1\nx=2\n");
	refused(arena, "[A]\nx=1\n[B]\n[A]\n");
	refused(arena, "[A]\n[A]\n");
	// The same key in two sections is two keys, and a dotted name is not
	// its parent.
	reads_as(arena, "[A]\nx=1\n[B]\nx=2\n", "B", "x", "2");
	accepted(arena, "[A]\nx=1\n[A.B]\nx=2\n");
	// Case is not folded.
	accepted(arena, "[A]\nx=1\nX=2\n");
	accepted(arena, "[A]\n[a]\n");
}

static void a_key_belongs_to_a_section(voe_base_arena *arena)
{
	refused(arena, "x=1\n");
	refused(arena, "x=1\n[A]\n");
	refused(arena, "// comment\n\nx=1\n");
}

// `//` starts a comment only at the start of a line. Everywhere else it is
// text, which is what keeps a URL whole whether it is quoted or not.
static void a_comment_is_a_whole_line(voe_base_arena *arena)
{
	reads_as(arena, "[P]\nname=\"http://example.com\"\n", "P", "name",
		 "http://example.com");
	reads_as(arena, "[P]\nurl=http://example.com\n", "P", "url",
		 "http://example.com");
	// There is no trailing comment: this is the value.
	reads_as(arena, "[P]\nhp=20 // health\n", "P", "hp", "20 // health");
	// A comment may be indented, and anything after // is ignored.
	reads_as(arena, "[P]\n   // a=1\nb=2\n", "P", "b", "2");
	reads_as(arena, "[P]\n////\nb=2\n", "P", "b", "2");
	// One slash is not a comment, and it is not a key either.
	refused(arena, "[P]\n/ not a comment\n");
}

// Nothing, and the several kinds of almost nothing.
static void empty_things_are_legal(voe_base_arena *arena)
{
	voe_assets_sectioned doc;
	uint32_t section;

	// An empty file, with the size honestly nought.
	VOE_TEST_CHECK(voe_assets_sectioned_parse("", 0, arena, &doc));
	VOE_TEST_CHECK_INT(doc.section_count, 0);
	VOE_TEST_CHECK_INT(doc.key_count, 0);
	VOE_TEST_CHECK_INT(voe_assets_sectioned_find(&doc, "A"),
			   VOE_ASSETS_SECTIONED_NONE);

	// Only blanks, only comments, only line endings.
	VOE_TEST_CHECK(parse(arena, "  \n\t\n", &doc));
	VOE_TEST_CHECK_INT(doc.section_count, 0);
	VOE_TEST_CHECK(parse(arena, "// one\n// two\n", &doc));
	VOE_TEST_CHECK_INT(doc.section_count, 0);
	VOE_TEST_CHECK(parse(arena, "\n\n\n", &doc));
	VOE_TEST_CHECK_INT(doc.section_count, 0);

	// A section with no keys, between two that have some.
	VOE_TEST_CHECK(parse(arena, "[A]\nx=1\n[B]\n[C]\ny=2\n", &doc));
	VOE_TEST_CHECK_INT(doc.section_count, 3);
	section = voe_assets_sectioned_find(&doc, "B");
	VOE_TEST_CHECK_INT(doc.sections[section].key_count, 0);
	VOE_TEST_CHECK_INT(doc.sections[section].first_key, 1);
	VOE_TEST_CHECK(voe_assets_sectioned_value(&doc, section, "x") == NULL);
	VOE_TEST_CHECK(voe_assets_sectioned_value(&doc, section, "y") == NULL);

	// A key with an empty value, both ways, is "" and never NULL.
	reads_as(arena, "[A]\nx=\n", "A", "x", "");
	reads_as(arena, "[A]\nx=\"\"\n", "A", "x", "");
	reads_as(arena, "[A]\nx=   \n", "A", "x", "");
}

// A section and a key carry the 1-based physical line they were read from —
// counting every line, comments and blanks included, and not just the ones
// that came back as something.
static void a_section_and_a_key_carry_their_line(voe_base_arena *arena)
{
	voe_assets_sectioned doc;

	// Across a blank line and a comment before the first section: the
	// section is not on line 1.
	VOE_TEST_CHECK(parse(arena,
			     "\n"
			     "// a comment\n"
			     "[A]\n"
			     "x=1\n",
			     &doc));
	VOE_TEST_CHECK_INT(doc.sections[0].line, 3);
	VOE_TEST_CHECK_INT(doc.keys[0].line, 4);

	// Across a blank line and a comment between the section and its key.
	VOE_TEST_CHECK(parse(arena,
			     "[A]\n"
			     "\n"
			     "// a comment\n"
			     "x=1\n",
			     &doc));
	VOE_TEST_CHECK_INT(doc.sections[0].line, 1);
	VOE_TEST_CHECK_INT(doc.keys[0].line, 4);

	// `\r\n` endings count the same as `\n`.
	VOE_TEST_CHECK(parse(arena, "[A]\r\nx=1\r\ny=2\r\n", &doc));
	VOE_TEST_CHECK_INT(doc.sections[0].line, 1);
	VOE_TEST_CHECK_INT(doc.keys[0].line, 2);
	VOE_TEST_CHECK_INT(doc.keys[1].line, 3);
}

// `\r\n` reads as `\n`, and the last line need not end at all.
static void line_endings(voe_base_arena *arena)
{
	reads_as(arena, "[A]\r\nx=1\r\n", "A", "x", "1");
	reads_as(arena, "[A]\r\nx=\"1\"\r\n", "A", "x", "1");
	reads_as(arena, "[A]\nx=1", "A", "x", "1");
	reads_as(arena, "[A]\nx=1\r", "A", "x", "1");
	accepted(arena, "[A]");

	// A carriage return in the middle of a line is text and not an ending.
	refused(arena, "[A]\rx=1\n");
}

static void broken_lines_are_refused(voe_base_arena *arena)
{
	refused(arena, "garbage\n");
	refused(arena, "[A]\ngarbage\n");
	refused(arena, "[A]\n=5\n");
	refused(arena, "[A]\n  =5\n");
	refused(arena, "[A\n");
	refused(arena, "[A] junk\n");
	refused(arena, "[A]x=1\n");
	refused(arena, "[]\n");
	refused(arena, "[ ]\n");
	refused(arena, "[A]\nx=\"open\n");
	refused(arena, "[A]\nx=\"a\"b\n");
	refused(arena, "[A]\nx=\"a\" \"b\"\n");
	refused(arena, "]\n");

	// A NUL byte, which the terminated strings could not carry. Passed with
	// its real length so the reader sees past it.
	{
		static const char nul[] = "[A]\nx=a\0b\n";
		voe_assets_sectioned doc;

		VOE_TEST_CHECK(!voe_assets_sectioned_parse(
			nul, sizeof(nul) - 1, arena, &doc));
	}
}

// The reader stops at `size`, not at the terminator, and sees nothing past it.
static void the_size_is_the_end(voe_base_arena *arena)
{
	const char *text = "[A]\nx=1\n[B]\ny=2\n";
	voe_assets_sectioned doc;

	VOE_TEST_CHECK(voe_assets_sectioned_parse(text, 8, arena, &doc));
	VOE_TEST_CHECK_INT(doc.section_count, 1);
	VOE_TEST_CHECK_INT(doc.key_count, 1);

	// Cut in the middle of a line: what is there is what is read.
	VOE_TEST_CHECK(voe_assets_sectioned_parse(text, 6, arena, &doc));
	VOE_TEST_CHECK_INT(doc.key_count, 1);
	VOE_TEST_CHECK(strcmp(doc.keys[0].value, "") == 0);
	VOE_TEST_CHECK(voe_assets_sectioned_parse(text, 7, arena, &doc));
	VOE_TEST_CHECK(strcmp(doc.keys[0].value, "1") == 0);
}

// Text is copied, so the caller's buffer may go the moment the reader returns.
static void the_bytes_are_not_kept(voe_base_arena *arena)
{
	char text[] = "[A]\nx=hello\n";
	voe_assets_sectioned doc;

	VOE_TEST_CHECK(voe_assets_sectioned_parse(text, strlen(text), arena,
						  &doc));
	memset(text, '#', sizeof(text) - 1);
	VOE_TEST_CHECK(strcmp(doc.sections[0].name, "A") == 0);
	VOE_TEST_CHECK(strcmp(doc.keys[0].name, "x") == 0);
	VOE_TEST_CHECK(strcmp(doc.keys[0].value, "hello") == 0);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(64 * 1024);

	the_sketch_comes_back_as_its_text(arena);
	a_name_has_at_most_two_parts(arena);
	blanks_are_tolerated_around_and_refused_inside(arena);
	a_quoted_value_has_two_escapes(arena);
	duplicates_are_refused(arena);
	a_key_belongs_to_a_section(arena);
	a_comment_is_a_whole_line(arena);
	empty_things_are_legal(arena);
	a_section_and_a_key_carry_their_line(arena);
	line_endings(arena);
	broken_lines_are_refused(arena);
	the_size_is_the_end(arena);
	the_bytes_are_not_kept(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
