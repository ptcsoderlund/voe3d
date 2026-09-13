// The JSON reader. A state machine with an explicit stack, one token per value,
// and nothing tolerated.
//
// THE STATE MACHINE IS THE WHOLE OF THE STRICTNESS. Where a recursive parser
// would express "a member is a string, a colon and a value" as a function call,
// this expresses it as three states, and there is no path through them that
// accepts a trailing comma or a missing colon. The stack holds one token index
// per open container and nothing else; how deep it may go is
// VOE_ASSETS_JSON_MAX_DEPTH, and a document that goes deeper is refused rather
// than followed.
//
// `next` IS FILLED IN WHEN A CONTAINER CLOSES, WHICH IS WHY TOKENS COME OUT IN
// DOCUMENT ORDER. At the moment a container closes, every token inside it has
// already been allocated, so the count of tokens so far is exactly one past its
// subtree. A scalar's `next` is its own index plus one, because a scalar has no
// subtree.
//
// KEYS ARE TOKENS BUT NOT MEMBERS. A key's token sits immediately before its
// value's, and an object's `count` counts values — so a lookup steps key, value,
// key, value using `next`, and a caller never has to know whether a count means
// members or tokens.
//
// EVERY LENGTH READ OUT OF THE TEXT IS CHECKED AGAINST THE END OF THE TEXT
// BEFORE IT IS USED. The text is not NUL-terminated: it is a span inside a
// larger buffer — the JSON chunk of a `.glb` — so every read is bounded by
// `size` and never by a terminator. That is the one thing a reader written
// against a C string would get wrong here.
#include "json.h"

#include <base/assert.h>
#include <base/report.h>

#include <stdlib.h>
#include <string.h>

// Long enough for any number a JSON file needs — a double round-trips in 17
// significant digits, and this leaves room for a sign, a point and a
// three-digit exponent twice over.
#define NUMBER_MAX 64

// What the parser is waiting for.
enum state {
	// A value, in a place where one is required.
	WANT_VALUE,
	// A value or the end of the array it would be the first element of.
	WANT_VALUE_OR_ARRAY_END,
	// A key or the end of the object it would be the first member of.
	WANT_KEY_OR_OBJECT_END,
	// A key, after a comma inside an object.
	WANT_KEY,
	WANT_COLON,
	// A comma, or the end of whatever container is open.
	WANT_COMMA_OR_END,
	// The top-level value is complete; only whitespace may follow.
	DONE,
};

struct parser {
	const char *text;
	size_t size;
	size_t pos;

	voe_assets_json_token *tokens;
	uint32_t max_tokens;
	uint32_t count;

	uint32_t stack[VOE_ASSETS_JSON_MAX_DEPTH];
	uint32_t depth;

	enum state state;
	voe_base_error error;
};

static void fail(struct parser *parser, voe_base_error error,
		 const char *message)
{
	VOE_BASE_ERROR("assets", "JSON at byte %zu: %s", parser->pos, message);
	parser->error = error;
}

static bool malformed(struct parser *parser, const char *message)
{
	fail(parser, VOE_BASE_ERROR_MALFORMED, message);
	return false;
}

static void skip_whitespace(struct parser *parser)
{
	while (parser->pos < parser->size) {
		char c = parser->text[parser->pos];

		if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
			return;
		parser->pos++;
	}
}

// A token, and the parent's count if this one is a value rather than a key.
static uint32_t push_token(struct parser *parser, uint32_t kind, bool is_value)
{
	uint32_t index;

	if (parser->count == parser->max_tokens) {
		fail(parser, VOE_BASE_ERROR_UNSUPPORTED,
		     "more values than this reader was told it could hold");
		return VOE_ASSETS_JSON_NONE;
	}

	index = parser->count++;
	parser->tokens[index] = (voe_assets_json_token){
		.kind = kind,
		.start = (uint32_t)parser->pos,
		.end = (uint32_t)parser->pos,
		.count = 0,
		.next = index + 1,
	};

	if (is_value && parser->depth > 0)
		parser->tokens[parser->stack[parser->depth - 1]].count++;

	return index;
}

// What follows a completed value: either the document is done, or the container
// it was in wants a comma or its end.
static void after_value(struct parser *parser)
{
	parser->state = parser->depth == 0 ? DONE : WANT_COMMA_OR_END;
}

static bool open_container(struct parser *parser, uint32_t kind, bool is_value)
{
	uint32_t index;

	if (parser->depth == VOE_ASSETS_JSON_MAX_DEPTH)
		return malformed(parser,
				 "nested deeper than this reader will follow");

	index = push_token(parser, kind, is_value);
	if (index == VOE_ASSETS_JSON_NONE)
		return false;

	parser->stack[parser->depth++] = index;
	parser->pos++;
	parser->state = kind == VOE_ASSETS_JSON_OBJECT ?
				WANT_KEY_OR_OBJECT_END :
				WANT_VALUE_OR_ARRAY_END;
	return true;
}

static void close_container(struct parser *parser)
{
	uint32_t index = parser->stack[--parser->depth];

	parser->pos++;
	parser->tokens[index].end = (uint32_t)parser->pos;
	// Every token inside it has been allocated by now, so this is one past
	// its subtree.
	parser->tokens[index].next = parser->count;
	after_value(parser);
}

// The span between the quotes, with escapes validated but not expanded. pos is
// on the opening quote when this is called.
static bool read_string(struct parser *parser, bool is_value)
{
	uint32_t index;

	parser->pos++;
	index = push_token(parser, VOE_ASSETS_JSON_STRING, is_value);
	if (index == VOE_ASSETS_JSON_NONE)
		return false;

	while (parser->pos < parser->size) {
		unsigned char c = (unsigned char)parser->text[parser->pos];

		if (c == '"') {
			parser->tokens[index].end = (uint32_t)parser->pos;
			parser->pos++;
			return true;
		}
		// A control character has to be escaped in JSON, and accepting
		// a raw one is how two readers end up disagreeing about where a
		// string ended.
		if (c < 0x20)
			return malformed(parser,
					 "an unescaped control character inside a string");
		if (c != '\\') {
			parser->pos++;
			continue;
		}

		parser->pos++;
		if (parser->pos == parser->size)
			return malformed(parser,
					 "a string that ends in the middle of an escape");
		c = (unsigned char)parser->text[parser->pos];
		if (c == 'u') {
			// Four hex digits, and they have to be there. The
			// code point itself is not decoded: nothing this
			// folder compares needs it, and the span is raw.
			if (parser->size - parser->pos < 5)
				return malformed(parser,
						 "a \\u escape that runs off the end of the text");
			for (uint32_t i = 1; i <= 4; i++) {
				char digit = parser->text[parser->pos + i];

				if (!((digit >= '0' && digit <= '9') ||
				      (digit >= 'a' && digit <= 'f') ||
				      (digit >= 'A' && digit <= 'F')))
					return malformed(parser,
							 "a \\u escape with something that is not a hex digit in it");
			}
			parser->pos += 5;
			continue;
		}
		if (c != '"' && c != '\\' && c != '/' && c != 'b' && c != 'f' &&
		    c != 'n' && c != 'r' && c != 't')
			return malformed(parser, "an escape that is not one");
		parser->pos++;
	}

	return malformed(parser, "a string with no closing quote");
}

static bool is_digit(char c)
{
	return c >= '0' && c <= '9';
}

// -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)? — JSON's number, and no
// more than that. No leading plus, no leading zeros, no hexadecimal, no
// infinity: every one of those is something one reader accepts and another does
// not.
static bool read_number(struct parser *parser, bool is_value)
{
	uint32_t index = push_token(parser, VOE_ASSETS_JSON_NUMBER, is_value);

	if (index == VOE_ASSETS_JSON_NONE)
		return false;

	if (parser->pos < parser->size && parser->text[parser->pos] == '-')
		parser->pos++;

	if (parser->pos == parser->size || !is_digit(parser->text[parser->pos]))
		return malformed(parser, "a number with no digits in it");

	if (parser->text[parser->pos] == '0') {
		parser->pos++;
	} else {
		while (parser->pos < parser->size &&
		       is_digit(parser->text[parser->pos]))
			parser->pos++;
	}

	if (parser->pos < parser->size && parser->text[parser->pos] == '.') {
		parser->pos++;
		if (parser->pos == parser->size ||
		    !is_digit(parser->text[parser->pos]))
			return malformed(parser,
					 "a number with a point and no digits after it");
		while (parser->pos < parser->size &&
		       is_digit(parser->text[parser->pos]))
			parser->pos++;
	}

	if (parser->pos < parser->size &&
	    (parser->text[parser->pos] == 'e' ||
	     parser->text[parser->pos] == 'E')) {
		parser->pos++;
		if (parser->pos < parser->size &&
		    (parser->text[parser->pos] == '+' ||
		     parser->text[parser->pos] == '-'))
			parser->pos++;
		if (parser->pos == parser->size ||
		    !is_digit(parser->text[parser->pos]))
			return malformed(parser,
					 "a number with an exponent and no digits in it");
		while (parser->pos < parser->size &&
		       is_digit(parser->text[parser->pos]))
			parser->pos++;
	}

	parser->tokens[index].end = (uint32_t)parser->pos;
	return true;
}

static bool read_literal(struct parser *parser, const char *word,
			 uint32_t kind, bool is_value)
{
	size_t length = strlen(word);
	uint32_t index;

	if (parser->size - parser->pos < length ||
	    memcmp(parser->text + parser->pos, word, length) != 0)
		return malformed(parser, "something that is not a value");

	index = push_token(parser, kind, is_value);
	if (index == VOE_ASSETS_JSON_NONE)
		return false;

	parser->pos += length;
	parser->tokens[index].end = (uint32_t)parser->pos;
	return true;
}

// One value, in a place where a value is allowed. pos is on its first byte.
static bool read_value(struct parser *parser)
{
	char c = parser->text[parser->pos];

	switch (c) {
	case '{':
		return open_container(parser, VOE_ASSETS_JSON_OBJECT, true);
	case '[':
		return open_container(parser, VOE_ASSETS_JSON_ARRAY, true);
	case '"':
		if (!read_string(parser, true))
			return false;
		after_value(parser);
		return true;
	case 't':
		if (!read_literal(parser, "true", VOE_ASSETS_JSON_TRUE, true))
			return false;
		after_value(parser);
		return true;
	case 'f':
		if (!read_literal(parser, "false", VOE_ASSETS_JSON_FALSE, true))
			return false;
		after_value(parser);
		return true;
	case 'n':
		if (!read_literal(parser, "null", VOE_ASSETS_JSON_NULL, true))
			return false;
		after_value(parser);
		return true;
	default:
		if (!read_number(parser, true))
			return false;
		after_value(parser);
		return true;
	}
}

static bool step(struct parser *parser)
{
	char c = parser->text[parser->pos];
	bool object_open = parser->depth > 0 &&
			   parser->tokens[parser->stack[parser->depth - 1]].kind ==
				   VOE_ASSETS_JSON_OBJECT;

	switch (parser->state) {
	case WANT_VALUE:
		return read_value(parser);

	case WANT_VALUE_OR_ARRAY_END:
		if (c == ']') {
			close_container(parser);
			return true;
		}
		return read_value(parser);

	case WANT_KEY_OR_OBJECT_END:
		if (c == '}') {
			close_container(parser);
			return true;
		}
		if (c != '"')
			return malformed(parser,
					 "a member whose name is not a string");
		if (!read_string(parser, false))
			return false;
		parser->state = WANT_COLON;
		return true;

	case WANT_KEY:
		if (c != '"')
			return malformed(parser,
					 "a member whose name is not a string, or a trailing comma");
		if (!read_string(parser, false))
			return false;
		parser->state = WANT_COLON;
		return true;

	case WANT_COLON:
		if (c != ':')
			return malformed(parser,
					 "a member name with no colon after it");
		parser->pos++;
		parser->state = WANT_VALUE;
		return true;

	case WANT_COMMA_OR_END:
		if (c == ',') {
			parser->pos++;
			parser->state = object_open ? WANT_KEY : WANT_VALUE;
			return true;
		}
		if (c == '}' && object_open) {
			close_container(parser);
			return true;
		}
		if (c == ']' && !object_open) {
			close_container(parser);
			return true;
		}
		return malformed(parser,
				 "something that is neither a comma nor the end of the container it is in");

	case DONE:
		return malformed(parser,
				 "more than one value in the document");
	}

	return malformed(parser, "a reader that lost its place");
}

bool voe_assets_json_parse(const char *text, size_t size,
			   voe_base_arena *arena, uint32_t max_tokens,
			   voe_assets_json *out, voe_base_error *error)
{
	struct parser parser = {
		.text = text,
		.size = size,
		.max_tokens = max_tokens,
		.state = WANT_VALUE,
		.error = VOE_BASE_OK,
	};

	VOE_BASE_ASSERT(text != NULL, "parsing JSON from nothing");
	VOE_BASE_ASSERT(arena != NULL, "parsing JSON without an arena");
	VOE_BASE_ASSERT(out != NULL, "parsing JSON into nothing");
	VOE_BASE_ASSERT(max_tokens > 0, "parsing JSON with room for no values");

	// The whole array in one push: two pushes are not guaranteed to be
	// adjacent (base/arena.h), so a token array has to be one of them.
	parser.tokens = voe_base_arena_push(
		arena, (size_t)max_tokens * sizeof(*parser.tokens));

	for (;;) {
		skip_whitespace(&parser);
		if (parser.pos == parser.size)
			break;
		if (!step(&parser)) {
			if (error != NULL)
				*error = parser.error;
			return false;
		}
	}

	if (parser.state != DONE) {
		parser.error = VOE_BASE_OK;
		(void)malformed(&parser,
				"a document that ends in the middle of a value");
		if (error != NULL)
			*error = parser.error;
		return false;
	}

	out->text = text;
	out->size = size;
	out->tokens = parser.tokens;
	out->count = parser.count;
	return true;
}

static const voe_assets_json_token *at(const voe_assets_json *json,
				       uint32_t token)
{
	VOE_BASE_DEBUG_ASSERT(json != NULL, "reading no document");
	VOE_BASE_ASSERT(token < json->count,
			"a token index this document does not have");
	return &json->tokens[token];
}

uint32_t voe_assets_json_kind_of(const voe_assets_json *json, uint32_t token)
{
	return at(json, token)->kind;
}

uint32_t voe_assets_json_count(const voe_assets_json *json, uint32_t token)
{
	const voe_assets_json_token *value = at(json, token);

	if (value->kind != VOE_ASSETS_JSON_OBJECT &&
	    value->kind != VOE_ASSETS_JSON_ARRAY)
		return 0;
	return value->count;
}

bool voe_assets_json_is(const voe_assets_json *json, uint32_t token,
			const char *text)
{
	const voe_assets_json_token *value = at(json, token);
	size_t length = strlen(text);

	if (value->kind != VOE_ASSETS_JSON_STRING)
		return false;
	if (value->end - value->start != length)
		return false;
	return memcmp(json->text + value->start, text, length) == 0;
}

uint32_t voe_assets_json_member(const voe_assets_json *json, uint32_t object,
				const char *name)
{
	const voe_assets_json_token *value = at(json, object);
	uint32_t key = object + 1;

	VOE_BASE_ASSERT(value->kind == VOE_ASSETS_JSON_OBJECT,
			"asking for a member of something that is not an object");

	for (uint32_t i = 0; i < value->count; i++) {
		// The key, then its value immediately after it, then the next
		// key one past the value's subtree.
		uint32_t member = key + 1;

		if (voe_assets_json_is(json, key, name))
			return member;
		key = at(json, member)->next;
	}
	return VOE_ASSETS_JSON_NONE;
}

uint32_t voe_assets_json_element(const voe_assets_json *json, uint32_t array,
				 uint32_t index)
{
	const voe_assets_json_token *value = at(json, array);
	uint32_t element = array + 1;

	VOE_BASE_ASSERT(value->kind == VOE_ASSETS_JSON_ARRAY,
			"asking for an element of something that is not an array");

	if (index >= value->count)
		return VOE_ASSETS_JSON_NONE;

	for (uint32_t i = 0; i < index; i++)
		element = at(json, element)->next;
	return element;
}

bool voe_assets_json_number(const voe_assets_json *json, uint32_t token,
			    double *out)
{
	const voe_assets_json_token *value = at(json, token);
	char digits[NUMBER_MAX];
	size_t length;
	char *end = NULL;

	VOE_BASE_DEBUG_ASSERT(out != NULL, "reading a number into nothing");

	if (value->kind != VOE_ASSETS_JSON_NUMBER)
		return false;

	// Copied out because the text is a span and not a C string, and strtod
	// needs a terminator. The span has already been validated as a JSON
	// number by the parser, so the only thing that can go wrong here is the
	// length.
	length = value->end - value->start;
	if (length == 0 || length >= sizeof(digits))
		return false;
	memcpy(digits, json->text + value->start, length);
	digits[length] = '\0';

	*out = strtod(digits, &end);
	return end == digits + length;
}

bool voe_assets_json_uint(const voe_assets_json *json, uint32_t token,
			  uint32_t limit, uint32_t *out)
{
	double number;

	VOE_BASE_DEBUG_ASSERT(out != NULL, "reading an index into nothing");

	if (!voe_assets_json_number(json, token, &number))
		return false;
	if (number < 0.0 || number > (double)limit)
		return false;
	if (number != (double)(uint32_t)number)
		return false;

	*out = (uint32_t)number;
	return true;
}

bool voe_assets_json_float(const voe_assets_json *json, uint32_t token,
			   float *out)
{
	double number;

	VOE_BASE_DEBUG_ASSERT(out != NULL, "reading a number into nothing");

	if (!voe_assets_json_number(json, token, &number))
		return false;

	*out = (float)number;
	return true;
}
