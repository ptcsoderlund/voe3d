// The proof that the C and Slang layouts of every record a shader reads agree:
// the per-object, shading, camera, sun, shadow and element records, the
// per-pass block, the blocker region and the light bins, by size and by the
// offset of every member a buffer layout rule could move. A dropped pad or a
// reordered member fails the build here instead of drawing a wrong picture.
//
// Whoever changes a record in device.h, device_parts.h or a .slang or .slangh
// file changes its asserts here in the same commit.
//
// It holds no code: only compile-time asserts, built as part of the folder.
#include "device_internal.h"
#include "light_bins.h"

#include <stddef.h>

// Every record a shader reads by index has to have the same layout on both
// sides of the bus, and every member of every one of them starts on a
// sixteen-byte boundary so that the two shader layout rules cannot disagree
// about them. These are what turns "somebody removed the padding" into a build
// error rather than a picture that is wrong in a way nobody can see.
static_assert(sizeof(voe_render_object) == 224,
	      "voe_render_object no longer matches the shader's per-object record");
static_assert(sizeof(voe_render_shading_values) == 96,
	      "voe_render_shading_values no longer matches the shader's shading record");
static_assert(sizeof(voe_render_view) == 144,
	      "voe_render_view no longer matches the shader's camera block");
static_assert(sizeof(voe_render_light) == 48,
	      "voe_render_light no longer matches the shader's light block");
static_assert(sizeof(voe_render_shadow) == 304,
	      "voe_render_shadow no longer matches the shader's shadow block");
static_assert(sizeof(voe_render_directional_light) == 368,
	      "voe_render_directional_light is no longer the frame light and its bounce words");
static_assert(sizeof(struct voe_render_frame_light) == 368,
	      "the frame light no longer matches bindings.slangh's");
static_assert(sizeof(struct voe_render_frame_block) == 656 + 3 * 368,
	      "the per-pass block no longer matches what draw.slang reads at binding 0");
static_assert(offsetof(struct voe_render_frame_block, more_count) == 640,
	      "the further light count moved inside the per-pass block; draw.slang has it at 640");
static_assert(offsetof(struct voe_render_frame_block, more) == 656,
	      "the further lights moved inside the per-pass block; draw.slang has them at 656");

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
static_assert(offsetof(voe_render_object, heights) == 132,
	      "the object record's heights moved; draw.slang has it at 132");
static_assert(offsetof(voe_render_object, terrain) == 192,
	      "the object record's terrain moved; draw.slang has it at 192");
static_assert(offsetof(voe_render_object, morph) == 208,
	      "the object record's morph moved; draw.slang has it at 208");
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
static_assert(sizeof(struct voe_render_frame_blockers) == 32 * 64 + 256 * 4 + 4 * 4,
	      "the blocker region no longer matches what draw.slang reads at binding 11");
static_assert(sizeof(struct voe_render_light_bins) == 1408 * 4,
	      "the light bins no longer match the 1408 words draw.slang reads at binding 8");
static_assert(offsetof(struct voe_render_frame_block, bounce) == 512,
	      "the bounce records moved inside the per-pass block; draw.slang has them at 512");
static_assert(sizeof(struct voe_render_frame_bounce) == 32,
	      "a bounce record is no longer the 32 bytes draw.slang strides them by");
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
