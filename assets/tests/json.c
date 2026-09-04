// The JSON reader: that a document comes back as the values it holds, that
// navigation finds them, and that every shape of broken input is refused.
//
// THE REFUSALS ARE THE POINT OF THIS FILE. A reader that accepts a trailing
// comma or a missing colon disagrees with every other JSON reader about what a
// file says, and the way that shows up is a model that loads on one machine and
// not the next. Each case below is one thing a tolerant reader would let past.
//
// THE TEXT IS NEVER NUL-TERMINATED HERE, deliberately: a length is passed and
// the string literal's terminator is not counted, because the reader's real
// input is a span inside a `.glb` and a reader that leant on a terminator would
// pass this file and fail on a real model.
#include <base/arena.h>
#include <base/error.h>

#include "../src/json.h"

#include <testing/test.h>

#include <string.h>

#define TOKENS 256

static bool parse(voe_base_arena *arena, const char *text,
		  voe_assets_json *json, voe_base_error *error)
{
	return voe_assets_json_parse(text, strlen(text), arena, TOKENS, json,
				     error);
}

static void refused(voe_base_arena *arena, const char *text,
		    voe_base_error expected)
{
	voe_assets_json json;
	voe_base_error error = VOE_BASE_OK;
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);

	if (voe_assets_json_parse(text, strlen(text), arena, TOKENS, &json,
				  &error)) {
		// The text is printed so that a failure names the case rather
		// than a line number in a list of them.
		fprintf(stderr, "      accepted: %s\n", text);
		VOE_TEST_CHECK(false);
	} else {
		VOE_TEST_CHECK_INT(error, expected);
	}
	voe_base_arena_rewind(arena, mark);
}

static void a_document_comes_back_as_its_values(voe_base_arena *arena)
{
	const char *text =
		"{\"asset\":{\"version\":\"2.0\"},"
		"\"counts\":[1,2,3],"
		"\"scale\":-1.5e2,"
		"\"on\":true,\"off\":false,\"nothing\":null}";
	voe_assets_json json;
	voe_base_error error = VOE_BASE_OK;
	uint32_t asset;
	uint32_t counts;
	uint32_t token;
	float number;
	uint32_t index;

	VOE_TEST_CHECK(parse(arena, text, &json, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);

	// Six members, and a key with its value is one member and not two.
	VOE_TEST_CHECK_INT(voe_assets_json_kind_of(&json, 0),
			   VOE_ASSETS_JSON_OBJECT);
	VOE_TEST_CHECK_INT(voe_assets_json_count(&json, 0), 6);

	asset = voe_assets_json_member(&json, 0, "asset");
	VOE_TEST_CHECK(asset != VOE_ASSETS_JSON_NONE);
	VOE_TEST_CHECK_INT(voe_assets_json_count(&json, asset), 1);
	token = voe_assets_json_member(&json, asset, "version");
	VOE_TEST_CHECK(voe_assets_json_is(&json, token, "2.0"));
	VOE_TEST_CHECK(!voe_assets_json_is(&json, token, "2.1"));
	VOE_TEST_CHECK(!voe_assets_json_is(&json, token, "2.00"));

	// A member that is not there, from an object that has others: the case
	// every optional glTF property takes.
	VOE_TEST_CHECK_INT(voe_assets_json_member(&json, asset, "generator"),
			   VOE_ASSETS_JSON_NONE);

	// Past the last member of an object, which is where a lookup that
	// walked one token too far would wander.
	VOE_TEST_CHECK_INT(voe_assets_json_member(&json, 0, "nodes"),
			   VOE_ASSETS_JSON_NONE);

	counts = voe_assets_json_member(&json, 0, "counts");
	VOE_TEST_CHECK_INT(voe_assets_json_count(&json, counts), 3);
	for (uint32_t i = 0; i < 3; i++) {
		VOE_TEST_CHECK(voe_assets_json_uint(
			&json, voe_assets_json_element(&json, counts, i), 16,
			&index));
		VOE_TEST_CHECK_INT(index, i + 1);
	}
	VOE_TEST_CHECK_INT(voe_assets_json_element(&json, counts, 3),
			   VOE_ASSETS_JSON_NONE);

	// A negative number with an exponent, which is where a hand-written
	// number reader gives up.
	token = voe_assets_json_member(&json, 0, "scale");
	VOE_TEST_CHECK(voe_assets_json_float(&json, token, &number));
	VOE_TEST_CHECK_FLOAT(number, -150.0f, 1e-6f);

	// And it is not a whole number in range, which is what every glTF index
	// is read as.
	VOE_TEST_CHECK(!voe_assets_json_uint(&json, token, 1000, &index));

	VOE_TEST_CHECK_INT(voe_assets_json_kind_of(
				   &json, voe_assets_json_member(&json, 0, "on")),
			   VOE_ASSETS_JSON_TRUE);
	VOE_TEST_CHECK_INT(voe_assets_json_kind_of(
				   &json,
				   voe_assets_json_member(&json, 0, "off")),
			   VOE_ASSETS_JSON_FALSE);
	VOE_TEST_CHECK_INT(voe_assets_json_kind_of(
				   &json,
				   voe_assets_json_member(&json, 0, "nothing")),
			   VOE_ASSETS_JSON_NULL);

	// A count from something that is not a container is nought rather than
	// nonsense, so a caller that expected an array reads a length it can
	// loop over safely.
	VOE_TEST_CHECK_INT(voe_assets_json_count(&json, token), 0);
}

// A subtree is stepped over and not walked into, which is what `next` is for and
// the one thing a flat token array can get wrong.
static void navigation_steps_over_whole_subtrees(voe_base_arena *arena)
{
	const char *text =
		"{\"a\":{\"deep\":{\"deeper\":[1,2,{\"x\":3}]}},"
		"\"b\":[[1],[2,3]],"
		"\"c\":7}";
	voe_assets_json json;
	uint32_t token;
	uint32_t index;

	VOE_TEST_CHECK(parse(arena, text, &json, NULL));

	// c is the last member, after two large subtrees.
	token = voe_assets_json_member(&json, 0, "c");
	VOE_TEST_CHECK(token != VOE_ASSETS_JSON_NONE);
	VOE_TEST_CHECK(voe_assets_json_uint(&json, token, 10, &index));
	VOE_TEST_CHECK_INT(index, 7);

	// And an array of arrays indexes by element and not by token.
	token = voe_assets_json_member(&json, 0, "b");
	VOE_TEST_CHECK_INT(voe_assets_json_count(&json, token), 2);
	VOE_TEST_CHECK_INT(voe_assets_json_count(
				   &json,
				   voe_assets_json_element(&json, token, 1)),
			   2);
	VOE_TEST_CHECK(voe_assets_json_uint(
		&json,
		voe_assets_json_element(
			&json, voe_assets_json_element(&json, token, 1), 0),
		10, &index));
	VOE_TEST_CHECK_INT(index, 2);
}

static void broken_documents_are_malformed(voe_base_arena *arena)
{
	// Nothing at all, and whitespace that is nothing at all.
	refused(arena, "", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "   \n\t ", VOE_BASE_ERROR_MALFORMED);

	// Every prefix of a good document, which is the shape a truncated file
	// takes.
	refused(arena, "{", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "{\"a\"", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "{\"a\":", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "{\"a\":1", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "[1,2", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "\"unterminated", VOE_BASE_ERROR_MALFORMED);

	// The tolerated mistakes, each of which some reader somewhere accepts.
	refused(arena, "{\"a\":1,}", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "[1,2,]", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "{a:1}", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "{\"a\" 1}", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "{\"a\":1}{\"b\":2}", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "[1 2]", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "{\"a\":1]", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "[1}", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "01", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "+1", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "1.", VOE_BASE_ERROR_MALFORMED);
	refused(arena, ".1", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "1e", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "1e+", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "tru", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "TRUE", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "\"a\\q\"", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "\"a\\u00g0\"", VOE_BASE_ERROR_MALFORMED);
	refused(arena, "\"a\\u00\"", VOE_BASE_ERROR_MALFORMED);

	// A valid escape is not refused, which is the other half of that check.
	{
		voe_assets_json json;

		VOE_TEST_CHECK(parse(arena, "\"a\\u00e5\\n\\\\\"", &json, NULL));
		VOE_TEST_CHECK_INT(voe_assets_json_kind_of(&json, 0),
				   VOE_ASSETS_JSON_STRING);
	}
}

// Nesting deeper than the reader will follow is a refusal and not a crash: this
// is the case rule 14 exists for, and a recursive reader would meet it with a
// stack overflow instead.
static void deep_nesting_is_refused_rather_than_followed(voe_base_arena *arena)
{
	char deep[VOE_ASSETS_JSON_MAX_DEPTH * 2 + 8];
	uint32_t depth = VOE_ASSETS_JSON_MAX_DEPTH + 1;
	voe_assets_json json;
	voe_base_error error = VOE_BASE_OK;

	for (uint32_t i = 0; i < depth; i++)
		deep[i] = '[';
	for (uint32_t i = 0; i < depth; i++)
		deep[depth + i] = ']';
	deep[depth * 2] = '\0';

	VOE_TEST_CHECK(!voe_assets_json_parse(deep, depth * 2, arena, TOKENS,
					      &json, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_MALFORMED);

	// One shallower is fine, which is what says the limit is the limit and
	// not something one off from it.
	depth = VOE_ASSETS_JSON_MAX_DEPTH;
	for (uint32_t i = 0; i < depth; i++)
		deep[i] = '[';
	for (uint32_t i = 0; i < depth; i++)
		deep[depth + i] = ']';
	VOE_TEST_CHECK(voe_assets_json_parse(deep, depth * 2, arena, TOKENS,
					     &json, &error));
}

// More values than the caller said it would hold is unsupported and not
// malformed: the document is fine, this reader was told it could not have that
// much. It is the whole of how a small file is stopped from asking for a large
// allocation.
static void too_many_values_is_unsupported(voe_base_arena *arena)
{
	const char *text = "[1,2,3,4,5]";
	voe_assets_json json;
	voe_base_error error = VOE_BASE_OK;

	// Six tokens: the array and its five elements.
	VOE_TEST_CHECK(voe_assets_json_parse(text, strlen(text), arena, 6,
					     &json, &error));
	VOE_TEST_CHECK(!voe_assets_json_parse(text, strlen(text), arena, 5,
					      &json, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNSUPPORTED);
}

int main(void)
{
	voe_base_arena *arena = voe_base_arena_new(256 * 1024);

	a_document_comes_back_as_its_values(arena);
	navigation_steps_over_whole_subtrees(arena);
	broken_documents_are_malformed(arena);
	deep_nesting_is_refused_rather_than_followed(arena);
	too_many_values_is_unsupported(arena);

	voe_base_arena_destroy(arena);
	return voe_test_result();
}
