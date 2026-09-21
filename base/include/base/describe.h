// A struct written once, as a list of its fields, and — when a build asks for it
// — a table beside it saying what each field is and where it lives.
//
//     #define VOE_SCENE_TRANSFORM_FIELDS(F, F_READ_ONLY) \
//             F(voe_math_float3, position, FLOAT3)       \
//             F(voe_math_quat, rotation, QUAT)           \
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
// A FIELD LIST TAKES TWO PARAMETERS AND EVERY FIELD GOES THROUGH ONE OF THEM.
// F and F_READ_ONLY are invoked with the same arguments, and the member they
// declare and the checks it must pass are the same either way: the struct is
// byte-for-byte what it would be if every field were listed through F. What
// differs is the field's row, which says read_only for a field listed through
// F_READ_ONLY.
//
// THE DECLARING FOLDER SUPPLIES THE C TYPE AND BASE SUPPLIES THE KIND. A kind is
// a word in the list below and a size in bytes; base never maps it onto a type,
// which is what lets it describe a maths vector or an entity while including
// neither folder. The first argument of F is one element's type — never an array
// — and a member is declared through typeof so it may be any type the declaring
// folder names, primitive or its own struct.
//
// A FIELD IS ONE KIND AND ZERO TO SEVEN DIMENSIONS, OUTERMOST FIRST, AFTER THE
// KIND (ADR-0154). F(voe_math_float3, path, FLOAT3, 4, 2) declares
// voe_math_float3 path[4][2], a row of rank 2 with dims {4, 2}. No dimensions is
// one value, rank 0. EVERY KIND MAY BE AN ARRAY — the old ENUM/CHAR/ENTITY-only
// restriction is gone. FOR CHAR, THE INNERMOST DIMENSION IS THE STRING'S BYTES:
// CHAR, 32 is one string of 32 bytes; CHAR, 8, 32 is eight of them, written as
// eight strings and not 256 characters. THE DIMENSIONS ARE SPELLED OUT ON THE
// FIELD LINE, NOT RECOVERED FROM THE TYPE, because C cannot pull an array's
// bounds back out of a type in a constant expression — a member spelled
// F(char[32], name, CHAR) could not have told the macro whether it held one
// string of 32 bytes or 32 strings of one, which is why that spelling is gone in
// favour of F(char, name, CHAR, 32).
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
#include <string.h>

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
	// COLOUR IS THREE FLOATS, LINEAR RGB, EACH 0 TO 1, LAID OUT AND SPELLED AS
	// FLOAT3. It is 12 bytes like FLOAT3 and is written and read in scene text the
	// same way; it exists so a tool can show the field as a swatch without naming
	// the component, the way QUAT lets it show three angles (ADR-0191).
	VOE_BASE_FIELD_COLOUR,
	VOE_BASE_FIELD_ENUM,
	VOE_BASE_FIELD_CHAR,
	VOE_BASE_FIELD_ENTITY,
} voe_base_field_kind;

// A field has at most this many dimensions. With a vector kind's own bracket in
// the text format, 7 is the 8 bracket levels authoring's reader refuses past
// (ADR-0154 point 3).
#define VOE_BASE_FIELD_RANK_MAX 7

typedef struct {
	const char *name;
	voe_base_field_kind kind;
	// Bytes from the start of the struct.
	size_t offset;
	// Bytes the whole field takes, so `count` elements of size / count each.
	size_t size;
	// 1 for a single value, N for a fixed-size array of N.
	uint32_t count;
	// How many dimensions: 0 for a single value, up to VOE_BASE_FIELD_RANK_MAX.
	uint32_t rank;
	// Outermost first, as C writes them; the entries past `rank` are 0.
	uint32_t dims[VOE_BASE_FIELD_RANK_MAX];
	// READ-ONLY IS A NOTE TO A TOOL AND NOTHING MORE. It says that an editor shows
	// the field and does not offer to change it — an authored id being the case it
	// was written for. It is not const, it does not bind the code, and the program
	// runs the same with it and without it; the mark is the declaring folder's to
	// write, on the line beside the field.
	//
	// True for a field listed through F_READ_ONLY: a tool shows it and does
	// not edit it. Nothing in the struct or the program depends on it.
	bool read_only;
} voe_base_field_description;

// What one field's values are called, for a tool to show instead of the number.
typedef struct {
	// The field these names belong to, as it is spelled in the struct.
	const char *field;
	// values[i] is the name of value i, NULL for a value this list does not
	// name.
	const char *const *values;
	uint32_t value_count;
} voe_base_field_names;

typedef struct {
	const char *name;
	// In declaration order, which is layout order.
	const voe_base_field_description *fields;
	uint32_t field_count;
	// One row per named field, or NULL and 0 for a struct described without
	// names — which is every struct that does not ask for them.
	const voe_base_field_names *names;
	uint32_t names_count;
} voe_base_struct_description;

// The names for `field`, or NULL when it has none. A handful of rows at most, so
// a scan and a strcmp.
static inline const voe_base_field_names *
voe_base_names_find(const voe_base_struct_description *description,
		    const char *field)
{
	if (!description || !description->names || !field)
		return NULL;
	for (uint32_t i = 0; i < description->names_count; i++)
		if (strcmp(description->names[i].field, field) == 0)
			return &description->names[i];
	return NULL;
}

// The size of one element of each kind, in bytes. Pasted onto a kind's name by
// the macros below.
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
#define VOE_BASE_FIELD_SIZE_COLOUR 12
#define VOE_BASE_FIELD_SIZE_ENUM 4
#define VOE_BASE_FIELD_SIZE_CHAR 1
#define VOE_BASE_FIELD_SIZE_ENTITY 8

// The sizes above that are the platform's to decide rather than a declaring
// folder's, checked where they are written down.
static_assert(sizeof(float) == VOE_BASE_FIELD_SIZE_FLOAT32, "float is not 4 bytes");
static_assert(sizeof(double) == VOE_BASE_FIELD_SIZE_FLOAT64, "double is not 8 bytes");
static_assert(sizeof(bool) == VOE_BASE_FIELD_SIZE_BOOL, "bool is not 1 byte");
static_assert(sizeof(voe_base_field_kind) == VOE_BASE_FIELD_SIZE_ENUM,
	      "a plain enum is not 4 bytes");

// THE BUILD REFUSES A KIND THAT DOES NOT MATCH ITS TYPE. Every field is a
// static_assert of one element's size against the kind's — sizeof(typeof(type))
// exactly equal to the kind's size, for every kind, array or not — so FLOAT3 on a
// float2 does not compile. The check is by size and nothing more — a uint32_t
// called FLOAT32 is four bytes either way and passes. ENUM means a plain C enum,
// which is int-sized; one given a narrower underlying type fails. A DIMENSION OF
// 0 FAILS TOO: a zero-size array member is a GNU extension, and voe_module()
// always builds with -Wpedantic -Werror, which turns that extension into a hard
// error. AN EIGHTH DIMENSION FAILS AS WELL, though not as cleanly: past seven,
// VOE_BASE_FIELD_RANK_'s counting reads one of the extra dimensions back as the
// rank, so the dispatch below still finds a real ARRAYn/DIMSn macro rather than
// an undefined one — and calls it with more arguments than it takes, which is
// "too many arguments provided to function-like macro invocation", still a
// build failure.
#define VOE_BASE_DESCRIBE_STRUCT(struct_name, field_list)                     \
	typedef struct {                                                      \
		field_list(VOE_BASE_DESCRIBE_MEMBER_,                         \
			   VOE_BASE_DESCRIBE_MEMBER_)                         \
	} struct_name;                                                        \
	VOE_BASE_DESCRIBE_TABLE_(struct_name, field_list)                     \
	field_list(VOE_BASE_DESCRIBE_CHECK_, VOE_BASE_DESCRIBE_CHECK_)

// NAMES ARE A NOTE TO A TOOL, EXACTLY AS READ-ONLY IS. A struct may carry, beside
// its fields, a small table saying what one field's values are called:
// VOE_BASE_DESCRIBE_STRUCT_NAMED(struct, field_list, names_list), where a names
// list invokes its one parameter once per named field with the field's name and an
// array of names — #define VOE_3D_SHAPE_NAMES(N) N(kind, voe_3d_shape_kind_names).
// Entry i names value i, and a NULL entry is a value with no name, so a set
// numbered from one leaves its first entry NULL and a tool shows that value as the
// number it is. THE FIELD'S KIND DOES NOT CHANGE: a named field is the UINT32 it
// already was, and the scene text it is written to is unchanged. The names array
// belongs to the declaring folder and is extern there, so a build without
// descriptions carries no copy of it. What a tool does with the names — a dropdown
// of the named values — is the tool's business and not this folder's (ADR-0195,
// ADR-0198).
//
// The same struct, the same checks and the same field table, plus the table that
// names one or more fields' values.
#define VOE_BASE_DESCRIBE_STRUCT_NAMED(struct_name, field_list, names_list)   \
	typedef struct {                                                      \
		field_list(VOE_BASE_DESCRIBE_MEMBER_,                         \
			   VOE_BASE_DESCRIBE_MEMBER_)                         \
	} struct_name;                                                        \
	VOE_BASE_DESCRIBE_TABLE_NAMED_(struct_name, field_list, names_list)   \
	field_list(VOE_BASE_DESCRIBE_CHECK_, VOE_BASE_DESCRIBE_CHECK_)

// Everything from here down is the expansion, and not for use on its own.

// Turning a trailing dimension list into `[d1][d2]…`, into `{d1, d2, …}` padded
// to VOE_BASE_FIELD_RANK_MAX, and into the rank itself, is a counted dispatch: one
// macro per rank from 0 to 7, picked by pasting the rank onto a common prefix.
// Nothing here recurses — a fixed, small upper bound reads better as a table than
// as a trick. Past seven dimensions the rank calculation below no longer counts
// truly — it reads one of the extra dimensions back as if it were the rank — but
// the dispatch it feeds still fails to build: whichever ARRAYn/DIMSn macro that
// wrong rank names takes fewer parameters than the extra dimensions supply, and
// the preprocessor refuses the call outright.

#define VOE_BASE_FIELD_CONCAT_(a, b) a##b
// The indirection matters: without it, `rank` would be pasted onto the prefix
// before being expanded to the digit VOE_BASE_FIELD_RANK_ computed.
#define VOE_BASE_FIELD_CONCAT_2_(a, b) VOE_BASE_FIELD_CONCAT_(a, b)

// The rank is the count of a variadic argument list, 0 to 7: the trailing
// sentinel 7,6,…,0 lines up so that the Nth argument supplied pushes the
// sentinel's N-th entry into the position VOE_BASE_FIELD_RANK_N_ picks off.
#define VOE_BASE_FIELD_RANK_(...) \
	VOE_BASE_FIELD_RANK_N_(__VA_ARGS__ __VA_OPT__(, ) 7, 6, 5, 4, 3, 2, 1, 0)
#define VOE_BASE_FIELD_RANK_N_(a1, a2, a3, a4, a5, a6, a7, n, ...) n

#define VOE_BASE_FIELD_ARRAY_(rank, ...) \
	VOE_BASE_FIELD_CONCAT_2_(VOE_BASE_FIELD_ARRAY, rank)(__VA_ARGS__)
#define VOE_BASE_FIELD_ARRAY0()
#define VOE_BASE_FIELD_ARRAY1(d1) [d1]
#define VOE_BASE_FIELD_ARRAY2(d1, d2) [d1][d2]
#define VOE_BASE_FIELD_ARRAY3(d1, d2, d3) [d1][d2][d3]
#define VOE_BASE_FIELD_ARRAY4(d1, d2, d3, d4) [d1][d2][d3][d4]
#define VOE_BASE_FIELD_ARRAY5(d1, d2, d3, d4, d5) [d1][d2][d3][d4][d5]
#define VOE_BASE_FIELD_ARRAY6(d1, d2, d3, d4, d5, d6) \
	[d1][d2][d3][d4][d5][d6]
#define VOE_BASE_FIELD_ARRAY7(d1, d2, d3, d4, d5, d6, d7) \
	[d1][d2][d3][d4][d5][d6][d7]

#define VOE_BASE_FIELD_DIMS_(rank, ...) \
	VOE_BASE_FIELD_CONCAT_2_(VOE_BASE_FIELD_DIMS, rank)(__VA_ARGS__)
#define VOE_BASE_FIELD_DIMS0() 0, 0, 0, 0, 0, 0, 0
#define VOE_BASE_FIELD_DIMS1(d1) d1, 0, 0, 0, 0, 0, 0
#define VOE_BASE_FIELD_DIMS2(d1, d2) d1, d2, 0, 0, 0, 0, 0
#define VOE_BASE_FIELD_DIMS3(d1, d2, d3) d1, d2, d3, 0, 0, 0, 0
#define VOE_BASE_FIELD_DIMS4(d1, d2, d3, d4) d1, d2, d3, d4, 0, 0, 0
#define VOE_BASE_FIELD_DIMS5(d1, d2, d3, d4, d5) d1, d2, d3, d4, d5, 0, 0
#define VOE_BASE_FIELD_DIMS6(d1, d2, d3, d4, d5, d6) \
	d1, d2, d3, d4, d5, d6, 0
#define VOE_BASE_FIELD_DIMS7(d1, d2, d3, d4, d5, d6, d7) \
	d1, d2, d3, d4, d5, d6, d7

#define VOE_BASE_DESCRIBE_MEMBER_(type, field, KIND, ...)                    \
	typeof(type) field VOE_BASE_FIELD_ARRAY_(                            \
		VOE_BASE_FIELD_RANK_(__VA_ARGS__), __VA_ARGS__);

#define VOE_BASE_DESCRIBE_CHECK_(type, field, KIND, ...)                     \
	static_assert(sizeof(typeof(type)) == VOE_BASE_FIELD_SIZE_##KIND,     \
		      #field ": the declared type is not the size of " #KIND);

#if defined(VOE_BASE_DESCRIPTIONS) && VOE_BASE_DESCRIPTIONS

// voe_base_describe_self_ is the struct being described, named inside the
// accessor so that a row can take offsetof and sizeof the member itself without
// F having to carry the struct's name to every field. `count` and `size` come
// from sizeof(field) rather than from multiplying the dimensions again, so they
// can never disagree with the member the compiler actually laid out.
#define VOE_BASE_DESCRIBE_ROW_IMPL_(type, field, KIND, read_only_, ...)      \
	{                                                                     \
		.name = #field,                                               \
		.kind = VOE_BASE_FIELD_##KIND,                                \
		.offset = offsetof(voe_base_describe_self_, field),           \
		.size = sizeof(((voe_base_describe_self_ *)0)->field),        \
		.count = (uint32_t)(sizeof(((voe_base_describe_self_ *)0)    \
						    ->field) /                 \
				    sizeof(typeof(type))),                    \
		.rank = VOE_BASE_FIELD_RANK_(__VA_ARGS__),                    \
		.dims = { VOE_BASE_FIELD_DIMS_(                               \
			VOE_BASE_FIELD_RANK_(__VA_ARGS__), __VA_ARGS__) },    \
		.read_only = read_only_,                                      \
	},

#define VOE_BASE_DESCRIBE_ROW_(type, field, KIND, ...)                       \
	VOE_BASE_DESCRIBE_ROW_IMPL_(type, field, KIND, false, __VA_ARGS__)

#define VOE_BASE_DESCRIBE_ROW_READ_ONLY_(type, field, KIND, ...)             \
	VOE_BASE_DESCRIBE_ROW_IMPL_(type, field, KIND, true, __VA_ARGS__)

#define VOE_BASE_DESCRIBE_TABLE_(struct_name, field_list)                     \
	static inline const voe_base_struct_description *                     \
		struct_name##_description(void)                               \
	{                                                                     \
		typedef struct_name voe_base_describe_self_;                  \
		static const voe_base_field_description rows[] = {            \
			field_list(VOE_BASE_DESCRIBE_ROW_,                    \
				   VOE_BASE_DESCRIBE_ROW_READ_ONLY_)          \
		};                                                            \
		static const voe_base_struct_description description = {      \
			.name = #struct_name,                                 \
			.fields = rows,                                       \
			.field_count = sizeof(rows) / sizeof(rows[0]),        \
		};                                                            \
		return &description;                                          \
	}

// The count is the names array's own declared bound, so it is never a number
// written twice.
#define VOE_BASE_DESCRIBE_NAMES_ROW_(field_, values_)                        \
	{                                                                     \
		.field = #field_,                                             \
		.values = (values_),                                          \
		.value_count = (uint32_t)(sizeof(values_) /                   \
					  sizeof((values_)[0])),              \
	},

// Its own table macro rather than the plain one with an empty names list: an
// empty initializer for a zero-length array is the GNU extension that
// -Wpedantic -Werror refuses.
#define VOE_BASE_DESCRIBE_TABLE_NAMED_(struct_name, field_list, names_list)   \
	static inline const voe_base_struct_description *                     \
		struct_name##_description(void)                               \
	{                                                                     \
		typedef struct_name voe_base_describe_self_;                  \
		static const voe_base_field_description rows[] = {            \
			field_list(VOE_BASE_DESCRIBE_ROW_,                    \
				   VOE_BASE_DESCRIBE_ROW_READ_ONLY_)          \
		};                                                            \
		static const voe_base_field_names named[] = {                 \
			names_list(VOE_BASE_DESCRIBE_NAMES_ROW_)              \
		};                                                            \
		static const voe_base_struct_description description = {      \
			.name = #struct_name,                                 \
			.fields = rows,                                       \
			.field_count = sizeof(rows) / sizeof(rows[0]),        \
			.names = named,                                       \
			.names_count = sizeof(named) / sizeof(named[0]),      \
		};                                                            \
		return &description;                                          \
	}

#else

#define VOE_BASE_DESCRIBE_TABLE_(struct_name, field_list)
#define VOE_BASE_DESCRIBE_TABLE_NAMED_(struct_name, field_list, names_list)

#endif
