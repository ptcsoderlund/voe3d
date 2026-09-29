// What base/describe.h promises, checked from outside: that a struct declared
// through it has a table naming every field in order, with the kind it was
// declared as, the shape (rank and dims), the count, and an offset and size that
// are the compiler's own.
//
// THE OFFSETS ARE THE CLAIM, AND THE STRUCT BELOW IS LAID OUT TO CATCH A TABLE
// THAT WORKS THEM OUT. Each field after the first sits behind padding — a byte,
// then a float vector that starts at four and not at one, then thirteen chars
// that end at twenty-nine, then an entity that starts at thirty-two — so a table
// that summed the sizes before a field instead of asking offsetof would be wrong
// at every row after the first.
//
// A KIND THAT DOES NOT MATCH ITS TYPE IS REFUSED BY THE BUILD, NOT HERE. That is
// a static_assert in the header, and a test that ran could only ever see it pass.
// What this file checks is that the kind, the shape and the count which went in
// are the kind, the shape and the count which come out.
//
// A READ-ONLY FIELD IS A ROW THAT SAYS SO AND A MEMBER THAT DOES NOT DIFFER.
// `owner` is listed through F_READ_ONLY, so the claim under test is two-sided:
// its row reads read_only and no other row does, and the struct is the same
// bytes as the one written by hand below, which has no mark on any member. The
// second described struct below repeats the same claim on a field that is both
// read-only and an array (`deep`), so a shape does not lose the mark.
//
// A NAMED FIELD IS A ROW BESIDE THE FIELDS AND A FIELD THAT DID NOT CHANGE.
// `dial` below names its one field's values through the NAMED macro, with a NULL
// first entry, so the two claims under test are that the names come back as they
// went in and that a struct described the plain way has no names at all.
//
// `lever` NAMES ITS FIELD THROUGH A POINTER, AS LINUX'S STAND-IN FOR IMPORTED
// DATA. On Windows a names array another module exports is dllimport data, whose
// address is loaded at run time and is no constant (ADR-0284). `(*lever_names)`
// is the same: sizeof still sees the array's bound, but its value is a load, so
// a table that put it in a static initialiser would not compile here either.
//
// THE SWITCH IS TURNED ON HERE, WHATEVER THE BUILD SAID. A build that has not
// asked for descriptions has no table, and check.cmake builds that way, so a test
// that followed the build would test nothing on the one run that gates a card.
#undef VOE_BASE_DESCRIPTIONS
#define VOE_BASE_DESCRIPTIONS 1

#include <base/describe.h>

#include <testing/test.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// A float vector and an entity as the folders that own them spell them. base may
// include neither, and the declaring folder supplying the type is the point.
typedef struct {
	float x, y, z;
} vector;

typedef struct {
	uint32_t index;
	uint32_t generation;
} entity;

#define THING_FIELDS(F, F_READ_ONLY)          \
	F(uint8_t, flags, UINT8)              \
	F(vector, where, FLOAT3)              \
	F(char, label, CHAR, 13)              \
	F_READ_ONLY(entity, owner, ENTITY)

VOE_BASE_DESCRIBE_STRUCT(thing, THING_FIELDS)

// The same four members written out, so that "the macro changes nothing about
// the struct" is checked against a struct and not against remembered numbers.
typedef struct {
	uint8_t flags;
	vector where;
	char label[13];
	entity owner;
} thing_by_hand;

static_assert(sizeof(thing) == sizeof(thing_by_hand),
	      "a described struct is not the size of the same members by hand");
static_assert(alignof(thing) == alignof(thing_by_hand),
	      "a described struct is not the alignment of the same members by hand");
static_assert(offsetof(thing, flags) == offsetof(thing_by_hand, flags),
	      "flags moved");
static_assert(offsetof(thing, where) == offsetof(thing_by_hand, where),
	      "where moved");
static_assert(offsetof(thing, label) == offsetof(thing_by_hand, label),
	      "label moved");
static_assert(offsetof(thing, owner) == offsetof(thing_by_hand, owner),
	      "owner moved — the read-only mark reached the member");

// A second struct, proving every rank from 0 to 7 and every kind that may now
// repeat, plus a field that is both read-only and an array (`deep`).
#define SHAPES_FIELDS(F, F_READ_ONLY)                    \
	F(vector, grid, FLOAT3, 4, 2)                     \
	F(char, names, CHAR, 8, 32)                       \
	F(entity, links, ENTITY, 3)                       \
	F_READ_ONLY(uint8_t, deep, UINT8, 1, 2, 1, 2, 1, 2, 1) \
	F(int32_t, one, INT32)

VOE_BASE_DESCRIBE_STRUCT(shapes, SHAPES_FIELDS)

typedef struct {
	vector grid[4][2];
	char names[8][32];
	entity links[3];
	uint8_t deep[1][2][1][2][1][2][1];
	int32_t one;
} shapes_by_hand;

static_assert(sizeof(shapes) == sizeof(shapes_by_hand),
	      "a shaped struct is not the size of the same members by hand");
static_assert(alignof(shapes) == alignof(shapes_by_hand),
	      "a shaped struct is not the alignment of the same members by hand");
static_assert(offsetof(shapes, grid) == offsetof(shapes_by_hand, grid),
	      "grid moved");
static_assert(offsetof(shapes, names) == offsetof(shapes_by_hand, names),
	      "names moved");
static_assert(offsetof(shapes, links) == offsetof(shapes_by_hand, links),
	      "links moved");
static_assert(offsetof(shapes, deep) == offsetof(shapes_by_hand, deep),
	      "deep moved");
static_assert(offsetof(shapes, one) == offsetof(shapes_by_hand, one),
	      "one moved");

// A colour as a declaring folder spells it: three floats, described as COLOUR
// and not FLOAT3, so the kind that comes out is the one that went in.
typedef struct {
	float r, g, b;
} colour;

#define TINTED_FIELDS(F, F_READ_ONLY) F(colour, tint, COLOUR)

VOE_BASE_DESCRIBE_STRUCT(tinted, TINTED_FIELDS)

// A world position as a declaring folder spells it: three doubles, behind a byte
// so it starts at eight, and an array of two after it.
typedef struct {
	double x, y, z;
} position;

#define PLACED_FIELDS(F, F_READ_ONLY)   \
	F(uint8_t, tag, UINT8)          \
	F(position, at, DOUBLE3)        \
	F(position, path, DOUBLE3, 2)

VOE_BASE_DESCRIBE_STRUCT(placed, PLACED_FIELDS)

// A field whose values have names. Value 0 is left unnamed, as a set numbered
// from one leaves its first entry, and a declaring folder would put this array in
// its own .c and declare it extern.
static const char *const dial_mode_names[] = { NULL, "One", "Two" };

#define DIAL_FIELDS(F, F_READ_ONLY) F(uint32_t, mode, UINT32)
#define DIAL_NAMES(N) N(mode, dial_mode_names)

VOE_BASE_DESCRIBE_STRUCT_NAMED(dial, DIAL_FIELDS, DIAL_NAMES)

// Not const, so dereferencing it is a load and never an address constant.
static const char *const lever_position_names[] = { "Down", "Middle", "Up" };
static const char *const (*lever_names)[3] = &lever_position_names;

#define LEVER_FIELDS(F, F_READ_ONLY) F(uint32_t, position, UINT32)
#define LEVER_NAMES(N) N(position, (*lever_names))

VOE_BASE_DESCRIBE_STRUCT_NAMED(lever, LEVER_FIELDS, LEVER_NAMES)

static void check_name(const char *actual, const char *expected)
{
	VOE_TEST_CHECK(strcmp(actual, expected) == 0);
	if (strcmp(actual, expected) != 0)
		fprintf(stderr, "      actual:   \"%s\"\n      expected: \"%s\"\n",
			actual, expected);
}

// dims holds exactly `rank` entries; the rest of the field's dims are expected
// to be 0, which is checked along with the ones given.
static void check_field(const voe_base_field_description *actual,
			const char *name, voe_base_field_kind kind,
			size_t offset, size_t size, uint32_t count,
			uint32_t rank, const uint32_t *dims, bool read_only)
{
	check_name(actual->name, name);
	VOE_TEST_CHECK_INT(actual->kind, kind);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)offset);
	VOE_TEST_CHECK_INT((long long)actual->size, (long long)size);
	VOE_TEST_CHECK_INT(actual->count, count);
	VOE_TEST_CHECK_INT(actual->rank, rank);
	VOE_TEST_CHECK_INT(actual->read_only, read_only);
	for (uint32_t i = 0; i < VOE_BASE_FIELD_RANK_MAX; i++) {
		uint32_t expected_dim = i < rank ? dims[i] : 0;
		VOE_TEST_CHECK_INT(actual->dims[i], expected_dim);
	}
}

// Names reached through a load come back whole, and asking twice answers the
// same table with the same names.
static void check_names_bound_at_run_time(void)
{
	const voe_base_struct_description *lever_desc = lever_description();
	const voe_base_field_names *position_names =
		voe_base_names_find(lever_desc, "position");
	uint32_t bound = sizeof(lever_position_names) /
			 sizeof(lever_position_names[0]);

	VOE_TEST_CHECK(position_names != NULL);
	if (!position_names)
		return;
	check_name(position_names->field, "position");
	VOE_TEST_CHECK_INT(position_names->value_count, bound);
	for (uint32_t i = 0; i < bound && i < position_names->value_count; i++)
		VOE_TEST_CHECK(position_names->values[i] ==
			       lever_position_names[i]);

	VOE_TEST_CHECK(lever_description() == lever_desc);
	VOE_TEST_CHECK(voe_base_names_find(lever_description(), "position")
				       ->values == lever_position_names);
}

int main(void)
{
	const voe_base_struct_description *thing_desc = thing_description();
	uint32_t no_dims[1] = { 0 };
	uint32_t label_dims[1] = { 13 };

	check_name(thing_desc->name, "thing");
	VOE_TEST_CHECK_INT(thing_desc->field_count, 4);
	if (thing_desc->field_count != 4)
		return voe_test_result();

	check_field(&thing_desc->fields[0], "flags", VOE_BASE_FIELD_UINT8,
		    offsetof(thing, flags), 1, 1, 0, no_dims, false);
	check_field(&thing_desc->fields[1], "where", VOE_BASE_FIELD_FLOAT3,
		    offsetof(thing, where), 12, 1, 0, no_dims, false);
	check_field(&thing_desc->fields[2], "label", VOE_BASE_FIELD_CHAR,
		    offsetof(thing, label), 13, 13, 1, label_dims, false);
	check_field(&thing_desc->fields[3], "owner", VOE_BASE_FIELD_ENTITY,
		    offsetof(thing, owner), 8, 1, 0, no_dims, true);

	// The padding the header above promises is really there, so the offset
	// checks are not passing on a struct that happened to pack tight.
	VOE_TEST_CHECK_INT((long long)offsetof(thing, where), 4);
	VOE_TEST_CHECK_INT((long long)offsetof(thing, owner), 32);

	// A field declared through the macro is the member it names, so writing
	// through one and reading back through the table's offset lands in it.
	thing value = { 0 };
	entity read = { 0 };
	value.owner.generation = 7;
	memcpy(&read, (char *)&value + thing_desc->fields[3].offset,
	       sizeof(read));
	VOE_TEST_CHECK_INT(read.generation, 7);

	const voe_base_struct_description *shapes_desc = shapes_description();
	uint32_t grid_dims[2] = { 4, 2 };
	uint32_t names_dims[2] = { 8, 32 };
	uint32_t links_dims[1] = { 3 };
	uint32_t deep_dims[7] = { 1, 2, 1, 2, 1, 2, 1 };

	check_name(shapes_desc->name, "shapes");
	VOE_TEST_CHECK_INT(shapes_desc->field_count, 5);
	if (shapes_desc->field_count != 5)
		return voe_test_result();

	check_field(&shapes_desc->fields[0], "grid", VOE_BASE_FIELD_FLOAT3,
		    offsetof(shapes, grid), 96, 8, 2, grid_dims, false);
	check_field(&shapes_desc->fields[1], "names", VOE_BASE_FIELD_CHAR,
		    offsetof(shapes, names), 256, 256, 2, names_dims, false);
	check_field(&shapes_desc->fields[2], "links", VOE_BASE_FIELD_ENTITY,
		    offsetof(shapes, links), 24, 3, 1, links_dims, false);
	check_field(&shapes_desc->fields[3], "deep", VOE_BASE_FIELD_UINT8,
		    offsetof(shapes, deep), 8, 8, 7, deep_dims, true);
	check_field(&shapes_desc->fields[4], "one", VOE_BASE_FIELD_INT32,
		    offsetof(shapes, one), 4, 1, 0, no_dims, false);

	const voe_base_struct_description *tinted_desc = tinted_description();
	check_name(tinted_desc->name, "tinted");
	VOE_TEST_CHECK_INT(tinted_desc->field_count, 1);
	if (tinted_desc->field_count != 1)
		return voe_test_result();
	check_field(&tinted_desc->fields[0], "tint", VOE_BASE_FIELD_COLOUR,
		    offsetof(tinted, tint), 12, 1, 0, no_dims, false);

	const voe_base_struct_description *placed_desc = placed_description();
	uint32_t path_dims[1] = { 2 };
	VOE_TEST_CHECK_INT(placed_desc->field_count, 3);
	if (placed_desc->field_count != 3)
		return voe_test_result();
	check_field(&placed_desc->fields[1], "at", VOE_BASE_FIELD_DOUBLE3,
		    offsetof(placed, at), 24, 1, 0, no_dims, false);
	check_field(&placed_desc->fields[2], "path", VOE_BASE_FIELD_DOUBLE3,
		    offsetof(placed, path), 48, 2, 1, path_dims, false);
	VOE_TEST_CHECK_INT((long long)offsetof(placed, at), 8);

	const voe_base_struct_description *dial_desc = dial_description();
	VOE_TEST_CHECK_INT(dial_desc->names_count, 1);
	check_field(&dial_desc->fields[0], "mode", VOE_BASE_FIELD_UINT32,
		    offsetof(dial, mode), 4, 1, 0, no_dims, false);

	const voe_base_field_names *mode_names =
		voe_base_names_find(dial_desc, "mode");
	VOE_TEST_CHECK(mode_names != NULL);
	if (mode_names) {
		check_name(mode_names->field, "mode");
		VOE_TEST_CHECK_INT(mode_names->value_count, 3);
		VOE_TEST_CHECK(mode_names->values[0] == NULL);
		check_name(mode_names->values[1], "One");
	}
	VOE_TEST_CHECK(voe_base_names_find(dial_desc, "absent") == NULL);

	// A struct described the way every struct is described today carries no
	// names, and asking for one of its own fields answers NULL.
	VOE_TEST_CHECK(thing_desc->names == NULL);
	VOE_TEST_CHECK_INT(thing_desc->names_count, 0);
	VOE_TEST_CHECK(voe_base_names_find(thing_desc, "flags") == NULL);

	check_names_bound_at_run_time();

	return voe_test_result();
}
