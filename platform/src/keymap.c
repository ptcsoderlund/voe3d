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
// The XKB keysym value for ISO_Level3_Shift — AltGr — as a real compositor
// writes it in numeric form.
#define KEYSYM_ISO_LEVEL3_SHIFT 0xfe03

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

static bool is_hex_digit(char c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	       (c >= 'A' && c <= 'F');
}

// The next token, or false at the end of the text. Whitespace is skipped
// first; anything not recognised below becomes a one-character TOKEN_PUNCT,
// so the reader never gets stuck on a character it does not know — the
// callers that care about a specific punctuation mark check for it, and the
// ones that do not just skip past whatever came back.
//
// A NUMBER IS ONE OF TWO SPELLINGS: a plain decimal run, or `0x`/`0X`
// followed by hex digits consumed whole as a single token. Reading the `0x`
// prefix here, rather than leaving `x` to fall into an identifier the way a
// bare digit scanner would, is the fix for the bug this feature exists to
// close: without it, `0x71` lexes as the number `0` followed by the
// identifier `x71`, and every keysym written as a `0x` value reads as the
// keysym named `0`.
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

		if (c == '0' && lx->pos + 1 < lx->size &&
		    (lx->text[lx->pos + 1] == 'x' ||
		     lx->text[lx->pos + 1] == 'X')) {
			lx->pos += 2;
			while (lx->pos < lx->size &&
			      is_hex_digit(lx->text[lx->pos]))
				lx->pos++;
		} else {
			while (lx->pos < lx->size && is_digit(lx->text[lx->pos]))
				lx->pos++;
		}
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

static uint32_t parse_hex(const char *digits, size_t length)
{
	uint32_t value = 0;

	for (size_t i = 0; i < length; i++) {
		char c = digits[i];
		uint32_t d = c <= '9' ? (uint32_t)(c - '0') :
			    c <= 'F' ? (uint32_t)(c - 'A' + 10) :
					  (uint32_t)(c - 'a' + 10);

		value = value * 16 + d;
	}
	return value;
}

// A number token's text becomes a value in whichever of the two spellings it
// was lexed in (see next_token): `0x`/`0X` followed by hex digits, consumed
// here past the prefix, or a plain decimal run.
static uint32_t parse_number(const char *text, size_t length)
{
	if (length >= 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
		return parse_hex(text + 2, length - 2);
	return parse_uint(text, length);
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

// A keysym NAME becomes a code point through a fixed table, not a computed
// rule: the X11 Latin-1 names above, plus `U` followed by hex digits, which
// names a Unicode code point directly. Anything else is 0 — a dead key, a
// function key, a modifier, level three's own names, or a legacy keysym name
// this reader does not carry a table for.
static uint32_t keysym_lookup(const char *name)
{
	size_t length = strlen(name);

	if (name[0] == 'U' && all_hex(name + 1, length - 1))
		return (uint32_t)parse_hex(name + 1, length - 1);

	for (size_t i = 0; i < sizeof(latin1_names) / sizeof(latin1_names[0]);
	    i++) {
		if (strcmp(latin1_names[i].name, name) == 0)
			return latin1_names[i].code;
	}
	return 0;
}

// A keysym VALUE becomes a code point by range, not by table (keymap.h,
// and plan.md's Decisions): the two Latin-1 blocks are themselves, the
// Unicode form is itself minus its 0x01000000 marker, and everything else —
// every dead key and function key value, and the legacy keysym blocks
// between the two Latin-1 ranges and the Unicode form (Greek, Cyrillic, the
// currency block that holds the euro sign) — types nothing. Deliberately no
// table for those blocks: nothing in this feature calls for one, and `text`
// would draw them as the missing-glyph box regardless.
static uint32_t codepoint_from_value(uint32_t value)
{
	if (value >= 0x20 && value <= 0x7e)
		return value;
	if (value >= 0xa0 && value <= 0xff)
		return value;
	if (value >= 0x01000000 && value <= 0x0110ffff)
		return value - 0x01000000;
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
// block in this file does. N is read by parse_number, so a keycode written
// in hex is read the same as one written in decimal.
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
					parse_number(lx->text + num.start,
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

// True when a Group index token names Group 1, in either spelling this
// format uses: the identifier `Group1`, or the bare number `1`.
static bool token_is_group1(const lexer *lx, const token *t)
{
	if (t->kind == TOKEN_IDENT)
		return token_is(lx, t, "Group1");
	return t->kind == TOKEN_NUMBER && t->length == 1 &&
	      lx->text[t->start] == '1';
}

// Consumes a `[ sym, sym, ... ]` already past its own '[', keeps Group 1's
// four levels (plain, Shift, AltGr, Shift+AltGr) and writes them for code —
// or does nothing when code names no known key. A token spelled `0x...` is a
// keysym value and becomes a code point by range (codepoint_from_value);
// any other token is a keysym name and goes through keysym_lookup. Whichever
// form a level takes, it also flags code as a level-three shift (keymap.h)
// when it is the keysym value 0xfe03 or the name `ISO_Level3_Shift` — this
// runs whether or not that level types a character, which is how a key like
// AltGr itself gets the flag despite typing nothing.
// The existing fallback rule is kept and not extended: a list of exactly one
// entry types that entry shifted too; a third or fourth level is whatever
// the list held there, and nothing when the list held fewer entries.
static void read_symbol_list(lexer *lx, uint32_t code, voe_platform_keymap *out)
{
	token t;
	uint32_t level = 0;
	uint32_t values[VOE_PLATFORM_KEYMAP_LEVELS] = { 0 };
	bool have_first = false;

	while (next_token(lx, &t)) {
		if (token_is_punct(&t, ']'))
			break;
		if (token_is_punct(&t, ','))
			continue;
		if (t.kind == TOKEN_IDENT || t.kind == TOKEN_NUMBER) {
			char span[KEYSYM_CAPACITY];
			bool is_value;
			uint32_t raw = 0;
			uint32_t value;

			copy_span(lx, &t, span, KEYSYM_CAPACITY);
			is_value = span[0] == '0' &&
				  (span[1] == 'x' || span[1] == 'X');
			if (is_value) {
				raw = parse_number(span, strlen(span));
				value = codepoint_from_value(raw);
			} else {
				value = keysym_lookup(span);
			}

			if (code < VOE_PLATFORM_KEYMAP_CODES &&
			   ((is_value && raw == KEYSYM_ISO_LEVEL3_SHIFT) ||
			    (!is_value &&
			     strcmp(span, "ISO_Level3_Shift") == 0)))
				out->level3_shift[code] = true;

			if (level < VOE_PLATFORM_KEYMAP_LEVELS) {
				values[level] = value;
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
	out->typed[code][2] = values[2];
	out->typed[code][3] = values[3];
}

// Skips a `[ ... ]` already past its own '[' without reading it — a group or
// a statement's value this reader is not keeping.
static void skip_bracket(lexer *lx)
{
	token t;

	while (next_token(lx, &t)) {
		if (token_is_punct(&t, ']'))
			return;
	}
}

// After a statement identifier at body depth 1 — `symbols`, `type`, or any
// other — reads its optional `[ index ]` and the `=` that follows, leaving
// the lexer positioned at the value. *is_group1 is set when the index read
// was `Group1` or `1`; false for no index, any other index, or input this
// grammar does not expect. Returns false when the `=` this grammar requires
// is never found, in which case the caller abandons the statement where it
// stands, which happens only on truncated or malformed input.
static bool skip_statement_index_and_equals(lexer *lx, bool *is_group1)
{
	token t;

	*is_group1 = false;
	if (!next_token(lx, &t))
		return false;
	if (token_is_punct(&t, '[')) {
		token index_tok;

		if (!next_token(lx, &index_tok))
			return false;
		if (token_is_group1(lx, &index_tok))
			*is_group1 = true;
		if (!token_is_punct(&index_tok, ']'))
			skip_bracket(lx);
		if (!next_token(lx, &t))
			return false;
	}
	return token_is_punct(&t, '=');
}

// `key <NAME> { ... }`, already past its own '{'. code is the evdev code
// <NAME> resolved to, or VOE_PLATFORM_KEYMAP_CODES when it did not, in which
// case the body is still walked so the lexer stays in step but nothing is
// kept.
//
// A KEY BODY IS READ BY ITS STATEMENTS, NOT BY THE FIRST '['. At depth 1, a
// '[' with no identifier before it is the bare symbol list — the only
// spelling this format has with no keyword at all. Any other identifier —
// `symbols`, `type`, `virtualMods`, `repeat`, `actions`, ... — starts a
// statement of the shape `identifier[ index ]= value` or
// `identifier= value`; every identifier but `symbols` has its optional index,
// its '=' and its value (a bracketed list, skipped, or one token, already
// consumed by reading it) swallowed unread. This is what stops a statement
// such as `type[Group1]= "..."` from being misread as the bare symbol list
// the way a `[` alone used to trigger it regardless of what came before.
// `symbols` reads its own bracketed value as the Group 1 list when its index
// is `Group1` or `1`, and skips it otherwise. Stops once one Group 1 list has
// been found, in either spelling, and otherwise skips the rest of the body
// the same way an unrecognised block is skipped.
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

		if (t.kind == TOKEN_IDENT) {
			bool is_symbols = token_is(lx, &t, "symbols");
			bool is_group1;
			token value;

			if (!skip_statement_index_and_equals(lx, &is_group1))
				continue;
			if (!next_token(lx, &value))
				continue;
			if (token_is_punct(&value, '[')) {
				if (is_symbols && is_group1) {
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

// True when at least one code types at least one level — the condition that
// separates a keymap this reader can turn into typed characters from one
// that, despite having an `xkb_symbols` block, resolves to nothing at all
// (keymap.h).
static bool any_key_typed(const voe_platform_keymap *map)
{
	for (size_t code = 0; code < VOE_PLATFORM_KEYMAP_CODES; code++) {
		for (size_t level = 0; level < VOE_PLATFORM_KEYMAP_LEVELS;
		    level++) {
			if (map->typed[code][level] != 0)
				return true;
		}
	}
	return false;
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

	if (!found_symbols || !any_key_typed(out)) {
		memset(out, 0, sizeof(*out));
		return false;
	}
	return true;
}
