// A JSON reader, internal to this folder. It exists because glTF's description
// is JSON and nothing is fetched (ADR-0023).
//
// IT PARSES INTO A FLAT ARRAY OF TOKENS AND BUILDS NO TREE OF POINTERS. Every
// value in the document becomes one token holding its kind, its span in the
// text, how many children it has, and where its subtree ends. Tokens are in
// document order, so a container's children are the tokens after it and the
// `next` field is what lets a walk step over a subtree it does not care about.
//
// THERE IS NO RECURSION IN IT, WHICH IS RULE 14 AND NOT A STYLE CHOICE. A
// recursive-descent reader is a stack overflow on deeply nested input, and that
// is a real crash on a corrupt or hostile file that neither -Werror nor the
// analyser will find. So the parser has an explicit stack, a named depth limit,
// and it refuses a document that nests past it.
//
// THE CALLER STATES HOW MANY TOKENS IT WILL ALLOW, for the same reason the
// inflate reader makes the caller state the output size: a small file must not
// be able to ask for a large allocation. A document with more values than that
// is refused as unsupported — it is a well-formed file this reader will not
// read, which is a different thing from a broken one.
//
// IT IS STRICT. A trailing comma, a missing colon, a key that is not a string, a
// number that is not a JSON number, a string with a bad escape in it, anything
// after the top-level value — all malformed. A reader that shrugged at those
// would be a reader that disagreed with every other JSON reader about what a
// file says, which is worse than refusing it.
//
// STRING SPANS ARE RAW AND ESCAPES ARE NOT EXPANDED. Escapes are validated so
// that the end of a string is found correctly, and then the span is the bytes
// between the quotes exactly as they appear. Everything this folder compares a
// string against — a glTF property name, an attribute name, an image's MIME type
// — is plain ASCII with no escape in it, so nothing needs the expansion and
// nothing here allocates for one.
#pragma once

#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// How deeply a document may nest. glTF's own structure is four or five deep;
// this is far above anything a real file has and far below anything that could
// trouble the stack — the stack is an array of this many uint32_t, on the
// parser's own frame.
#define VOE_ASSETS_JSON_MAX_DEPTH 64

// What a token index holds when there is no such value: a member that is not
// there, an element past the end of an array.
#define VOE_ASSETS_JSON_NONE UINT32_MAX

typedef enum {
	VOE_ASSETS_JSON_OBJECT,
	VOE_ASSETS_JSON_ARRAY,
	VOE_ASSETS_JSON_STRING,
	VOE_ASSETS_JSON_NUMBER,
	VOE_ASSETS_JSON_TRUE,
	VOE_ASSETS_JSON_FALSE,
	VOE_ASSETS_JSON_NULL,
} voe_assets_json_kind;

// One value. `start` and `end` are byte offsets into the text: for a string they
// are the span between the quotes, and for everything else the whole of the
// value. `count` is how many members an object has or how many elements an array
// has — a key and its value are one member, not two. `next` is the index one
// past this value's entire subtree, which is where a sibling begins.
typedef struct {
	uint32_t kind;
	uint32_t start;
	uint32_t end;
	uint32_t count;
	uint32_t next;
} voe_assets_json_token;

typedef struct {
	const char *text;
	size_t size;
	const voe_assets_json_token *tokens;
	uint32_t count;
} voe_assets_json;

// Token 0 is the document's top-level value when this succeeds.
[[nodiscard]] bool voe_assets_json_parse(const char *text, size_t size,
					 voe_base_arena *arena,
					 uint32_t max_tokens,
					 voe_assets_json *out,
					 voe_base_error *error);

// The value of the named member, or VOE_ASSETS_JSON_NONE. Asserts if the token
// is not an object — asking an array for a member by name is the caller's bug.
uint32_t voe_assets_json_member(const voe_assets_json *json, uint32_t object,
				const char *name);

// The element at that position, or VOE_ASSETS_JSON_NONE when the array is
// shorter than that. Asserts if the token is not an array.
uint32_t voe_assets_json_element(const voe_assets_json *json, uint32_t array,
				 uint32_t index);

// How many members or elements. Zero for anything else, so a caller that
// expected a container and got a number reads a length of nought rather than
// wandering.
uint32_t voe_assets_json_count(const voe_assets_json *json, uint32_t token);

uint32_t voe_assets_json_kind_of(const voe_assets_json *json, uint32_t token);

// True when the token is a string whose raw span is exactly this. Escapes are
// not expanded, so a name written with one does not match — see the header.
bool voe_assets_json_is(const voe_assets_json *json, uint32_t token,
			const char *text);

// A number, as a double. False when the token is not a number or is longer than
// any JSON number needs to be, which is malformed either way.
[[nodiscard]] bool voe_assets_json_number(const voe_assets_json *json,
					  uint32_t token, double *out);

// A number that has to be a whole number in range. False when it is not one —
// which covers a negative index, a fractional count and a value past `limit`,
// all of which are malformed in a glTF and all of which have to be checked at
// every use.
[[nodiscard]] bool voe_assets_json_uint(const voe_assets_json *json,
					uint32_t token, uint32_t limit,
					uint32_t *out);

// A number as a float. False when the token is not a number.
[[nodiscard]] bool voe_assets_json_float(const voe_assets_json *json,
					 uint32_t token, float *out);
