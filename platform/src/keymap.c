// The XKB keymap reader. See keymap.h for what it reads and why there is no
// recursion in it.
//
// THE SHAPE: one lexer (a cursor over the text, one token at a time) and a
// handful of small parsing functions, each responsible for exactly one level
// of the format's fixed nesting — a block, a key, a bracket — and none of
// them calling itself or calling back up. The top-level scan does not track
// nesting at all: it reads one token at a time from the whole file and acts
// only on the two keywords it knows, `xkb_keycodes` and `xkb_symbols`,
// wherever they sit; everything else, including the `xkb_keymap { ... }`
// that wraps a real keymap, is a token it reads and does nothing with. Once
// one of those two keywords is found, the function that reads its block
// consumes exactly its own contents by counting `{`/`}`, which is what
// stands in for recursion here, bounded by the length of the text and never
// by how deep the text nests.
//
// TOKENS ARE NEVER PUT BACK. Each parsing function reads exactly as many
// tokens as its own grammar needs and, the moment one does not fit, treats it
// as the start of whatever the enclosing loop reads next. That is what keeps
// this a single forward pass: no lookahead buffer, no backtracking.
#include "keymap.h"

#include <base/assert.h>

#include <stdlib.h>
#include <string.h>

// A keycode name ("<AC101>" and its like) never runs past this; the longest
// in real keymaps is five or six characters.
#define NAME_CAPACITY 16
// The longest X11 keysym name ("guillemotright", "questiondown") is under 20
// characters; this leaves headroom without inviting a large stack buffer.
#define KEYSYM_CAPACITY 32
// How many names an `xkb_keycodes` block may declare before this reader stops
// keeping more. Real keymaps declare on the order of 200; this is generous
// over that without being unbounded.
#define MAX_NAMES 512
// How many `alias` statements one block may hold before this reader stops
// keeping more.
#define MAX_ALIASES 128

typedef enum {
	TOKEN_IDENT,
	TOKEN_KEYNAME,
	TOKEN_NUMBER,
	TOKEN_STRING,
	TOKEN_PUNCT,
} token_kind;

typedef struct {
	token_kind kind;
	size_t start;
	size_t length;
	char punct;
} token;

typedef struct {
	const char *text;
	size_t size;
	size_t pos;
} lexer;

typedef struct {
	char name[NAME_CAPACITY];
	uint32_t code;
} name_entry;

typedef struct {
	char alias[NAME_CAPACITY];
	char target[NAME_CAPACITY];
} alias_entry;

typedef struct {
	name_entry names[MAX_NAMES];
	uint32_t name_count;
	alias_entry aliases[MAX_ALIASES];
	uint32_t alias_count;
} name_table;

static bool is_ident_start(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_ident_char(char c)
{
	return is_ident_start(c) || (c >= '0' && c <= '9');
}

static bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

// The next token, or false at the end of the text. Whitespace is skipped
// first; anything not recognised below becomes a one-character TOKEN_PUNCT,
// so the reader never gets stuck on a character it does not know — the
// callers that care about a specific punctuation mark check for it, and the
// ones that do not just skip past whatever came back.
static bool next_token(lexer *lx, token *out)
{
	char c;

	while (lx->pos < lx->size) {
		c = lx->text[lx->pos];
		if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
			lx->pos++;
		else
			break;
	}
	if (lx->pos >= lx->size)
		return false;

	c = lx->text[lx->pos];

	if (c == '<') {
		size_t start = ++lx->pos;

		while (lx->pos < lx->size && lx->text[lx->pos] != '>')
			lx->pos++;
		out->kind = TOKEN_KEYNAME;
		out->start = start;
		out->length = lx->pos - start;
		if (lx->pos < lx->size)
			lx->pos++; // the closing '>'
		return true;
	}

	if (c == '"') {
		size_t start = ++lx->pos;

		while (lx->pos < lx->size && lx->text[lx->pos] != '"')
			lx->pos++;
		out->kind = TOKEN_STRING;
		out->start = start;
		out->length = lx->pos - start;
		if (lx->pos < lx->size)
			lx->pos++; // the closing '"'
		return true;
	}

	if (is_ident_start(c)) {
		size_t start = lx->pos;

		while (lx->pos < lx->size && is_ident_char(lx->text[lx->pos]))
			lx->pos++;
		out->kind = TOKEN_IDENT;
		out->start = start;
		out->length = lx->pos - start;
		return true;
	}

	if (is_digit(c)) {
		size_t start = lx->pos;

		while (lx->pos < lx->size && is_digit(lx->text[lx->pos]))
			lx->pos++;
		out->kind = TOKEN_NUMBER;
		out->start = start;
		out->length = lx->pos - start;
		return true;
	}

	out->kind = TOKEN_PUNCT;
	out->start = lx->pos;
	out->length = 1;
	out->punct = c;
	lx->pos++;
	return true;
}

static bool token_is(const lexer *lx, const token *t, const char *text)
{
	size_t length = strlen(text);

	return t->length == length && memcmp(lx->text + t->start, text, length) == 0;
}

static bool token_is_punct(const token *t, char c)
{
	return t->kind == TOKEN_PUNCT && t->punct == c;
}

static void copy_span(const lexer *lx, const token *t, char *out, size_t capacity)
{
	size_t length = t->length < capacity - 1 ? t->length : capacity - 1;

	memcpy(out, lx->text + t->start, length);
	out[length] = '\0';
}

static uint32_t parse_uint(const char *digits, size_t length)
{
	uint32_t value = 0;

	for (size_t i = 0; i < length; i++)
		value = value * 10 + (uint32_t)(digits[i] - '0');
	return value;
}

static bool is_hex_digit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	       (c >= 'A' && c <= 'F');
}

static bool all_hex(const char *text, size_t length)
{
	if (length == 0)
		return false;
	for (size_t i = 0; i < length; i++) {
		if (!is_hex_digit(text[i]))
			return false;
	}
	return true;
}

// The X11 Latin-1 keysym names, 0x20-0x7e and 0xa0-0xff, spelled the way
// libxkbcommon writes them. Anything not here, plus every dead key, function
// key and modifier name, is not a character this reader turns into one.
static const struct {
	const char *name;
	uint32_t code;
} latin1_names[] = {
	{ "space", 0x20 }, { "exclam", 0x21 }, { "quotedbl", 0x22 },
	{ "numbersign", 0x23 }, { "dollar", 0x24 }, { "percent", 0x25 },
	{ "ampersand", 0x26 }, { "apostrophe", 0x27 }, { "parenleft", 0x28 },
	{ "parenright", 0x29 }, { "asterisk", 0x2a }, { "plus", 0x2b },
	{ "comma", 0x2c }, { "minus", 0x2d }, { "period", 0x2e },
	{ "slash", 0x2f }, { "0", 0x30 }, { "1", 0x31 }, { "2", 0x32 },
	{ "3", 0x33 }, { "4", 0x34 }, { "5", 0x35 }, { "6", 0x36 },
	{ "7", 0x37 }, { "8", 0x38 }, { "9", 0x39 }, { "colon", 0x3a },
	{ "semicolon", 0x3b }, { "less", 0x3c }, { "equal", 0x3d },
	{ "greater", 0x3e }, { "question", 0x3f }, { "at", 0x40 },
	{ "A", 0x41 }, { "B", 0x42 }, { "C", 0x43 }, { "D", 0x44 },
	{ "E", 0x45 }, { "F", 0x46 }, { "G", 0x47 }, { "H", 0x48 },
	{ "I", 0x49 }, { "J", 0x4a }, { "K", 0x4b }, { "L", 0x4c },
	{ "M", 0x4d }, { "N", 0x4e }, { "O", 0x4f }, { "P", 0x50 },
	{ "Q", 0x51 }, { "R", 0x52 }, { "S", 0x53 }, { "T", 0x54 },
	{ "U", 0x55 }, { "V", 0x56 }, { "W", 0x57 }, { "X", 0x58 },
	{ "Y", 0x59 }, { "Z", 0x5a }, { "bracketleft", 0x5b },
	{ "backslash", 0x5c }, { "bracketright", 0x5d },
	{ "asciicircum", 0x5e }, { "underscore", 0x5f }, { "grave", 0x60 },
	{ "a", 0x61 }, { "b", 0x62 }, { "c", 0x63 }, { "d", 0x64 },
	{ "e", 0x65 }, { "f", 0x66 }, { "g", 0x67 }, { "h", 0x68 },
	{ "i", 0x69 }, { "j", 0x6a }, { "k", 0x6b }, { "l", 0x6c },
	{ "m", 0x6d }, { "n", 0x6e }, { "o", 0x6f }, { "p", 0x70 },
	{ "q", 0x71 }, { "r", 0x72 }, { "s", 0x73 }, { "t", 0x74 },
	{ "u", 0x75 }, { "v", 0x76 }, { "w", 0x77 }, { "x", 0x78 },
	{ "y", 0x79 }, { "z", 0x7a }, { "braceleft", 0x7b }, { "bar", 0x7c },
	{ "braceright", 0x7d }, { "asciitilde", 0x7e },
	{ "nobreakspace", 0xa0 }, { "exclamdown", 0xa1 }, { "cent", 0xa2 },
	{ "sterling", 0xa3 }, { "currency", 0xa4 }, { "yen", 0xa5 },
	{ "brokenbar", 0xa6 }, { "section", 0xa7 }, { "diaeresis", 0xa8 },
	{ "copyright", 0xa9 }, { "ordfeminine", 0xaa },
	{ "guillemotleft", 0xab }, { "notsign", 0xac }, { "hyphen", 0xad },
	{ "registered", 0xae }, { "macron", 0xaf }, { "degree", 0xb0 },
	{ "plusminus", 0xb1 }, { "twosuperior", 0xb2 },
	{ "threesuperior", 0xb3 }, { "acute", 0xb4 }, { "mu", 0xb5 },
	{ "paragraph", 0xb6 }, { "periodcentered", 0xb7 }, { "cedilla", 0xb8 },
	{ "onesuperior", 0xb9 }, { "masculine", 0xba },
	{ "guillemotright", 0xbb }, { "onequarter", 0xbc },
	{ "onehalf", 0xbd }, { "threequarters", 0xbe },
	{ "questiondown", 0xbf }, { "Agrave", 0xc0 }, { "Aacute", 0xc1 },
	{ "Acircumflex", 0xc2 }, { "Atilde", 0xc3 }, { "Adiaeresis", 0xc4 },
	{ "Aring", 0xc5 }, { "AE", 0xc6 }, { "Ccedilla", 0xc7 },
	{ "Egrave", 0xc8 }, { "Eacute", 0xc9 }, { "Ecircumflex", 0xca },
	{ "Ediaeresis", 0xcb }, { "Igrave", 0xcc }, { "Iacute", 0xcd },
	{ "Icircumflex", 0xce }, { "Idiaeresis", 0xcf }, { "ETH", 0xd0 },
	{ "Ntilde", 0xd1 }, { "Ograve", 0xd2 }, { "Oacute", 0xd3 },
	{ "Ocircumflex", 0xd4 }, { "Otilde", 0xd5 }, { "Odiaeresis", 0xd6 },
	{ "multiply", 0xd7 }, { "Oslash", 0xd8 }, { "Ugrave", 0xd9 },
	{ "Uacute", 0xda }, { "Ucircumflex", 0xdb }, { "Udiaeresis", 0xdc },
	{ "Yacute", 0xdd }, { "THORN", 0xde }, { "ssharp", 0xdf },
	{ "agrave", 0xe0 }, { "aacute", 0xe1 }, { "acircumflex", 0xe2 },
	{ "atilde", 0xe3 }, { "adiaeresis", 0xe4 }, { "aring", 0xe5 },
	{ "ae", 0xe6 }, { "ccedilla", 0xe7 }, { "egrave", 0xe8 },
	{ "eacute", 0xe9 }, { "ecircumflex", 0xea }, { "ediaeresis", 0xeb },
	{ "igrave", 0xec }, { "iacute", 0xed }, { "icircumflex", 0xee },
	{ "idiaeresis", 0xef }, { "eth", 0xf0 }, { "ntilde", 0xf1 },
	{ "ograve", 0xf2 }, { "oacute", 0xf3 }, { "ocircumflex", 0xf4 },
	{ "otilde", 0xf5 }, { "odiaeresis", 0xf6 }, { "division", 0xf7 },
	{ "oslash", 0xf8 }, { "ugrave", 0xf9 }, { "uacute", 0xfa },
	{ "ucircumflex", 0xfb }, { "udiaeresis", 0xfc }, { "yacute", 0xfd },
	{ "thorn", 0xfe }, { "ydiaeresis", 0xff },
};

// A keysym name to the code point it types, or 0 for a name this reader does
// not know — a dead key, a function key, a modifier, level three's own names.
// The two hexadecimal forms name a Unicode code point directly and are
// checked before the table, because a name like "U0041" is not in it.
static uint32_t keysym_lookup(const char *name)
{
	size_t length = strlen(name);

	if (name[0] == 'U' && all_hex(name + 1, length - 1))
		return (uint32_t)strtoul(name + 1, NULL, 16);

	if (length > 6 && memcmp(name, "0x0100", 6) == 0 &&
	    all_hex(name + 6, length - 6))
		return (uint32_t)strtoul(name + 6, NULL, 16);

	for (size_t i = 0; i < sizeof(latin1_names) / sizeof(latin1_names[0]);
	    i++) {
		if (strcmp(latin1_names[i].name, name) == 0)
			return latin1_names[i].code;
	}
	return 0;
}

// Consumes tokens until the '{' that opens the block this identifier named,
// skipping the quoted name a real keymap gives it (`xkb_keycodes "evdev"`).
// Consumes the '{' itself. Leaves the lexer at the end of the text with
// nothing consumed by the caller when no '{' is ever found, which happens
// only on truncated or malformed input.
static void skip_to_open_brace(lexer *lx)
{
	token t;

	while (next_token(lx, &t)) {
		if (token_is_punct(&t, '{'))
			return;
	}
}

static void add_name(name_table *table, const lexer *lx, const token *name,
		     uint32_t code)
{
	if (table->name_count == MAX_NAMES)
		return;
	copy_span(lx, name, table->names[table->name_count].name, NAME_CAPACITY);
	table->names[table->name_count].code = code;
	table->name_count++;
}

static void add_alias(name_table *table, const lexer *lx, const token *alias,
		      const token *target)
{
	if (table->alias_count == MAX_ALIASES)
		return;
	copy_span(lx, alias, table->aliases[table->alias_count].alias,
		 NAME_CAPACITY);
	copy_span(lx, target, table->aliases[table->alias_count].target,
		 NAME_CAPACITY);
	table->alias_count++;
}

static bool find_code_by_name(const name_table *table, const char *name,
			      uint32_t *out)
{
	for (uint32_t i = 0; i < table->name_count; i++) {
		if (strcmp(table->names[i].name, name) == 0) {
			*out = table->names[i].code;
			return true;
		}
	}
	return false;
}

static bool find_code(const name_table *table, const lexer *lx,
		      const token *name, uint32_t *out)
{
	char buffer[NAME_CAPACITY];

	copy_span(lx, name, buffer, NAME_CAPACITY);
	return find_code_by_name(table, buffer, out);
}

// Turns every pending alias into a name entry sharing its target's code, once
// the block that declared both is fully read — an alias may point at a name
// declared later in the same block, so this cannot run alias by alias as
// they are seen.
static void resolve_aliases(name_table *table)
{
	for (uint32_t i = 0; i < table->alias_count; i++) {
		uint32_t code;

		if (!find_code_by_name(table, table->aliases[i].target, &code))
			continue;
		if (table->name_count == MAX_NAMES)
			return;
		memcpy(table->names[table->name_count].name,
		      table->aliases[i].alias, NAME_CAPACITY);
		table->names[table->name_count].code = code;
		table->name_count++;
	}
}

// `xkb_keycodes { ... }`, already past its own '{'. Reads `<NAME> = N;` and
// `alias <A> = <B>;` at the block's own depth and skips everything else,
// including any nested braces, by counting them the same way every other
// block in this file does.
static void parse_keycodes_block(lexer *lx, name_table *table)
{
	token t;
	int depth = 1;

	while (depth > 0 && next_token(lx, &t)) {
		if (token_is_punct(&t, '{')) {
			depth++;
			continue;
		}
		if (token_is_punct(&t, '}')) {
			depth--;
			continue;
		}
		if (depth != 1)
			continue;

		if (t.kind == TOKEN_KEYNAME) {
			token name = t;
			token eq, num;

			if (next_token(lx, &eq) && token_is_punct(&eq, '=') &&
			    next_token(lx, &num) && num.kind == TOKEN_NUMBER)
				add_name(table, lx, &name,
					parse_uint(lx->text + num.start,
						  num.length));
			continue;
		}

		if (t.kind == TOKEN_IDENT && token_is(lx, &t, "alias")) {
			token a, eq, b;

			if (next_token(lx, &a) && a.kind == TOKEN_KEYNAME &&
			    next_token(lx, &eq) && token_is_punct(&eq, '=') &&
			    next_token(lx, &b) && b.kind == TOKEN_KEYNAME)
				add_alias(table, lx, &a, &b);
			continue;
		}
	}
}

// Reads a `[ sym, sym, ... ]` already past its own '[', keeps the first two
// levels (Group 1's plain and shifted symbols) and skips the rest, and
// writes them for code — or does nothing when code names no known key.
static void read_symbol_list(lexer *lx, uint32_t code, voe_platform_keymap *out)
{
	token t;
	uint32_t level = 0;
	uint32_t values[2] = { 0, 0 };
	bool have_first = false;

	while (next_token(lx, &t)) {
		if (token_is_punct(&t, ']'))
			break;
		if (token_is_punct(&t, ','))
			continue;
		if (t.kind == TOKEN_IDENT || t.kind == TOKEN_NUMBER) {
			if (level < 2) {
				char name[KEYSYM_CAPACITY];

				copy_span(lx, &t, name, KEYSYM_CAPACITY);
				values[level] = keysym_lookup(name);
				if (level == 0)
					have_first = true;
			}
			level++;
		}
	}

	if (!have_first || code >= VOE_PLATFORM_KEYMAP_CODES)
		return;
	out->typed[code][0] = values[0];
	out->typed[code][1] = level >= 2 ? values[1] : values[0];
}

// Skips a `[ ... ]` already past its own '[' without reading it — a group
// this reader is not keeping, e.g. `symbols[Group2] = [ ... ]`.
static void skip_bracket(lexer *lx)
{
	token t;

	while (next_token(lx, &t)) {
		if (token_is_punct(&t, ']'))
			return;
	}
}

// `key <NAME> { ... }`, already past its own '{'. code is the evdev code
// <NAME> resolved to, or VOE_PLATFORM_KEYMAP_CODES when it did not, in which
// case the body is still walked so the lexer stays in step but nothing is
// kept.
// Stops reading symbols once one Group 1 list has been found, in either
// spelling the format allows (see keymap.h), and otherwise skips the rest of
// the body the same way an unrecognised block is skipped.
static void parse_key_body(lexer *lx, uint32_t code, voe_platform_keymap *out)
{
	token t;
	int depth = 1;
	bool matched = false;

	while (depth > 0 && next_token(lx, &t)) {
		if (token_is_punct(&t, '{')) {
			depth++;
			continue;
		}
		if (token_is_punct(&t, '}')) {
			depth--;
			continue;
		}
		if (depth != 1 || matched)
			continue;

		if (token_is_punct(&t, '[')) {
			read_symbol_list(lx, code, out);
			matched = true;
			continue;
		}

		if (t.kind == TOKEN_IDENT && token_is(lx, &t, "symbols")) {
			token open_index, group, close_index, eq, open_list;

			if (next_token(lx, &open_index) &&
			    token_is_punct(&open_index, '[') &&
			    next_token(lx, &group) &&
			    group.kind == TOKEN_IDENT &&
			    next_token(lx, &close_index) &&
			    token_is_punct(&close_index, ']') &&
			    next_token(lx, &eq) && token_is_punct(&eq, '=') &&
			    next_token(lx, &open_list) &&
			    token_is_punct(&open_list, '[')) {
				if (token_is(lx, &group, "Group1")) {
					read_symbol_list(lx, code, out);
					matched = true;
				} else {
					skip_bracket(lx);
				}
			}
			continue;
		}
	}
}

// `xkb_symbols { ... }`, already past its own '{'. Reads `key <NAME> { ... }`
// at the block's own depth and skips everything else the same way
// parse_keycodes_block does.
static void parse_symbols_block(lexer *lx, const name_table *table,
				voe_platform_keymap *out)
{
	token t;
	int depth = 1;

	while (depth > 0 && next_token(lx, &t)) {
		if (token_is_punct(&t, '{')) {
			depth++;
			continue;
		}
		if (token_is_punct(&t, '}')) {
			depth--;
			continue;
		}
		if (depth != 1)
			continue;

		if (t.kind == TOKEN_IDENT && token_is(lx, &t, "key")) {
			token name, brace;

			if (next_token(lx, &name) &&
			    name.kind == TOKEN_KEYNAME &&
			    next_token(lx, &brace) &&
			    token_is_punct(&brace, '{')) {
				uint32_t xkb_code;
				uint32_t code = VOE_PLATFORM_KEYMAP_CODES;

				// The keycodes block gives the XKB keycode;
				// wl_keyboard.key and this reader's table both
				// index by evdev code, which is that minus 8
				// (see keymap.h).
				if (find_code(table, lx, &name, &xkb_code) &&
				    xkb_code >= 8)
					code = xkb_code - 8;
				parse_key_body(lx, code, out);
			}
			continue;
		}
	}
}

bool voe_platform_keymap_read(const char *text, size_t size,
			      voe_platform_keymap *out)
{
	lexer lx = { .text = text, .size = size, .pos = 0 };
	name_table table = { 0 };
	token t;
	bool found_symbols = false;

	VOE_BASE_ASSERT(text != NULL, "reading a keymap with no text");
	VOE_BASE_ASSERT(out != NULL, "reading a keymap into no output");

	memset(out, 0, sizeof(*out));

	// A real keymap wraps every block in `xkb_keymap { ... };`, but nothing
	// here needs to know that: the two blocks this reader cares about are
	// found by their keyword wherever it sits in the token stream, each
	// consumes exactly its own contents once found (including its own
	// nested braces, counted inside parse_keycodes_block and
	// parse_symbols_block), and everything else — the wrapping braces,
	// `xkb_types`, `xkb_compat`, `xkb_geometry`, every quoted name — is a
	// token this loop reads and does nothing with.
	while (next_token(&lx, &t)) {
		if (t.kind == TOKEN_IDENT && token_is(&lx, &t, "xkb_keycodes")) {
			skip_to_open_brace(&lx);
			parse_keycodes_block(&lx, &table);
			resolve_aliases(&table);
			continue;
		}
		if (t.kind == TOKEN_IDENT && token_is(&lx, &t, "xkb_symbols")) {
			skip_to_open_brace(&lx);
			parse_symbols_block(&lx, &table, out);
			found_symbols = true;
			continue;
		}
	}

	return found_symbols;
}
