// The cube: its eight vertices, its thirty-six indices, the three matrices that
// place it, and the descriptor the shader reads them through. Everything in here
// has a startup lifetime — a resize changes the projection's aspect ratio and
// nothing else, and that is recomputed per frame rather than stored.
//
// THIS IS THE TRIANGLE'S REPLACEMENT AND IT IS STILL A PLACEHOLDER. The cube is
// a constant in this file exactly as the triangle's three vertices were
// constants in its shader, and the camera below is a constant beside it. render
// has no scene to ask and must not grow one: a camera that can be moved arrives
// with the card that gives it something to be moved by, and geometry that is not
// a cube arrives with the card that loads a file. What is worth keeping here is
// the plumbing under it — buffers, an upload, a descriptor, a depth test — not
// the shape.
//
// EIGHT VERTICES, NOT TWENTY-FOUR, AND THAT IS THE CARD'S DECISION. A corner
// shared by three faces can share one position and one colour, but it cannot
// share three different texture coordinates; the moment texture coordinates
// exist this becomes twenty-four vertices and the data below is rewritten. That
// is known and accepted rather than discovered later.
//
// THE WINDING IS COUNTER-CLOCKWISE SEEN FROM OUTSIDE, WHICH IS glTF'S AND SO
// THIS ENGINE'S. Every one of the twelve triangles below is wound so that its
// cross product points out of the cube. The pipeline in device.c culls back
// faces and names counter-clockwise as the front, and the viewport in frame.c
// flips Y — three facts that only agree by construction, which is what
// render/tests/offscreen.c is for.
//
// THE CAMERA LOOKS ALONG ITS OWN -Z AND THE VIEW MATRIX IS BUILT HERE BECAUSE
// math WILL NOT BUILD IT. A look-at is a camera, math is pure value types with
// no opinion about cameras, and a projection matrix encodes a clip-space
// convention that math is explicitly not allowed to know — see float4x4.h. So
// both are assembled here out of math's vectors, and this file is where this
// engine's conventions turn into sixteen floats.
#include "device_internal.h"

#include <base/assert.h>

#include <math.h>
#include <stdio.h>

// Half the side, so the cube spans one metre. Units are metres (CLAUDE.md).
#define HALF 0.5f

// 60 degrees of vertical field of view, in radians because every angle in this
// engine is. Vertical and not horizontal, because the projection below divides
// by the aspect ratio rather than multiplying, so a wider window shows more
// rather than the same amount squashed.
#define FIELD_OF_VIEW 1.0471976f

// The near plane, in metres. There is deliberately no far plane: see
// voe_render_cube_projection.
#define NEAR_PLANE 0.1f

// Where the camera is, and what it looks at. Off-axis in all three, so that
// three faces of the cube are visible at once and the depth test has something
// to do — head on, a cube is a square and a broken depth test looks fine.
#define EYE_X 2.2f
#define EYE_Y 1.8f
#define EYE_Z 3.0f

// The eight corners, and a colour per corner rather than per face. The colour is
// the position moved into 0..1, so opposite corners are opposite colours and no
// two faces read the same — which is the whole of how a person checks by eye
// that they are looking at the near face and not through it.
static const struct voe_render_vertex CUBE_VERTICES[8] = {
	{ { -HALF, -HALF, -HALF }, { 0.0f, 0.0f, 0.0f } },
	{ { HALF, -HALF, -HALF }, { 1.0f, 0.0f, 0.0f } },
	{ { HALF, HALF, -HALF }, { 1.0f, 1.0f, 0.0f } },
	{ { -HALF, HALF, -HALF }, { 0.0f, 1.0f, 0.0f } },
	{ { -HALF, -HALF, HALF }, { 0.0f, 0.0f, 1.0f } },
	{ { HALF, -HALF, HALF }, { 1.0f, 0.0f, 1.0f } },
	{ { HALF, HALF, HALF }, { 1.0f, 1.0f, 1.0f } },
	{ { -HALF, HALF, HALF }, { 0.0f, 1.0f, 1.0f } },
};

// Two triangles per face, six faces, counter-clockwise seen from outside. The
// faces are in the order +Z -Z +X -X +Y -Y so that a reader can check one
// against the corner table above without hunting.
static const uint16_t CUBE_INDICES[36] = {
	4, 5, 6, 4, 6, 7, // +Z
	1, 0, 3, 1, 3, 2, // -Z
	5, 1, 2, 5, 2, 6, // +X
	0, 4, 7, 0, 7, 3, // -X
	7, 6, 2, 7, 2, 3, // +Y
	0, 1, 5, 0, 5, 4, // -Y
};

// ------------------------------------------------------------- the matrices

// The camera, as a view matrix. Right-handed, +Y up, and the camera looks along
// its own -Z, so the third row is the direction pointing *back* at the viewer
// and not the direction of view. Getting that sign wrong puts the world behind
// the camera, which looks exactly like nothing being drawn.
//
// The rotation is the transpose of the camera's axes — the inverse of a rotation
// — and the translation column is minus each axis projected onto the eye, which
// is the inverse of the translation expressed in those axes. Written out rather
// than composed from two matrices and an inverse, because the closed form is
// three dot products and the general inverse is not free.
static voe_math_float4x4 look_at(voe_math_float3 eye, voe_math_float3 target,
				 voe_math_float3 up)
{
	voe_math_float3 forward = voe_math_float3_normalize(
		voe_math_float3_sub(target, eye));
	// The camera's own +Z, which points from the target back to the eye.
	voe_math_float3 z = voe_math_float3_neg(forward);
	voe_math_float3 x = voe_math_float3_normalize(
		voe_math_float3_cross(up, z));
	voe_math_float3 y = voe_math_float3_cross(z, x);
	voe_math_float4x4 view = { 0 };

	view.m[0][0] = x.x;
	view.m[0][1] = x.y;
	view.m[0][2] = x.z;
	view.m[0][3] = -voe_math_float3_dot(x, eye);

	view.m[1][0] = y.x;
	view.m[1][1] = y.y;
	view.m[1][2] = y.z;
	view.m[1][3] = -voe_math_float3_dot(y, eye);

	view.m[2][0] = z.x;
	view.m[2][1] = z.y;
	view.m[2][2] = z.z;
	view.m[2][3] = -voe_math_float3_dot(z, eye);

	view.m[3][3] = 1.0f;
	return view;
}

voe_math_float4x4 voe_render_cube_projection(VkExtent2D extent)
{
	// cot(fov/2): how far the near plane is from the eye in units of half
	// its own height.
	float focal = 1.0f / tanf(FIELD_OF_VIEW * 0.5f);
	float aspect;
	voe_math_float4x4 projection = { 0 };

	VOE_BASE_DEBUG_ASSERT(extent.width > 0 && extent.height > 0,
			      "a projection for a target with no area");

	aspect = (float)extent.width / (float)extent.height;

	// DEPTH RUNS BACKWARDS AND THE FAR PLANE IS AT INFINITY. Read the third
	// row as arithmetic: clip.z is NEAR_PLANE for every vertex and clip.w is
	// -z, so the depth the rasteriser sees is NEAR_PLANE / -z. At the near
	// plane that is 1.0; as z goes to negative infinity it approaches 0.0
	// and never reaches it. That is this engine's convention — near 1, far
	// 0, cleared to 0, compared GREATER — and it is why there is no far
	// plane to pass in and nothing to clip against at the back.
	//
	// Every tutorial writes the reciprocal of this and CLAUDE.md says not to
	// correct it. A float depth buffer has its precision bunched near zero,
	// which is where the distance is now, so this arrangement spends
	// precision where the geometry is instead of where it is not.
	//
	// NOTHING HERE NEGATES Y. Vulkan's clip space is Y-down and the whole of
	// the reconciliation is the negative viewport height in frame.c. A minus
	// sign on m[1][1] as well would flip twice, and flipping twice is
	// invisible until something is culled.
	projection.m[0][0] = focal / aspect;
	projection.m[1][1] = focal;
	projection.m[2][3] = NEAR_PLANE;
	projection.m[3][2] = -1.0f;
	return projection;
}

void voe_render_cube_uniforms_fill(struct voe_render_uniforms *uniforms,
				   VkExtent2D extent)
{
	voe_math_float3 eye = { EYE_X, EYE_Y, EYE_Z };
	voe_math_float3 origin = { 0.0f, 0.0f, 0.0f };
	voe_math_float3 up = { 0.0f, 1.0f, 0.0f };

	VOE_BASE_DEBUG_ASSERT(uniforms != NULL, "filling nothing with matrices");

	// The identity, and it earns its place by being the slot the next thing
	// writes into rather than by doing anything today: the cube is at the
	// origin, nothing moves it, and the card that gives something a
	// transform is the card that fills this in. Sent because the shader
	// reads three matrices and a shader that read two would have to be
	// rewritten then.
	uniforms->model = voe_math_float4x4_identity();
	uniforms->view = look_at(eye, origin, up);
	uniforms->projection = voe_render_cube_projection(extent);
}

// ------------------------------------------------------- the descriptor and

// One binding, and both stages named. The vertex stage is the one that
// transforms a position; the fragment stage is there because
// matrix_probe.slang reads this same buffer from a fragment shader, which is the
// only way a shader in this engine can report a value back to the CPU. A layout
// that named the vertex stage alone would make render/tests/matrix.c invalid
// rather than failing.
static bool build_descriptor_layout(voe_render_device *device)
{
	VkDescriptorSetLayoutBinding binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
			      VK_SHADER_STAGE_FRAGMENT_BIT,
	};
	VkDescriptorSetLayoutCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &binding,
	};
	VkDescriptorPoolSize size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = VOE_RENDER_FRAMES_IN_FLIGHT,
	};
	// Sized for exactly the sets that will ever be asked for, and without
	// FREE_DESCRIPTOR_SET: nothing frees one, the pool goes away whole at
	// shutdown, and a pool that cannot free is a pool that cannot be asked
	// to free by mistake.
	VkDescriptorPoolCreateInfo pool = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = VOE_RENDER_FRAMES_IN_FLIGHT,
		.poolSizeCount = 1,
		.pPoolSizes = &size,
	};
	VkResult result;

	result = voe_render_vk.create_descriptor_set_layout(
		device->device, &info, NULL, &device->descriptor_layout);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateDescriptorSetLayout failed (VkResult %d)\n",
			(int)result);
		device->descriptor_layout = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.create_descriptor_pool(device->device, &pool,
						      NULL,
						      &device->descriptor_pool);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkCreateDescriptorPool failed (VkResult %d)\n",
			(int)result);
		device->descriptor_pool = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

// One uniform buffer and one descriptor set per frame slot, mapped for good and
// pointed at each other. Every slot's set is allocated in one call because the
// pool is sized for exactly this many and a partial allocation would leave it in
// a state nothing here knows how to describe.
static bool build_slots(voe_render_device *device)
{
	VkDescriptorSetLayout layouts[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkDescriptorSet sets[VOE_RENDER_FRAMES_IN_FLIGHT];
	VkDescriptorSetAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorSetCount = VOE_RENDER_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts,
	};
	VkResult result;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++)
		layouts[i] = device->descriptor_layout;

	allocate.descriptorPool = device->descriptor_pool;
	result = voe_render_vk.allocate_descriptor_sets(device->device,
						       &allocate, sets);
	if (result != VK_SUCCESS) {
		fprintf(stderr,
			"render: vkAllocateDescriptorSets failed (VkResult %d)\n",
			(int)result);
		return false;
	}

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];
		VkDescriptorBufferInfo buffer_info = {
			.offset = 0,
			.range = sizeof(struct voe_render_uniforms),
		};
		VkWriteDescriptorSet write = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.pBufferInfo = &buffer_info,
		};

		if (!voe_render_buffer_build(
			    device, &frame->uniforms,
			    sizeof(struct voe_render_uniforms),
			    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			return false;

		// Mapped once and never unmapped until teardown: written every
		// frame, so there is nothing to be gained by asking twice.
		result = voe_render_vk.map_memory(device->device,
						  frame->uniforms.memory, 0,
						  VK_WHOLE_SIZE, 0,
						  &frame->uniforms_mapped);
		if (result != VK_SUCCESS || frame->uniforms_mapped == NULL) {
			fprintf(stderr,
				"render: vkMapMemory failed on a uniform buffer (VkResult %d)\n",
				(int)result);
			return false;
		}

		frame->descriptor = sets[i];
		buffer_info.buffer = frame->uniforms.buffer;
		write.dstSet = frame->descriptor;
		voe_render_vk.update_descriptor_sets(device->device, 1, &write,
						     0, NULL);
	}

	return true;
}

// The geometry, in device-local memory, through a staging buffer. TRANSFER_DST
// as well as its real usage, because that is what the copy writes into.
static bool build_geometry(voe_render_device *device)
{
	if (!voe_render_buffer_build(device, &device->vertices,
				     sizeof(CUBE_VERTICES),
				     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
					     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		return false;
	if (!voe_render_buffer_upload(device, &device->vertices, CUBE_VERTICES,
				      sizeof(CUBE_VERTICES)))
		return false;

	if (!voe_render_buffer_build(device, &device->indices,
				     sizeof(CUBE_INDICES),
				     VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		return false;
	if (!voe_render_buffer_upload(device, &device->indices, CUBE_INDICES,
				      sizeof(CUBE_INDICES)))
		return false;

	// The draw's count, not the buffer's size. Set last, so that a device
	// whose geometry failed to build has a zero here rather than a count
	// pointing at a buffer that does not exist.
	device->index_count =
		(uint32_t)(sizeof(CUBE_INDICES) / sizeof(CUBE_INDICES[0]));
	return true;
}

bool voe_render_cube_build(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "building a cube for nothing");

	return build_descriptor_layout(device) && build_slots(device) &&
	       build_geometry(device);
}

void voe_render_cube_teardown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "tearing down a cube on nothing");

	if (device->device == VK_NULL_HANDLE)
		return;

	voe_render_buffer_teardown(device, &device->indices);
	voe_render_buffer_teardown(device, &device->vertices);
	device->index_count = 0;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];

		if (frame->uniforms_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->uniforms.memory);
			frame->uniforms_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->uniforms);
		// Not freed on its own: the pool below owns every set and was
		// created without FREE_DESCRIPTOR_SET.
		frame->descriptor = VK_NULL_HANDLE;
	}

	if (device->descriptor_pool != VK_NULL_HANDLE) {
		voe_render_vk.destroy_descriptor_pool(device->device,
						      device->descriptor_pool,
						      NULL);
		device->descriptor_pool = VK_NULL_HANDLE;
	}
	if (device->descriptor_layout != VK_NULL_HANDLE) {
		voe_render_vk.destroy_descriptor_set_layout(
			device->device, device->descriptor_layout, NULL);
		device->descriptor_layout = VK_NULL_HANDLE;
	}
}
