// UTF-8, decoded one character at a time, and about as small as a decoder gets:
// laying a string out means walking it, and a string in this engine is UTF-8
// because a C source file is.
//
// IT IS PUBLIC BECAUSE A CALLER PLACING CHARACTERS ITSELF HAS TO WALK A STRING
// THE WAY THIS FOLDER DOES. voe_text_font_measure decodes the whole string and
// voe_text_font_glyph takes a codepoint, so a caller that stepped over bytes
// between the two would measure one width and draw another the moment a
// character took more than one byte — a label laid out for two letters and drawn
// as four. One decoder on both sides of that is what keeps them agreeing, and
// `ui` is the caller it was made public for.
//
// IT NEVER STOPS AND IT NEVER STANDS STILL. A byte that is not part of a
// well-formed sequence is one character — U+FFFD, the replacement character —
// and is stepped over. So a loop over a string ends whatever the string holds,
// which is the property that matters: a decoder that returns nothing on bad
// input turns a mangled string into a hung program.
//
// IT REFUSES THE THREE ENCODINGS THAT ARE THE CLASSIC WAY IN. An overlong
// sequence (a character written in more bytes than it needs, which is how a
// forbidden character is smuggled past a comparison), a surrogate half, and
// anything above U+10FFFF are each U+FFFD rather than the number they spell.
// Nothing downstream then has to know they exist.
#pragma once

#include <stdint.h>

// The character U+FFFD, which is what every malformed sequence becomes. It is
// also what the font will not have a glyph for, so bad input shows as the
// font's missing-glyph box rather than as nothing at all.
#define VOE_TEXT_UTF8_REPLACEMENT 0xfffdu

// Reads one character from `at`, which must not be empty, and writes it to
// `codepoint`. Comes back with how many bytes it used, which is never zero, so
// `at += voe_text_utf8_next(at, &c)` always advances.
uint32_t voe_text_utf8_next(const char *at, uint32_t *codepoint);
