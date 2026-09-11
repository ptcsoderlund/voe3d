// What base/describe.h promises, checked from outside: that a struct declared
// through it has a table naming every field in order, with the kind it was
// declared as, the count, and an offset and size that are the compiler's own.
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
// What this file checks is that the kind which went in is the kind which comes
// out.
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

#define THING_FIELDS(F)             \
	F(uint8_t, flags, UINT8)    \
	F(vector, where, FLOAT3)    \
	F(char[13], label, CHAR)    \
	F(entity, owner, ENTITY)

VOE_BASE_DESCRIBE_STRUCT(thing, THING_FIELDS)

static void check_name(const char *actual, const char *expected)
{
	VOE_TEST_CHECK(strcmp(actual, expected) == 0);
	if (strcmp(actual, expected) != 0)
		fprintf(stderr, "      actual:   \"%s\"\n      expected: \"%s\"\n",
			actual, expected);
}

static void check_field(const voe_base_field_description *actual,
			voe_base_field_description expected)
{
	check_name(actual->name, expected.name);
	VOE_TEST_CHECK_INT(actual->kind, expected.kind);
	VOE_TEST_CHECK_INT((long long)actual->offset, (long long)expected.offset);
	VOE_TEST_CHECK_INT((long long)actual->size, (long long)expected.size);
	VOE_TEST_CHECK_INT(actual->count, expected.count);
}

int main(void)
{
	const voe_base_struct_description *description = thing_description();
	voe_base_field_description expected[] = {
		{ "flags", VOE_BASE_FIELD_UINT8, offsetof(thing, flags), 1, 1 },
		{ "where", VOE_BASE_FIELD_FLOAT3, offsetof(thing, where), 12, 1 },
		{ "label", VOE_BASE_FIELD_CHAR, offsetof(thing, label), 13, 13 },
		{ "owner", VOE_BASE_FIELD_ENTITY, offsetof(thing, owner), 8, 1 },
	};
	uint32_t count = sizeof(expected) / sizeof(expected[0]);
	thing value = { 0 };
	entity read = { 0 };

	check_name(description->name, "thing");
	VOE_TEST_CHECK_INT(description->field_count, count);
	if (description->field_count != count)
		return voe_test_result();

	for (uint32_t i = 0; i < count; i++)
		check_field(&description->fields[i], expected[i]);

	// The padding the header above promises is really there, so the offset
	// checks are not passing on a struct that happened to pack tight.
	VOE_TEST_CHECK_INT((long long)offsetof(thing, where), 4);
	VOE_TEST_CHECK_INT((long long)offsetof(thing, owner), 32);

	// A field declared through the macro is the member it names, so writing
	// through one and reading back through the table's offset lands in it.
	value.owner.generation = 7;
	memcpy(&read, (char *)&value + description->fields[3].offset, sizeof(read));
	VOE_TEST_CHECK_INT(read.generation, 7);

	return voe_test_result();
}
