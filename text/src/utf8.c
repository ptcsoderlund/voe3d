// The decoder. See utf8.h for what it refuses and why it always advances.
#include <text/utf8.h>

#include <base/assert.h>

#include <stddef.h>

// The smallest character each sequence length is allowed to spell. A sequence
// that spells something below its own entry is overlong — the same character
// written the long way — and is refused.
static const uint32_t least[5] = { 0, 0, 0x80, 0x800, 0x10000 };

static uint32_t replacement(uint32_t *codepoint)
{
	*codepoint = VOE_TEXT_UTF8_REPLACEMENT;
	return 1;
}

uint32_t voe_text_utf8_next(const char *at, uint32_t *codepoint)
{
	const uint8_t *bytes = (const uint8_t *)at;
	uint8_t lead = bytes[0];
	uint32_t length;
	uint32_t value;

	VOE_BASE_ASSERT(at != NULL, "no string to decode a character out of");
	VOE_BASE_ASSERT(codepoint != NULL, "nowhere to put the character");

	if (lead < 0x80) {
		*codepoint = lead;
		return 1;
	}
	// 10xxxxxx is a continuation byte and cannot lead a sequence; the four
	// values above 0xf4 were never legal and 0xfe and 0xff never appear in
	// UTF-8 at all.
	if (lead < 0xc0 || lead > 0xf4)
		return replacement(codepoint);

	if (lead < 0xe0) {
		length = 2;
		value = lead & 0x1fu;
	} else if (lead < 0xf0) {
		length = 3;
		value = lead & 0x0fu;
	} else {
		length = 4;
		value = lead & 0x07u;
	}

	for (uint32_t i = 1; i < length; i++) {
		// The terminator ends the string, and it is not a continuation
		// byte, so a truncated sequence at the end stops here rather
		// than reading past the end of the buffer.
		if ((bytes[i] & 0xc0u) != 0x80u)
			return replacement(codepoint);
		value = value << 6 | (bytes[i] & 0x3fu);
	}

	if (value < least[length] || value > 0x10ffffu ||
	    (value >= 0xd800u && value <= 0xdfffu))
		return replacement(codepoint);

	*codepoint = value;
	return length;
}
