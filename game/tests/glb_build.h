// The hand-built `.glb` the model tests load: a one-triangle file, its JSON
// and BIN chunks packed by hand, the packer itself for a test's own mesh, and
// the world helpers that put a thing at the origin wearing a path. Included
// once by each of tests/models.c and tests/models_levels.c.
//
// IT IS A HEADER AND NOT A .c, because every tests/*.c is built as a program of
// its own: a shared .c would be a test with no main(). Every function is
// `static inline`, as render/tests/element_scene.h holds its own.
#pragma once

#include <3d/model_component.h>

#include <math/quat.h>

#include <scene/transform_system.h>

#include <testing/test.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char JSON[] =
	"{\"asset\":{\"version\":\"2.0\"},\"nodes\":[{\"mesh\":0}],"
	"\"materials\":[{}],"
	"\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},"
	"\"material\":0}]}],"
	"\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":3,"
	"\"type\":\"VEC3\",\"min\":[0,0,0],\"max\":[1,1,0]}],"
	"\"bufferViews\":[{\"buffer\":0,\"byteLength\":36}],"
	"\"buffers\":[{\"byteLength\":36}]}";

static const float TRIANGLE[9] = { 0, 0, 0, 1, 0, 0, 0, 1, 0 };

static inline void put_u32(uint8_t *at, uint32_t value)
{
	for (uint32_t i = 0; i < 4; i++)
		at[i] = (uint8_t)(value >> (8 * i));
}

// A `.glb` of `text` and `bin` into `out`, its length returned: a 12-byte
// header, the JSON chunk padded with spaces to four bytes, then the BIN chunk,
// whose size is a multiple of four.
static inline uint32_t glb_pack(uint8_t *out, size_t room, const char *text,
				const void *bin, uint32_t bin_size)
{
	uint32_t length = (uint32_t)strlen(text);
	uint32_t json = (length + 3) & ~3u;
	uint32_t total = 12 + 8 + json + 8 + bin_size;

	VOE_TEST_CHECK(total <= room);
	if (total > room)
		return 0;
	put_u32(out, 0x46546C67u);
	put_u32(out + 4, 2);
	put_u32(out + 8, total);
	put_u32(out + 12, json);
	put_u32(out + 16, 0x4E4F534Au);
	memset(out + 20, ' ', json);
	memcpy(out + 20, text, length);
	put_u32(out + 20 + json, bin_size);
	put_u32(out + 24 + json, 0x004E4942u);
	memcpy(out + 28 + json, bin, bin_size);
	return total;
}

// The one-triangle `.glb` into `out`, its length returned.
static inline uint32_t glb_build(uint8_t *out, size_t room)
{
	return glb_pack(out, room, JSON, TRIANGLE, (uint32_t)sizeof(TRIANGLE));
}

// A new thing at the origin.
static inline voe_ecs_entity place(voe_ecs_world *world)
{
	voe_scene_transform pose = {
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, 0.0f),
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	voe_ecs_entity thing;

	VOE_TEST_CHECK(voe_ecs_entity_create(world, &thing));
	VOE_TEST_CHECK(voe_scene_transform_add(world, thing, pose));
	return thing;
}

// A thing at the origin wearing `path`.
static inline void wear(voe_ecs_world *world, const char *path)
{
	voe_3d_model model = { 0 };

	snprintf(model.path, sizeof model.path, "%s", path);
	VOE_TEST_CHECK(voe_3d_model_add(world, place(world), model));
}
