// The decoder, and the two things about it that matter: it gets the right
// character out of a well-formed sequence, and it never stands still on a
// malformed one. Needs no graphics card.
//
// THE MALFORMED CASES ARE THE POINT OF THIS FILE. A decoder that returns zero
// bytes consumed turns a mangled string into a program that does not come back,
// and the string a text block is built from is whatever a caller wrote in a
// source file. Every case below therefore checks the length as well as the
// character.
#include <text/utf8.h>

#include <testing/test.h>

static void check_one(const char *bytes, uint32_t expected_length,
		      uint32_t expected)
{
	uint32_t codepoint = 0;
	uint32_t length = voe_text_utf8_next(bytes, &codepoint);

	VOE_TEST_CHECK_INT(length, expected_length);
	VOE_TEST_CHECK_INT(codepoint, expected);
}

static void check_well_formed(void)
{
	check_one("A", 1, 'A');
	check_one("\n", 1, '\n');
	// The last character one byte can hold, and the first that needs two.
	check_one("\x7f", 1, 0x7f);
	check_one("\xc2\x80", 2, 0x80);
	// é, ü and å — the three the atlas has to carry and the source files in
	// this repository are written in.
	check_one("é", 2, 0xe9);
	check_one("ü", 2, 0xfc);
	check_one("å", 2, 0xe5);
	// Three and four bytes. Nothing in this engine draws either yet, but the
	// decoder walks past them and has to walk past the right number of
	// bytes.
	check_one("\xe4\xb8\x80", 3, 0x4e00);
	check_one("\xf0\x9f\x98\x80", 4, 0x1f600);
}

static void check_malformed(void)
{
	// A continuation byte with nothing in front of it.
	check_one("\x80", 1, VOE_TEXT_UTF8_REPLACEMENT);
	// A two-byte lead with an ASCII byte after it: the sequence is broken
	// and the second byte is not consumed, so the next call reads it as the
	// character it is.
	check_one("\xc3z", 1, VOE_TEXT_UTF8_REPLACEMENT);
	// A lead byte at the very end of the string. The terminator is not a
	// continuation byte, so this stops rather than reading past it.
	check_one("\xe4", 1, VOE_TEXT_UTF8_REPLACEMENT);
	// Overlong: 'A' written in two bytes. Refused, because a comparison
	// against the one-byte form would not have matched it.
	check_one("\xc1\x81", 1, VOE_TEXT_UTF8_REPLACEMENT);
	// Overlong again, in three bytes, for the null character — the classic
	// one.
	check_one("\xe0\x80\x80", 1, VOE_TEXT_UTF8_REPLACEMENT);
	// A surrogate half, which UTF-8 may not encode.
	check_one("\xed\xa0\x80", 1, VOE_TEXT_UTF8_REPLACEMENT);
	// Above U+10FFFF, which is not a character at all.
	check_one("\xf5\x80\x80\x80", 1, VOE_TEXT_UTF8_REPLACEMENT);
	// Bytes that never appear in UTF-8.
	check_one("\xfe", 1, VOE_TEXT_UTF8_REPLACEMENT);
	check_one("\xff", 1, VOE_TEXT_UTF8_REPLACEMENT);
}

// The property the whole loop in text/src/font.c depends on: whatever the
// string holds, walking it ends.
static void check_a_walk_terminates(void)
{
	static const char rubbish[] = "a\xff\xc3z\xed\xa0\x80!\xf0\x9f\x98\x80";
	uint32_t characters = 0;

	for (const char *at = rubbish; *at != '\0';) {
		uint32_t codepoint;
		uint32_t length = voe_text_utf8_next(at, &codepoint);

		VOE_TEST_CHECK(length > 0);
		at += length;
		characters++;
		// Not a loop condition: a bound, so that a decoder which did
		// stand still fails rather than hangs.
		if (characters > sizeof rubbish)
			break;
	}
	VOE_TEST_CHECK_INT(characters, 9);
}

int main(void)
{
	check_well_formed();
	check_malformed();
	check_a_walk_terminates();
	return voe_test_result();
}
