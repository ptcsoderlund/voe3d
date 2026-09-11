// A struct written once, as a list of its fields, and — when a build asks for it
// — a table beside it saying what each field is and where it lives.
//
//     #define VOE_SCENE_TRANSFORM_FIELDS(F)          \
//             F(voe_math_float3, position, FLOAT3)   \
//             F(voe_math_quat, rotation, QUAT)       \
//             F(voe_math_float3, scale, FLOAT3)
//
//     VOE_BASE_DESCRIBE_STRUCT(voe_scene_transform, VOE_SCENE_TRANSFORM_FIELDS)
//
// That is `typedef struct { ... } voe_scene_transform;` with the three members in
// that order, and nothing else about the struct differs from writing it by hand.
// With descriptions compiled in it is also voe_scene_transform_description(),
// which returns the table.
//
// NO SEMICOLON AFTER THE MACRO. It expands to whole declarations that end in their
// own, the way a function definition does, and one more at file scope is an error
// under -Wpedantic.
//
// THE DECLARING FOLDER SUPPLIES THE C TYPE AND BASE SUPPLIES THE KIND. A kind is a
// word in the list below and a size in bytes; base never maps it onto a type,
// which is what lets it describe a maths vector or an entity while including
// neither folder. The first argument of F is whatever type the member really has,
// and a fixed-size array is spelled as its type — F(char[32], name, CHAR) — which
// is why a member is declared through typeof.
//
// THE BUILD REFUSES A KIND THAT DOES NOT MATCH ITS TYPE. Every field is a
// static_assert of the type's size against the kind's, so FLOAT3 on a float2 does
// not compile. ENUM, CHAR and ENTITY are the three kinds that may repeat: for
// those the type must be a whole number of elements and `count` says how many.
// Every other kind is exactly one element. The check is by size and nothing more
// — a uint32_t called FLOAT32 is four bytes either way and passes. ENUM means a
// plain C enum, which is int-sized; one given a narrower underlying type fails.
//
// VOE_BASE_DESCRIPTIONS IS THE SWITCH, AND IT IS OFF UNLESS A BUILD ASKS. Define
// it to 1 — on the command line, or before the first #include in a file — and the
// tables are compiled in; leave it undefined and there is no table, no accessor
// and no string in the binary. It is read once, when this header is first
// included into a translation unit. Everything it emits is static, so files built
// with it and without it link together; the price is that each file holds its
// own copy of a table, so two pointers to one struct's description need not be
// equal.
#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
	VOE_BASE_FIELD_INT8,
	VOE_BASE_FIELD_INT16,
	VOE_BASE_FIELD_INT32,
	VOE_BASE_FIELD_INT64,
	VOE_BASE_FIELD_UINT8,
	VOE_BASE_FIELD_UINT16,
	VOE_BASE_FIELD_UINT32,
	VOE_BASE_FIELD_UINT64,
	VOE_BASE_FIELD_FLOAT32,
	VOE_BASE_FIELD_FLOAT64,
	VOE_BASE_FIELD_BOOL,
	VOE_BASE_FIELD_FLOAT2,
	VOE_BASE_FIELD_FLOAT3,
	VOE_BASE_FIELD_FLOAT4,
	VOE_BASE_FIELD_QUAT,
	VOE_BASE_FIELD_FLOAT4X4,
	VOE_BASE_FIELD_ENUM,
	VOE_BASE_FIELD_CHAR,
	VOE_BASE_FIELD_ENTITY,
} voe_base_field_kind;

typedef struct {
	const char *name;
	voe_base_field_kind kind;
	// Bytes from the start of the struct.
	size_t offset;
	// Bytes the whole field takes, so `count` elements of size / count each.
	size_t size;
	// 1 for a single value, N for a fixed-size array of N.
	uint32_t count;
} voe_base_field_description;

typedef struct {
	const char *name;
	// In declaration order, which is layout order.
	const voe_base_field_description *fields;
	uint32_t field_count;
} voe_base_struct_description;

// The size of one element of each kind, in bytes, and whether a field of that
// kind may be an array of them. Pasted onto a kind's name by the macros below.
#define VOE_BASE_FIELD_SIZE_INT8 1
#define VOE_BASE_FIELD_SIZE_INT16 2
#define VOE_BASE_FIELD_SIZE_INT32 4
#define VOE_BASE_FIELD_SIZE_INT64 8
#define VOE_BASE_FIELD_SIZE_UINT8 1
#define VOE_BASE_FIELD_SIZE_UINT16 2
#define VOE_BASE_FIELD_SIZE_UINT32 4
#define VOE_BASE_FIELD_SIZE_UINT64 8
#define VOE_BASE_FIELD_SIZE_FLOAT32 4
#define VOE_BASE_FIELD_SIZE_FLOAT64 8
#define VOE_BASE_FIELD_SIZE_BOOL 1
#define VOE_BASE_FIELD_SIZE_FLOAT2 8
#define VOE_BASE_FIELD_SIZE_FLOAT3 12
#define VOE_BASE_FIELD_SIZE_FLOAT4 16
#define VOE_BASE_FIELD_SIZE_QUAT 16
#define VOE_BASE_FIELD_SIZE_FLOAT4X4 64
#define VOE_BASE_FIELD_SIZE_ENUM 4
#define VOE_BASE_FIELD_SIZE_CHAR 1
#define VOE_BASE_FIELD_SIZE_ENTITY 8

#define VOE_BASE_FIELD_REPEATS_INT8 0
#define VOE_BASE_FIELD_REPEATS_INT16 0
#define VOE_BASE_FIELD_REPEATS_INT32 0
#define VOE_BASE_FIELD_REPEATS_INT64 0
#define VOE_BASE_FIELD_REPEATS_UINT8 0
#define VOE_BASE_FIELD_REPEATS_UINT16 0
#define VOE_BASE_FIELD_REPEATS_UINT32 0
#define VOE_BASE_FIELD_REPEATS_UINT64 0
#define VOE_BASE_FIELD_REPEATS_FLOAT32 0
#define VOE_BASE_FIELD_REPEATS_FLOAT64 0
#define VOE_BASE_FIELD_REPEATS_BOOL 0
#define VOE_BASE_FIELD_REPEATS_FLOAT2 0
#define VOE_BASE_FIELD_REPEATS_FLOAT3 0
#define VOE_BASE_FIELD_REPEATS_FLOAT4 0
#define VOE_BASE_FIELD_REPEATS_QUAT 0
#define VOE_BASE_FIELD_REPEATS_FLOAT4X4 0
#define VOE_BASE_FIELD_REPEATS_ENUM 1
#define VOE_BASE_FIELD_REPEATS_CHAR 1
#define VOE_BASE_FIELD_REPEATS_ENTITY 1

// The sizes above that are the platform's to decide rather than a declaring
// folder's, checked where they are written down.
static_assert(sizeof(float) == VOE_BASE_FIELD_SIZE_FLOAT32, "float is not 4 bytes");
static_assert(sizeof(double) == VOE_BASE_FIELD_SIZE_FLOAT64, "double is not 8 bytes");
static_assert(sizeof(bool) == VOE_BASE_FIELD_SIZE_BOOL, "bool is not 1 byte");
static_assert(sizeof(voe_base_field_kind) == VOE_BASE_FIELD_SIZE_ENUM,
	      "a plain enum is not 4 bytes");

#define VOE_BASE_DESCRIBE_STRUCT(struct_name, field_list)                         \
	typedef struct {                                                      \
		field_list(VOE_BASE_DESCRIBE_MEMBER_)                             \
	} struct_name;                                                        \
	VOE_BASE_DESCRIBE_TABLE_(struct_name, field_list)                         \
	field_list(VOE_BASE_DESCRIBE_CHECK_)

// Everything from here down is the expansion, and not for use on its own.

#define VOE_BASE_DESCRIBE_MEMBER_(type, field, KIND) typeof(type) field;

#define VOE_BASE_DESCRIBE_CHECK_(type, field, KIND)                           \
	static_assert(VOE_BASE_FIELD_REPEATS_##KIND                           \
		? sizeof(typeof(type)) % VOE_BASE_FIELD_SIZE_##KIND == 0      \
		: sizeof(typeof(type)) == VOE_BASE_FIELD_SIZE_##KIND,         \
		#field ": the declared type is not the size of " #KIND);

#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS

// voe_base_describe_self_ is the struct being described, named inside the
// accessor so that a row can take offsetof without F having to carry the struct's
// name to every field.
#define VOE_BASE_DESCRIBE_ROW_(type, field, KIND)                             \
	{                                                                     \
		.name = #field,                                               \
		.kind = VOE_BASE_FIELD_##KIND,                                \
		.offset = offsetof(voe_base_describe_self_, field),           \
		.size = sizeof(typeof(type)),                                 \
		.count = (uint32_t)(sizeof(typeof(type)) /                    \
				    VOE_BASE_FIELD_SIZE_##KIND),              \
	},

#define VOE_BASE_DESCRIBE_TABLE_(struct_name, field_list)                         \
	static inline const voe_base_struct_description *                     \
		struct_name##_description(void)                               \
	{                                                                     \
		typedef struct_name voe_base_describe_self_;                  \
		static const voe_base_field_description rows[] = {            \
			field_list(VOE_BASE_DESCRIBE_ROW_)                        \
		};                                                            \
		static const voe_base_struct_description description = {      \
			.name = #struct_name,                                 \
			.fields = rows,                                       \
			.field_count = sizeof(rows) / sizeof(rows[0]),        \
		};                                                            \
		return &description;                                          \
	}

#else

#define VOE_BASE_DESCRIBE_TABLE_(struct_name, field_list)

#endif
