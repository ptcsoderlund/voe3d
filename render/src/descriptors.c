// Everything the shader reads, and the one layout that describes it: the
// descriptor set layout, the pool, and per frame slot one set, one mapped
// uniform buffer holding a camera, a sun and a shadow record for every pass,
// mapped buffers of per-object records, element records, point lights and their
// bins and light blockers, and the slot's shadow maps; and the probe volumes'
// two samplers.
//
// TWELVE BINDINGS, AND THE SPLIT IS BY HOW OFTEN EACH CHANGES:
//
//   0  the camera, the sun and its shadow record, one block per pass in one
//      uniform buffer per frame slot, written as each pass opens. A DYNAMIC
//      uniform buffer: every bind of the set names the offset of the pass's
//      block, which is how one set serves every pass without a set per pass
//   1  every texture at once, one descriptor array, rewritten when a texture
//      is created or destroyed and never during a frame
//   2  the per-object records, one storage buffer per frame slot, written as
//      the frame records its draws
//   3  the shading records, one storage buffer shared by every slot, written
//      once per record at startup
//   4  the element records, one storage buffer per frame slot, written as the
//      frame submits them
//   5  the slot's shadow maps as one array, through shadow.c's comparison
//      sampler, written once at startup. One per slot because each slot draws
//      its own maps while the card may still be reading the other's
//   6  the probe volumes' sums, four images each (the x, y and z axis
//      images of six-axis irradiance, then the validity),
//      (targets + 1) × 4 entries, the window's first; written as each volume
//      is built (ADR-0326 point 7), partially bound until then
//   7  the point lights, 8 their tile and slice masks (ADR-0320): storage
//      buffers per frame slot, one region per pass, written as each pass opens
//   9  the slot's point shadow maps, all 96 layers, through binding 5's
//      comparison sampler, written once at startup (ADR-0325)
//  10  the probe volumes' moments atlases, one each, (targets + 1) entries,
//      through a linear, clamping sampler; written with binding 6
//  11  the light blockers and their point lights' masks (ADR-0347): a storage
//      buffer per frame slot, one region per pass, written as each pass opens
//
// BINDING 4 IS IN THE SAME LAYOUT THOUGH draw.slang DOES NOT READ IT, AND THAT
// IS THE POINT. shaders/elements.slang reads it and shares this layout, so the
// descriptor set bound as a pass opens serves both — a second layout
// would be a second set, a second pool entry and a rebind between every mesh
// draw and every element draw. Vulkan does not require a shader to declare
// every binding its layout has.
//
// The one thing a mesh draw is told that the draw beside it is not is its object
// number, and that is still the only push constant a mesh draw uses; the element
// pipeline pushes its surface transform through the same range. See the range in
// device.c.
//
// THE POOL IS SIZED EXACTLY AND NEVER GROWS. VOE_RENDER_FRAMES_IN_FLIGHT sets
// are allocated once at startup and freed by destroying the pool; no set is
// destroyed on its own anywhere in this folder. Every per-slot buffer stays
// mapped for its whole life (see build_mapped).
//
// WRITING THEM IS SAFE BECAUSE OF THE FENCE AT THE TOP OF THE FRAME. The cameras,
// the suns, the object records and the element records a frame writes are in this
// slot's own buffers, which the GPU may have been reading until that fence was
// signalled — and the fence is waited on before anything here is touched.
#include "device_internal.h"
#include "light_bins.h"

#include <base/assert.h>
#include <base/report.h>

#include <stddef.h>

// Every record a shader reads by index has to have the same layout on both
// sides of the bus, and every member of every one of them starts on a
// sixteen-byte boundary so that the two shader layout rules cannot disagree
// about them. These are what turns "somebody removed the padding" into a build
// error rather than a picture that is wrong in a way nobody can see.
static_assert(sizeof(voe_render_object) == 192,
	      "voe_render_object no longer matches the shader's per-object record");
static_assert(sizeof(voe_render_shading_values) == 96,
	      "voe_render_shading_values no longer matches the shader's shading record");
static_assert(sizeof(voe_render_view) == 144,
	      "voe_render_view no longer matches the shader's camera block");
static_assert(sizeof(voe_render_light) == 48,
	      "voe_render_light no longer matches the shader's light block");
static_assert(sizeof(voe_render_shadow) == 304,
	      "voe_render_shadow no longer matches the shader's shadow block");
static_assert(sizeof(struct voe_render_frame_block) == 544,
	      "the per-pass block no longer matches what draw.slang reads at binding 0");

// And the offsets, because the sizes above can stay right while the order goes
// wrong. Every member a buffer layout rule would have moved is named here: the
// ones that follow a matrix or a vector, and the run of texture ids.
static_assert(offsetof(voe_render_object, normal) == 64,
	      "the object record's normal matrix moved; draw.slang has it at 64");
static_assert(offsetof(voe_render_object, shading) == 128,
	      "the object record's shading index moved; draw.slang has it at 128");
static_assert(offsetof(voe_render_object, colour) == 144,
	      "the object record's colour moved; draw.slang has it at 144");
static_assert(offsetof(voe_render_object, waves) == 160,
	      "the object record's waves moved; draw.slang has them at 160");
static_assert(offsetof(voe_render_object, sky) == 176,
	      "the object record's sky moved; draw.slang has it at 176");
static_assert(offsetof(voe_render_view, eye) == 128,
	      "the camera block's eye moved; draw.slang has it at 128");
static_assert(offsetof(voe_render_light, colour) == 16,
	      "the light's colour moved; draw.slang has it at 16");
static_assert(offsetof(voe_render_light, unshaded) == 28,
	      "the light's unshaded flag moved; draw.slang has it at 28");
static_assert(offsetof(voe_render_light, fill) == 32,
	      "the light's fill moved; draw.slang has it at 32");
static_assert(offsetof(struct voe_render_frame_block, light) == 144,
	      "the sun moved inside the per-pass block; draw.slang has it at 144");
static_assert(offsetof(struct voe_render_frame_block, shadow) == 192,
	      "the shadow record moved inside the per-pass block; draw.slang has it at 192");
static_assert(offsetof(struct voe_render_frame_block, depth_copy) == 496,
	      "the depth copy slot moved inside the per-pass block; draw.slang has it at 496");
static_assert(offsetof(struct voe_render_frame_block, lights) == 500,
	      "the point light count moved inside the per-pass block; draw.slang has it at 500");
static_assert(offsetof(struct voe_render_frame_block, region) == 504,
	      "the point light region moved inside the per-pass block; draw.slang has it at 504");
static_assert(offsetof(struct voe_render_frame_block, blockers) == 508,
	      "the light blocker count moved inside the per-pass block; draw.slang has it at 508");
static_assert(sizeof(struct voe_render_frame_blockers) == 32 * 64 + 256 * 4,
	      "the blocker region no longer matches what draw.slang reads at binding 11");
static_assert(sizeof(struct voe_render_light_bins) == 1408 * 4,
	      "the light bins no longer match the 1408 words draw.slang reads at binding 8");
static_assert(offsetof(struct voe_render_frame_block, bounce) == 512,
	      "the bounce record moved inside the per-pass block; draw.slang has it at 512");
static_assert(offsetof(struct voe_render_frame_bounce, grid) == 12,
	      "the bounce record's grid moved; draw.slang has it at 12");
static_assert(offsetof(struct voe_render_frame_bounce, cell) == 16,
	      "the bounce record's cell moved; draw.slang has it at 16");
static_assert(offsetof(struct voe_render_frame_bounce, spacing) == 28,
	      "the bounce record's spacing moved; draw.slang has it at 28");
static_assert(offsetof(voe_render_shadow, splits) == 256,
	      "the shadow record's splits moved; draw.slang has them at 256");
static_assert(offsetof(voe_render_shadow, texels) == 272,
	      "the shadow record's texels moved; draw.slang has them at 272");
static_assert(offsetof(voe_render_shadow, count) == 288,
	      "the shadow record's count moved; draw.slang has it at 288");
static_assert(offsetof(voe_render_shading_values, emissive) == 32,
	      "the shading record's emissive colour moved; draw.slang has it at 32");
static_assert(offsetof(voe_render_shading_values, base_colour_texture) == 48,
	      "the shading record's texture ids moved; draw.slang has them at 48");
static_assert(offsetof(voe_render_shading_values, water) == 72,
	      "the shading record's water flag moved; draw.slang has it at 72");
static_assert(offsetof(voe_render_shading_values, base_colour_uv_rect) == 80,
	      "the shading record's UV rect moved; draw.slang has it at 80");

// And the element record, which elements.slang declares rather than draw.slang.
// Eighty and not sixty-four because of the spare words after `kind`, one of
// which and the float4 after them are what the glyph kind's texture index and
// sheet rectangle now occupy — in place, moving nothing and changing no size,
// which is what they were reserved for. Two words are still spare.
static_assert(sizeof(voe_render_element) == 80,
	      "voe_render_element no longer matches the record elements.slang reads");
static_assert(offsetof(voe_render_element, clip) == 16,
	      "the element record's clip rect moved; elements.slang has it at 16");
static_assert(offsetof(voe_render_element, colour) == 32,
	      "the element record's colour moved; elements.slang has it at 32");
static_assert(offsetof(voe_render_element, kind) == 48,
	      "the element record's kind moved; elements.slang has it at 48");
static_assert(offsetof(voe_render_element, sheet_texture) == 52,
	      "the element record's sheet texture moved; elements.slang has it at 52");
static_assert(offsetof(voe_render_element, sheet) == 64,
	      "the element record's sheet rect moved; elements.slang has it at 64");

static bool build_layout(voe_render_device *device)
{
	const uint32_t grid_entries = (device->capacities.targets + 1) * 4;
	const uint32_t moment_entries = device->capacities.targets + 1;
	VkDescriptorSetLayoutBinding bindings[12] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
				      VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 1,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = VOE_RENDER_MAX_TEXTURES,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			// The vertex stage reads the world matrix out of it and
			// the fragment stage reads which shading record to use,
			// so both stages are named.
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
				      VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 3,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			// The vertex stage reads an element's bounds out of it
			// and the fragment stage reads its colour and its clip
			// rectangle, so both stages are named — the same shape
			// binding 2 has.
			.binding = 4,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
				      VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 5,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 6,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = grid_entries,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 7,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			// The vertex stage too: the point-shadow pass places
			// casters about its lights (ADR-0325).
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
				      VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 8,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 9,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 10,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = moment_entries,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
		{
			.binding = 11,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		},
	};
	// A volume not built is never written, or names images since freed, so
	// bindings 6 and 10 are partially bound: only the volume a pass names
	// has to be valid.
	const VkDescriptorBindingFlags flags[12] = {
		[6] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
		[10] = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
	};
	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount = 12,
		.pBindingFlags = flags,
	};
	VkDescriptorSetLayoutCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = &binding_flags,
		.bindingCount = 12,
		.pBindings = bindings,
	};
	VkDescriptorPoolSize sizes[3] = {
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
			.descriptorCount = VOE_RENDER_FRAMES_IN_FLIGHT,
		},
		{
			// The texture array, the shadow maps, the volumes' sums,
			// the point shadow maps and the volumes' moments.
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = VOE_RENDER_FRAMES_IN_FLIGHT *
					   (VOE_RENDER_MAX_TEXTURES + 1 +
					    grid_entries + 1 + moment_entries),
		},
		{
			// Six per set: the objects, the shadings, the
			// elements, the point lights, their bins and the
			// light blockers.
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = VOE_RENDER_FRAMES_IN_FLIGHT * 6,
		},
	};
	VkDescriptorPoolCreateInfo pool = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = VOE_RENDER_FRAMES_IN_FLIGHT,
		.poolSizeCount = 3,
		.pPoolSizes = sizes,
	};
	VkResult result;

	result = voe_render_vk.create_descriptor_set_layout(
		device->device, &info, NULL, &device->descriptor_layout);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateDescriptorSetLayout failed (VkResult %d)",
			       (int)result);
		device->descriptor_layout = VK_NULL_HANDLE;
		return false;
	}

	result = voe_render_vk.create_descriptor_pool(device->device, &pool,
						      NULL,
						      &device->descriptor_pool);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateDescriptorPool failed (VkResult %d)",
			       (int)result);
		device->descriptor_pool = VK_NULL_HANDLE;
		return false;
	}

	return true;
}

// One host-visible buffer, mapped and left mapped. Both per-slot buffers are
// made this way and the only difference is their size and their usage, which is
// why this is a function and not two copies of eleven lines. Returns the
// mapping, or NULL on the one way this fails (rule 13); a successful mapping
// is never NULL, so the two cannot be confused.
//
// EVERY PER-SLOT BUFFER STAYS MAPPED FOR ITS WHOLE LIFE. All of them are
// host-visible and coherent and all are written every frame, so mapping
// and unmapping around each write would be two driver calls to say what one
// pointer already says.
[[nodiscard]] static void *build_mapped(voe_render_device *device,
					struct voe_render_buffer *buffer,
					VkDeviceSize size,
					VkBufferUsageFlags usage)
{
	void *mapped;
	VkResult result;

	if (!voe_render_buffer_build(device, buffer, size, usage,
				     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return NULL;

	result = voe_render_vk.map_memory(device->device, buffer->memory, 0,
					  VK_WHOLE_SIZE, 0, &mapped);
	if (result != VK_SUCCESS || mapped == NULL) {
		VOE_BASE_ERROR("render",
			       "vkMapMemory failed on a per-frame buffer (VkResult %d)",
			       (int)result);
		return NULL;
	}
	return mapped;
}

// The per-pass block's size rounded up to the card's uniform offset alignment,
// which Vulkan guarantees is a power of two. A dynamic offset that is not a
// multiple of the alignment is invalid, so the blocks cannot simply sit
// sizeof(block) apart.
static VkDeviceSize pass_stride(const voe_render_device *device)
{
	VkPhysicalDeviceProperties properties;
	VkDeviceSize align;

	voe_render_vk.get_physical_device_properties(device->physical,
						     &properties);
	align = properties.limits.minUniformBufferOffsetAlignment;
	if (align == 0)
		align = 1;
	VOE_BASE_DEBUG_ASSERT((align & (align - 1)) == 0,
			      "a uniform offset alignment that is not a power of two");

	return (sizeof(struct voe_render_frame_block) + align - 1) & ~(align - 1);
}

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

	device->pass_stride = pass_stride(device);

	allocate.descriptorPool = device->descriptor_pool;
	result = voe_render_vk.allocate_descriptor_sets(device->device,
						       &allocate, sets);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkAllocateDescriptorSets failed (VkResult %d)",
			       (int)result);
		return false;
	}

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];
		// One block's range: the dynamic offset a pass binds with is
		// what moves it along the buffer.
		VkDescriptorBufferInfo camera = {
			.offset = 0,
			.range = sizeof(struct voe_render_frame_block),
		};
		VkDescriptorBufferInfo objects = {
			.offset = 0,
			.range = (VkDeviceSize)device->capacities.objects *
				 sizeof(voe_render_object),
		};
		// At least one, on a device that asked for no elements: a
		// descriptor the layout declares has to be a valid one whether
		// the pipeline reading it is ever used or not, and eighty bytes
		// is cheaper than a conditional descriptor and a rule about
		// when this set may be bound. What refuses a submit is the
		// capacity, not the size of this buffer — see
		// voe_render_frame_submit_element.
		VkDescriptorBufferInfo elements = {
			.offset = 0,
			.range = (VkDeviceSize)(device->capacities.elements > 0 ?
							device->capacities.elements :
							1) *
				 sizeof(voe_render_element),
		};
		VkDescriptorImageInfo shadow = {
			.sampler = device->shadow_sampler,
			.imageView = frame->shadow.array,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		VkDescriptorImageInfo point_shadow = {
			.sampler = device->shadow_sampler,
			.imageView = frame->point_shadow.sampled,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};
		// One region per pass in each, region = the pass's number.
		VkDescriptorBufferInfo lights = {
			.offset = 0,
			.range = (VkDeviceSize)device->capacities.passes *
				 VOE_RENDER_POINT_LIGHTS *
				 sizeof(voe_render_point_light),
		};
		VkDescriptorBufferInfo bins = {
			.offset = 0,
			.range = (VkDeviceSize)device->capacities.passes *
				 sizeof(struct voe_render_light_bins),
		};
		VkDescriptorBufferInfo blockers = {
			.offset = 0,
			.range = (VkDeviceSize)device->capacities.passes *
				 sizeof(struct voe_render_frame_blockers),
		};
		VkWriteDescriptorSet writes[8] = {
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 0,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				.pBufferInfo = &camera,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 2,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &objects,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 4,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &elements,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 5,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &shadow,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 7,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &lights,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 8,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bins,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 9,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &point_shadow,
			},
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstBinding = 11,
				.descriptorCount = 1,
				.descriptorType =
					VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &blockers,
			},
		};

		frame->uniforms_mapped = build_mapped(
			device, &frame->uniforms,
			(VkDeviceSize)device->capacities.passes *
				device->pass_stride,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		if (frame->uniforms_mapped == NULL)
			return false;
		frame->objects_mapped = build_mapped(
			device, &frame->objects, objects.range,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		if (frame->objects_mapped == NULL)
			return false;
		frame->elements_mapped = build_mapped(
			device, &frame->elements, elements.range,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		if (frame->elements_mapped == NULL)
			return false;
		frame->point_lights_mapped = build_mapped(
			device, &frame->point_lights, lights.range,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		if (frame->point_lights_mapped == NULL)
			return false;
		frame->light_bins_mapped = build_mapped(
			device, &frame->light_bins, bins.range,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		if (frame->light_bins_mapped == NULL)
			return false;
		frame->light_blockers_mapped = build_mapped(
			device, &frame->light_blockers, blockers.range,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		if (frame->light_blockers_mapped == NULL)
			return false;

		frame->descriptor = sets[i];
		camera.buffer = frame->uniforms.buffer;
		objects.buffer = frame->objects.buffer;
		elements.buffer = frame->elements.buffer;
		lights.buffer = frame->point_lights.buffer;
		bins.buffer = frame->light_bins.buffer;
		blockers.buffer = frame->light_blockers.buffer;
		for (uint32_t w = 0; w < 8; w++)
			writes[w].dstSet = frame->descriptor;
		voe_render_vk.update_descriptor_sets(device->device, 8, writes,
						     0, NULL);
	}

	return true;
}

void voe_render_descriptors_write_shadings(voe_render_device *device,
					   VkDescriptorSet set)
{
	VkDescriptorBufferInfo info = {
		.buffer = device->shadings.buffer,
		.offset = 0,
		.range = (VkDeviceSize)device->capacities.shadings *
			 sizeof(voe_render_shading_values),
	};
	VkWriteDescriptorSet write = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = set,
		.dstBinding = 3,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.pBufferInfo = &info,
	};

	VOE_BASE_DEBUG_ASSERT(device != NULL, "writing descriptors with no device");
	VOE_BASE_DEBUG_ASSERT(device->shadings.buffer != VK_NULL_HANDLE,
			      "pointing a descriptor at a shading buffer that does not exist");

	voe_render_vk.update_descriptor_sets(device->device, 1, &write, 0, NULL);
}

// One linear sampler with `address` on all three axes, or VK_NULL_HANDLE with
// a line naming `what`.
static VkSampler build_sampler(voe_render_device *device,
			       VkSamplerAddressMode address, const char *what)
{
	VkSamplerCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = address,
		.addressModeV = address,
		.addressModeW = address,
	};
	VkSampler sampler = VK_NULL_HANDLE;
	VkResult result;

	VOE_BASE_DEBUG_ASSERT(device != NULL && what != NULL,
			      "a sampler on no device");
	result = voe_render_vk.create_sampler(device->device, &info, NULL,
					      &sampler);
	if (result != VK_SUCCESS) {
		VOE_BASE_ERROR("render",
			       "vkCreateSampler failed for the %s (VkResult %d)",
			       what, (int)result);
		return VK_NULL_HANDLE;
	}
	return sampler;
}

// Binding 6's REPEAT, because a volume is addressed toroidally (ADR-0326
// point 2), and binding 10's CLAMP_TO_EDGE, because the read holds its uv half
// a texel inside a face and an atlas does not wrap.
static bool build_bounce_samplers(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "bounce samplers on no device");
	device->bounce_sampler = build_sampler(
		device, VK_SAMPLER_ADDRESS_MODE_REPEAT, "probe volumes' sums");
	device->moments_sampler =
		build_sampler(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
			      "probe volumes' moments");
	return device->bounce_sampler != VK_NULL_HANDLE &&
	       device->moments_sampler != VK_NULL_HANDLE;
}

bool voe_render_descriptors_build(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "building descriptors for nothing");
	VOE_BASE_DEBUG_ASSERT(device->capacities.objects > 0,
			      "a device with room for no drawn objects");
	VOE_BASE_DEBUG_ASSERT(device->capacities.passes > 0,
			      "a device with room for no passes");

	return build_layout(device) && build_slots(device) &&
	       build_bounce_samplers(device);
}

void voe_render_descriptors_write_volume(
	voe_render_device *device, const struct voe_render_bounce_volume *volume,
	uint32_t index)
{
	VkDescriptorImageInfo sums[4];
	VkDescriptorImageInfo moments = {
		.sampler = device->moments_sampler,
		.imageView = volume->moments.view,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkWriteDescriptorSet writes[2 * VOE_RENDER_FRAMES_IN_FLIGHT];

	VOE_BASE_DEBUG_ASSERT(device != NULL && volume != NULL && volume->built,
			      "writing descriptors for no device or an unbuilt volume");
	VOE_BASE_DEBUG_ASSERT(index <= device->capacities.targets,
			      "writing a volume past binding 6's (targets + 1) × 4 entries");

	for (uint32_t k = 0; k < 4; k++)
		sums[k] = (VkDescriptorImageInfo){
			.sampler = device->bounce_sampler,
			.imageView = k < 3 ? volume->irradiance[6][k].view :
					     volume->validity.view,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};
	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		writes[2 * i] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = device->frames[i].descriptor,
			.dstBinding = 6,
			.dstArrayElement = 4 * index,
			.descriptorCount = 4,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = sums,
		};
		writes[2 * i + 1] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = device->frames[i].descriptor,
			.dstBinding = 10,
			.dstArrayElement = index,
			.descriptorCount = 1,
			.descriptorType =
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo = &moments,
		};
	}
	voe_render_vk.update_descriptor_sets(device->device,
					     2 * VOE_RENDER_FRAMES_IN_FLIGHT,
					     writes, 0, NULL);
}

void voe_render_descriptors_teardown(voe_render_device *device)
{
	VOE_BASE_DEBUG_ASSERT(device != NULL, "tearing down descriptors on nothing");

	if (device->device == VK_NULL_HANDLE)
		return;

	for (uint32_t i = 0; i < VOE_RENDER_FRAMES_IN_FLIGHT; i++) {
		struct voe_render_frame *frame = &device->frames[i];

		if (frame->light_blockers_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->light_blockers.memory);
			frame->light_blockers_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->light_blockers);

		if (frame->light_bins_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->light_bins.memory);
			frame->light_bins_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->light_bins);

		if (frame->point_lights_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->point_lights.memory);
			frame->point_lights_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->point_lights);

		if (frame->elements_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->elements.memory);
			frame->elements_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->elements);

		if (frame->objects_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->objects.memory);
			frame->objects_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->objects);

		if (frame->uniforms_mapped != NULL) {
			voe_render_vk.unmap_memory(device->device,
						   frame->uniforms.memory);
			frame->uniforms_mapped = NULL;
		}
		voe_render_buffer_teardown(device, &frame->uniforms);

		// Not destroyed on its own: the pool below owns every set it
		// handed out.
		frame->descriptor = VK_NULL_HANDLE;
	}

	if (device->bounce_sampler != VK_NULL_HANDLE) {
		voe_render_vk.destroy_sampler(device->device,
					      device->bounce_sampler, NULL);
		device->bounce_sampler = VK_NULL_HANDLE;
	}
	if (device->moments_sampler != VK_NULL_HANDLE) {
		voe_render_vk.destroy_sampler(device->device,
					      device->moments_sampler, NULL);
		device->moments_sampler = VK_NULL_HANDLE;
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
