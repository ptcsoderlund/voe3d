// The device's pipeline cache: made empty at open, seeded from a caller's bytes
// before the first prepare step, and handed out as bytes once prepared
// (ADR-0370 point 6). device.c calls voe_render_pipeline_cache_create in
// open_device and destroys the handle in close_down; pipeline.c passes it to
// every mesh pipeline build. The two public calls are in render/device.h.
//
// THE BYTES ARE RENDER'S HEADER AND THEN THE DRIVER'S CACHE. The header holds a
// magic, a version, the card's vendor id, device id and pipelineCacheUUID, a
// 64-bit FNV-1a hash of the shaders pipeline.c and element.c embed, the
// payload's size and its FNV-1a checksum, every field native-endian. Any field
// off, or a buffer too short for the header and the size it claims, is a
// mismatch: the cache stays empty and seed answers false. The driver checks its
// own header inside the payload again, so a cache this header lets through and
// the driver still dislikes is ignored by the driver, never an error.
//
// SEEDING REPLACES THE EMPTY CACHE, it does not merge into it, and only before
// the first prepare step: a pipeline built against the empty one would be lost
// with it. The element pipeline, built at open, never goes through the cache.
//
// A deliberate ceiling: the checksum and the shader hash are byte-wise FNV-1a,
// a few milliseconds over a cache of megabytes; a wider hash would lift it.
#include "device_internal.h"

#include <base/assert.h>
#include <base/report.h>

#include <string.h>

// "VOEP", read as a little-endian word.
#define CACHE_MAGIC 0x50454f56u
// Raised whenever the header's layout or meaning changes.
#define CACHE_VERSION 1u

#define FNV_OFFSET 0xcbf29ce484222325u
#define FNV_PRIME 0x100000001b3u

struct cache_header {
	uint32_t magic;
	uint32_t version;
	uint32_t vendor_id;
	uint32_t device_id;
	uint8_t uuid[VK_UUID_SIZE];
	uint64_t shaders;
	uint64_t payload_size;
	uint64_t checksum;
};

static_assert(sizeof(struct cache_header) == 56,
	      "the cache header has padding, so its bytes are not all fields");

static uint64_t fnv1a(uint64_t hash, const void *bytes, size_t size)
{
	const unsigned char *at = bytes;

	for (size_t i = 0; i < size; i++)
		hash = (hash ^ at[i]) * FNV_PRIME;
	return hash;
}

// What this build on this card writes into a header; the payload's two fields
// are left nought for the caller.
static struct cache_header expected_header(const voe_render_device *device)
{
	VkPhysicalDeviceProperties properties;
	struct cache_header header = {
		.magic = CACHE_MAGIC,
		.version = CACHE_VERSION,
	};

	voe_render_vk.get_physical_device_properties(device->physical,
						     &properties);
	header.vendor_id = properties.vendorID;
	header.device_id = properties.deviceID;
	memcpy(header.uuid, properties.pipelineCacheUUID, VK_UUID_SIZE);
	header.shaders = fnv1a(FNV_OFFSET, voe_render_draw_spv,
			       voe_render_draw_spv_size);
	header.shaders = fnv1a(header.shaders, voe_render_elements_spv,
			       voe_render_elements_spv_size);
	return header;
}

// A cache holding `size` bytes of `payload`, or an empty one with NULL and 0.
static VkPipelineCache make_cache(const voe_render_device *device,
				  const void *payload, size_t size)
{
	VkPipelineCacheCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.initialDataSize = size,
		.pInitialData = payload,
	};
	VkPipelineCache cache = VK_NULL_HANDLE;

	if (voe_render_vk.create_pipeline_cache(device->device, &info, NULL,
						&cache) != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return cache;
}

bool voe_render_pipeline_cache_create(voe_render_device *device)
{
	device->pipeline_cache = make_cache(device, NULL, 0);
	if (device->pipeline_cache == VK_NULL_HANDLE) {
		VOE_BASE_ERROR("render", "vkCreatePipelineCache failed");
		return false;
	}
	return true;
}

bool voe_render_device_cache_seed(voe_render_device *device, const void *bytes,
				  size_t size)
{
	struct cache_header header;
	struct cache_header expected;
	const unsigned char *payload;
	VkPipelineCache seeded;

	VOE_BASE_ASSERT(device != NULL, "seeding the pipeline cache of no device");
	VOE_BASE_ASSERT(device->pipeline == VK_NULL_HANDLE &&
				!device->prepare_failed,
			"seeding the pipeline cache after the first prepare step");

	if (bytes == NULL || size < sizeof(header))
		return false;
	memcpy(&header, bytes, sizeof(header));
	payload = (const unsigned char *)bytes + sizeof(header);
	expected = expected_header(device);
	if (header.magic != expected.magic ||
	    header.version != expected.version ||
	    header.vendor_id != expected.vendor_id ||
	    header.device_id != expected.device_id ||
	    memcmp(header.uuid, expected.uuid, VK_UUID_SIZE) != 0 ||
	    header.shaders != expected.shaders || header.payload_size == 0 ||
	    header.payload_size != size - sizeof(header) ||
	    header.checksum != fnv1a(FNV_OFFSET, payload, size - sizeof(header)))
		return false;

	seeded = make_cache(device, payload, size - sizeof(header));
	if (seeded == VK_NULL_HANDLE)
		return false;
	voe_render_vk.destroy_pipeline_cache(device->device,
					     device->pipeline_cache, NULL);
	device->pipeline_cache = seeded;
	return true;
}

const void *voe_render_device_cache_bytes(voe_render_device *device,
					  voe_base_arena *arena, size_t *size)
{
	struct cache_header header;
	unsigned char *bytes;
	size_t payload_size = 0;

	VOE_BASE_ASSERT(device != NULL && arena != NULL && size != NULL,
			"handing out the pipeline cache with no device, arena or size");

	*size = 0;
	if (voe_render_vk.get_pipeline_cache_data(device->device,
						  device->pipeline_cache,
						  &payload_size, NULL) != VK_SUCCESS ||
	    payload_size == 0)
		return NULL;
	bytes = voe_base_arena_push(arena, sizeof(header) + payload_size);
	// VK_INCOMPLETE, should the cache have grown between the two calls, is
	// refused with the rest: a cut payload is no cache.
	if (voe_render_vk.get_pipeline_cache_data(device->device,
						  device->pipeline_cache,
						  &payload_size,
						  bytes + sizeof(header)) != VK_SUCCESS)
		return NULL;

	header = expected_header(device);
	header.payload_size = payload_size;
	header.checksum = fnv1a(FNV_OFFSET, bytes + sizeof(header), payload_size);
	memcpy(bytes, &header, sizeof(header));
	*size = sizeof(header) + payload_size;
	return bytes;
}
