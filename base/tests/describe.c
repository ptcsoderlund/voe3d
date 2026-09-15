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

	return voe_test_result();
}
