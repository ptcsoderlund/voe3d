// The import: a model's arrays uploaded through `render`, its node tree
// flattened, and one entity per drawn primitive.
//
// THE ORDER IS FORCED AND NOT CHOSEN. Pictures first, because a material names
// texture ids; materials next, because a primitive names a material; geometry
// next, because an entity names a range; the tree last, because an entity needs
// all three. Nothing here can be reordered without something naming an id that
// does not exist yet.
//
// THE ONE EXCEPTION IS THAT THE UPLOAD OF THE PICTURES READS THE MATERIALS. A
// picture is uploaded in one of two colour spaces and nothing in a picture says
// which — only the material slot that referenced it does — so upload_images
// walks the materials for their texture indices before it uploads anything, and
// upload_materials then walks them again for everything else. Two walks over a
// handful of materials, and the alternative is uploading a picture lazily from
// inside the material loop, which puts a GPU upload behind a field
// initialisation.
//
// THE TREE IS WALKED WITH AN EXPLICIT STACK IN THE ARENA (rule 14). The stack is
// as long as the file has nodes, which is enough because `assets` has already
// checked that no node is claimed as a child twice — so a node is pushed at most
// once and the walk cannot loop. The depth limit beside it is the rule's named
// limit, and it is what a file with a legal but absurd nesting is refused past.
//
// A WORLD MATRIX IS DECOMPOSED BECAUSE THE COMPONENT HOLDS THREE PARTS. The
// translation comes straight out of the last column, the scale is the length of
// each basis vector, and the rotation is what the basis becomes once those
// lengths are divided out. See rotation_of() for which of the four ways of
// reading a quaternion out of a matrix this is and why the branch is there.
//
// NOTHING HERE UNWINDS ON FAILURE, and the header says so out loud: `render`
// cannot free geometry yet, and half a model in a world nobody drew costs
// nothing. A caller that cannot load its model destroys the world.
#include <3d/import.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <assets/model.h>
#include <base/assert.h>
#include <base/report.h>
#include <math/quat.h>
#include <scene/transform_system.h>

#include <math.h>

// Everything one import needs to carry between its steps.
struct import {
	voe_ecs_world *world;
	voe_render_device *device;
	voe_base_arena *arena;
	const voe_assets_model *model;

	// One per picture per kind, one per glTF material, one per primitive —
	// the ids that came back from the uploads, indexed the way the file
	// indexes them. A slot left at zero is a picture that was never wanted
	// that way round, which is VOE_RENDER_NO_TEXTURE and samples white.
	voe_render_texture *colours;
	voe_render_texture *data;
	voe_3d_material *materials;
	voe_render_geometry *geometries;

	voe_ecs_entity *entities;
	uint32_t entity_count;

	// How many pictures were actually uploaded, which is not the file's
	// image count when one picture is wanted both ways round.
	uint32_t texture_count;
};

// One frame of the tree walk: which node, where its parent put it, and how deep
// it is.
struct frame {
	uint32_t node;
	voe_math_float4x4 world;
	uint32_t depth;
};

static bool no_room(voe_base_error *error, const char *what)
{
	VOE_BASE_ERROR("3d",
		       "no room for %s while importing a model — the world or the device was made smaller than this file needs",
		       what);
	if (error != NULL)
		*error = VOE_BASE_ERROR_REFUSED;
	return false;
}

// Says a picture is wanted, ignoring the indices that name none. Every index
// here came out of a file, so the bound is checked where the array is reached
// rather than trusted from a folder away.
static void wanted(bool *marks, uint32_t count, uint32_t index)
{
	if (index < count)
		marks[index] = true;
}

// A texture id per picture in the file, in the kind each picture is wanted as.
//
// A PICTURE IS UPLOADED ONCE PER KIND AND NOT ONCE. A texture slot holds one
// format, and a base colour map has to be sRGB while an ORM map has to be raw
// (render/device.h) — so a file whose one picture is referenced as both costs
// two slots and there is no way round it short of two views onto one image,
// which is a card of its own. Nothing real does this: the two kinds of picture
// are authored differently and an exporter that shared one between them would be
// wrong about one of the two. What the two arrays buy is that such a file is
// merely wasteful here rather than wrong.
//
// WHICH KIND EACH PICTURE IS COMES OUT OF THE MATERIALS AND NOT OUT OF THE
// PICTURE. Nothing in a PNG says whether its bytes are a colour, so the only
// answer is which slot of which material referenced it, which is why the
// materials are walked before anything is uploaded even though the materials
// need the ids this produces.
//
// `render` DEDUPLICATES NOTHING: the deduplication is that `assets` resolved
// every glTF texture to a picture, so this uploads each one once per kind and
// every material that wanted it that way round gets the same id.
static bool upload_images(struct import *import, voe_base_error *error)
{
	const voe_assets_model *model = import->model;
	uint32_t count = model->image_count;
	bool *as_colour;
	bool *as_data;

	if (count == 0)
		return true;

	import->colours = voe_base_arena_push(
		import->arena, (size_t)count * sizeof(*import->colours));
	import->data = voe_base_arena_push(
		import->arena, (size_t)count * sizeof(*import->data));
	as_colour = voe_base_arena_push(import->arena,
					(size_t)count * sizeof(*as_colour));
	as_data = voe_base_arena_push(import->arena,
				      (size_t)count * sizeof(*as_data));

	for (uint32_t i = 0; i < model->material_count; i++) {
		const voe_assets_material *material = &model->materials[i];

		wanted(as_colour, count, material->base_colour_image);
		wanted(as_colour, count, material->emissive_image);
		wanted(as_data, count, material->metallic_roughness_image);
		wanted(as_data, count, material->normal_image);
		wanted(as_data, count, material->occlusion_image);
	}

	for (uint32_t i = 0; i < count; i++) {
		if (as_colour[i] &&
		    !voe_render_texture_create(import->device,
					       VOE_RENDER_TEXTURE_COLOUR,
					       VOE_RENDER_SAMPLING_SMOOTH,
					       model->images[i].width,
					       model->images[i].height,
					       model->images[i].pixels,
					       &import->colours[i], error))
			return false;
		if (as_data[i] &&
		    !voe_render_texture_create(import->device,
					       VOE_RENDER_TEXTURE_DATA,
					       VOE_RENDER_SAMPLING_SMOOTH,
					       model->images[i].width,
					       model->images[i].height,
					       model->images[i].pixels,
					       &import->data[i], error))
			return false;
		import->texture_count += (uint32_t)as_colour[i] +
					 (uint32_t)as_data[i];
	}
	return true;
}

// The id of the picture at `index` in the kind asked for, or the "there isn't
// one" id. A material naming no picture and a material naming one that was never
// uploaded are the same thing to a shader: it samples white.
static voe_render_texture texture_at(const struct import *import,
				     const voe_render_texture *ids,
				     uint32_t index)
{
	voe_render_texture none = { .index = VOE_RENDER_NO_TEXTURE,
				    .generation = 0 };

	if (ids == NULL || index == VOE_ASSETS_MODEL_NONE ||
	    index >= import->model->image_count)
		return none;
	return ids[index];
}

// `assets`' three words into `render`'s three words. Two enums and not one
// because `assets` may not name `render` — see 3d/material_component.h — and
// this is the one place in the engine where the mapping is written down. It is
// one to one and it is total, so the default below is unreachable rather than a
// fallback: the reader refuses a mode it does not know (assets/model.h) and
// never hands one over.
static voe_render_alpha_mode alpha_mode_of(voe_assets_alpha_mode mode)
{
	switch (mode) {
	case VOE_ASSETS_ALPHA_CUTOUT:
		return VOE_RENDER_ALPHA_CUTOUT;
	case VOE_ASSETS_ALPHA_BLENDED:
		return VOE_RENDER_ALPHA_BLENDED;
	case VOE_ASSETS_ALPHA_OPAQUE:
		break;
	}
	return VOE_RENDER_ALPHA_OPAQUE;
}

static bool upload_materials(struct import *import, voe_base_error *error)
{
	const voe_assets_model *model = import->model;

	if (model->material_count == 0)
		return true;

	import->materials = voe_base_arena_push(
		import->arena,
		(size_t)model->material_count * sizeof(*import->materials));

	for (uint32_t i = 0; i < model->material_count; i++) {
		const voe_assets_material *from = &model->materials[i];

		import->materials[i] = (voe_3d_material){
			.base_colour = from->base_colour,
			.metallic = from->metallic,
			.roughness = from->roughness,
			.emissive = from->emissive,
			.alpha_mode = alpha_mode_of(from->alpha_mode),
			.alpha_cutoff = from->alpha_cutoff,
			.base_colour_texture =
				texture_at(import, import->colours,
					   from->base_colour_image),
			.metallic_roughness_texture =
				texture_at(import, import->data,
					   from->metallic_roughness_image),
			.normal_texture = texture_at(import, import->data,
						     from->normal_image),
			.occlusion_texture =
				texture_at(import, import->data,
					   from->occlusion_image),
			.emissive_texture = texture_at(import,
						       import->colours,
						       from->emissive_image),
		};

		// One record per glTF material and not one per entity, so two
		// entities wearing one material share the record as well as the
		// texture ids.
		if (!voe_3d_material_upload(import->device,
					    &import->materials[i], error))
			return false;
	}
	return true;
}

// glTF's default material: white, fully metallic, fully rough, opaque, no
// pictures. It
// is what an unmaterialled primitive is *defined* to be, so this is the file's
// answer and not a substitute for it — and it gets a shading record of its own
// like any other material.
static bool default_material(struct import *import, voe_3d_material *out,
			     voe_base_error *error)
{
	*out = (voe_3d_material){
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 1.0f,
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_OPAQUE,
		.alpha_cutoff = 0.5f,
		.base_colour_texture = { .index = VOE_RENDER_NO_TEXTURE },
		.metallic_roughness_texture = { .index = VOE_RENDER_NO_TEXTURE },
		.normal_texture = { .index = VOE_RENDER_NO_TEXTURE },
		.occlusion_texture = { .index = VOE_RENDER_NO_TEXTURE },
		.emissive_texture = { .index = VOE_RENDER_NO_TEXTURE },
	};

	return voe_3d_material_upload(import->device, out, error);
}

// One primitive's attributes, interleaved into whatever a vertex is on the GPU.
// The interleaving is here because the layout is `render`'s and `assets` may not
// name it: separate arrays go in, one array of vertices comes out.
static bool upload_geometry(struct import *import, voe_base_error *error)
{
	const voe_assets_model *model = import->model;
	struct voe_base_arena_mark mark;

	if (model->primitive_count == 0)
		return true;

	import->geometries = voe_base_arena_push(
		import->arena,
		(size_t)model->primitive_count * sizeof(*import->geometries));

	for (uint32_t i = 0; i < model->primitive_count; i++) {
		const voe_assets_primitive *primitive = &model->primitives[i];
		voe_render_vertex *vertices;

		// The interleaved copy is scratch: `render` has taken its own
		// copy by the time the upload returns, so the arena goes back
		// to where it was before the next primitive.
		mark = voe_base_arena_mark(import->arena);
		vertices = voe_base_arena_push(
			import->arena, (size_t)primitive->vertex_count *
					       sizeof(*vertices));

		for (uint32_t v = 0; v < primitive->vertex_count; v++) {
			vertices[v].position = primitive->positions[v];
			// A file without normals leaves them at nothing rather
			// than having this invent them. The card that lights
			// anything is the card that decides what a mesh with no
			// normals should look like.
			vertices[v].normal =
				primitive->normals != NULL ?
					primitive->normals[v] :
					(voe_math_float3){ 0.0f, 0.0f, 0.0f };
			vertices[v].uv = primitive->uvs != NULL ?
						 primitive->uvs[v] :
						 (voe_math_float2){ 0.0f, 0.0f };
		}

		if (!voe_render_geometry_create(import->device, vertices,
						primitive->vertex_count,
						primitive->indices,
						primitive->index_count,
						&import->geometries[i], error)) {
			voe_base_arena_rewind(import->arena, mark);
			return false;
		}

		voe_base_arena_rewind(import->arena, mark);
	}
	return true;
}

// ------------------------------------------------------- the node tree

// A rotation out of the rotation part of a basis, which is the one piece of
// arithmetic in this file that is not obvious.
//
// FOUR BRANCHES, AND THE BRANCH IS FOR PRECISION AND NOT FOR CORRECTNESS. Each
// component of the quaternion can be recovered from the diagonal, and each
// formula divides by that component — so the one to use is the largest, and the
// trace says which. Taking the w branch always is the version that works until
// something is turned by half a turn, where w is zero and the division is by
// nothing.
//
// THE SIGNS FOLLOW voe_math_float4x4_from_quat AND ARE PROVEN BY A ROUND TRIP.
// A quaternion read out with x, y and z negated describes the opposite rotation,
// which draws a mirrored model — the exact failure this card's verification
// looks for. 3d/tests/import.c composes the transform back into a matrix and
// checks a known point, which is what catches it.
static voe_math_quat rotation_of(const voe_math_float4x4 *basis)
{
	float trace = basis->m[0][0] + basis->m[1][1] + basis->m[2][2];
	voe_math_quat q;
	float root;
	float scale;

	if (trace > 0.0f) {
		root = sqrtf(trace + 1.0f);
		scale = 0.5f / root;
		q.w = 0.5f * root;
		q.x = (basis->m[2][1] - basis->m[1][2]) * scale;
		q.y = (basis->m[0][2] - basis->m[2][0]) * scale;
		q.z = (basis->m[1][0] - basis->m[0][1]) * scale;
		return q;
	}

	if (basis->m[0][0] >= basis->m[1][1] &&
	    basis->m[0][0] >= basis->m[2][2]) {
		root = sqrtf(1.0f + basis->m[0][0] - basis->m[1][1] -
			     basis->m[2][2]);
		scale = 0.5f / root;
		q.x = 0.5f * root;
		q.y = (basis->m[0][1] + basis->m[1][0]) * scale;
		q.z = (basis->m[0][2] + basis->m[2][0]) * scale;
		q.w = (basis->m[2][1] - basis->m[1][2]) * scale;
		return q;
	}

	if (basis->m[1][1] >= basis->m[2][2]) {
		root = sqrtf(1.0f + basis->m[1][1] - basis->m[0][0] -
			     basis->m[2][2]);
		scale = 0.5f / root;
		q.y = 0.5f * root;
		q.x = (basis->m[0][1] + basis->m[1][0]) * scale;
		q.z = (basis->m[1][2] + basis->m[2][1]) * scale;
		q.w = (basis->m[0][2] - basis->m[2][0]) * scale;
		return q;
	}

	root = sqrtf(1.0f + basis->m[2][2] - basis->m[0][0] - basis->m[1][1]);
	scale = 0.5f / root;
	q.z = 0.5f * root;
	q.x = (basis->m[0][2] + basis->m[2][0]) * scale;
	q.y = (basis->m[1][2] + basis->m[2][1]) * scale;
	q.w = (basis->m[1][0] - basis->m[0][1]) * scale;
	return q;
}

// A world matrix into the three parts the transform component holds. Vectors are
// columns, so the transformed axes are the matrix's columns and their lengths
// are the scale; the translation is the last column.
static voe_scene_transform decomposed(voe_math_float4x4 world)
{
	voe_math_float3 axes[3];
	voe_math_float4x4 basis = world;
	voe_scene_transform transform;
	float lengths[3];

	for (uint32_t column = 0; column < 3; column++) {
		axes[column] = (voe_math_float3){ world.m[0][column],
						  world.m[1][column],
						  world.m[2][column] };
		lengths[column] = voe_math_float3_length(axes[column]);
	}

	// Widened into the double position (ADR-0250).
	transform.position = (voe_math_double3){ world.m[0][3], world.m[1][3],
						 world.m[2][3] };
	transform.scale = (voe_math_float3){ lengths[0], lengths[1],
					     lengths[2] };

	// A zero-length axis is a scale of nothing in that direction — a
	// flattened object, which a file may legitimately hold. There is no
	// rotation to recover from it, so the axis is left as it is and the
	// quaternion comes out of whatever basis remains; dividing by it would
	// be a division by zero and an object full of NaNs.
	for (uint32_t column = 0; column < 3; column++) {
		float length = lengths[column] > 0.0f ? lengths[column] : 1.0f;

		basis.m[0][column] = axes[column].x / length;
		basis.m[1][column] = axes[column].y / length;
		basis.m[2][column] = axes[column].z / length;
	}

	transform.rotation = rotation_of(&basis);
	return transform;
}

// One entity per primitive of the node's mesh, with the node's world transform.
static bool place_node(struct import *import, const struct frame *frame,
		       voe_base_error *error)
{
	const voe_assets_model *model = import->model;
	const voe_assets_node *node = &model->nodes[frame->node];
	const voe_assets_mesh *mesh;
	voe_scene_transform transform;

	if (node->mesh == VOE_ASSETS_MODEL_NONE)
		return true;

	// A node naming a mesh the file does not describe cannot get this far —
	// the reader checks it — but this is the index that reaches an array, so
	// it is checked where the array is rather than trusted from another
	// folder away. The same goes for every index inside the loop below.
	if (import->geometries == NULL || node->mesh >= model->mesh_count)
		return true;

	mesh = &model->meshes[node->mesh];
	transform = decomposed(frame->world);

	for (uint32_t p = 0; p < mesh->primitive_count; p++) {
		uint32_t index = mesh->first_primitive + p;
		const voe_assets_primitive *primitive;
		voe_3d_material material;
		voe_ecs_entity entity;

		if (index >= model->primitive_count)
			return true;
		primitive = &model->primitives[index];

		// The bound is checked here as well as in the reader, because
		// this is the index that reaches an array and the number came
		// out of a file. VOE_ASSETS_MODEL_NONE is above any real count,
		// so a primitive that named no material takes the same branch:
		// glTF says an unmaterialled primitive is the default material,
		// and the default material is white and fully rough.
		if (import->materials == NULL ||
		    primitive->material >= model->material_count) {
			if (!default_material(import, &material, error))
				return false;
		} else {
			material = import->materials[primitive->material];
		}

		if (!voe_ecs_entity_create(import->world, &entity))
			return no_room(error, "another entity");
		if (!voe_scene_transform_add(import->world, entity, transform))
			return no_room(error, "another transform");
		// A file's contents are in the world. A layer is a thing a
		// program decides about something it has loaded, not a property
		// a `.glb` can carry — nothing in glTF says "above everything"
		// — so the importer names WORLD rather than leaving it to a
		// zeroed struct, and a caller that wants otherwise moves it
		// afterwards the way it moves anything else.
		if (!voe_3d_mesh_add(import->world, entity,
				     (voe_3d_mesh){
					     .geometry = import->geometries[index],
					     .layer = VOE_3D_LAYER_WORLD,
				     }))
			return no_room(error, "another mesh component");
		if (!voe_3d_material_add(import->world, entity, material))
			return no_room(error, "another material component");

		import->entities[import->entity_count++] = entity;
	}

	return true;
}

// How many entities the file will make, so that the array of them is one push.
static uint32_t count_entities(const voe_assets_model *model)
{
	uint32_t total = 0;

	for (uint32_t i = 0; i < model->node_count; i++) {
		if (model->nodes[i].mesh == VOE_ASSETS_MODEL_NONE)
			continue;
		total += model->meshes[model->nodes[i].mesh].primitive_count;
	}
	return total;
}

static bool walk(struct import *import, voe_base_error *error)
{
	const voe_assets_model *model = import->model;
	struct frame *stack;
	uint32_t open = 0;

	if (model->node_count == 0)
		return true;

	// As long as the file has nodes, which is enough: `assets` has checked
	// that no node is claimed as a child twice, so every node is pushed at
	// most once.
	stack = voe_base_arena_push(import->arena,
				    (size_t)model->node_count * sizeof(*stack));

	for (uint32_t i = 0; i < model->root_count; i++) {
		stack[open++] = (struct frame){
			.node = model->roots[i],
			.world = model->nodes[model->roots[i]].local,
			.depth = 1,
		};
	}

	while (open > 0) {
		struct frame frame = stack[--open];
		const voe_assets_node *node = &model->nodes[frame.node];

		if (frame.depth > VOE_3D_IMPORT_MAX_DEPTH) {
			VOE_BASE_ERROR("3d",
				       "a model whose nodes nest more than %u deep",
				       VOE_3D_IMPORT_MAX_DEPTH);
			if (error != NULL)
				*error = VOE_BASE_ERROR_UNSUPPORTED;
			return false;
		}

		if (!place_node(import, &frame, error))
			return false;

		for (uint32_t c = 0; c < node->child_count; c++) {
			uint32_t child = node->children[c];

			// The parent's matrix on the left: composition reads
			// right to left, so the child's own transform is
			// applied first and the parent's carries it into the
			// world.
			stack[open++] = (struct frame){
				.node = child,
				.world = voe_math_float4x4_mul(
					frame.world,
					model->nodes[child].local),
				.depth = frame.depth + 1,
			};
		}
	}

	return true;
}

bool voe_3d_import_glb(voe_ecs_world *world, voe_render_device *device,
		       voe_base_arena *arena, const uint8_t *bytes,
		       size_t size, voe_3d_import *out, voe_base_error *error)
{
	voe_assets_model model;
	struct import import = { 0 };
	uint32_t expected;

	VOE_BASE_ASSERT(world != NULL, "importing a model into no world");
	VOE_BASE_ASSERT(device != NULL, "importing a model with no device");
	VOE_BASE_ASSERT(arena != NULL, "importing a model without an arena");
	VOE_BASE_ASSERT(out != NULL, "importing a model into nothing");

	if (!voe_assets_model_read_glb(bytes, size, arena, &model, error))
		return false;

	import.world = world;
	import.device = device;
	import.arena = arena;
	import.model = &model;

	expected = count_entities(&model);
	// One push even when it is empty, so that `entities` is never a pointer
	// nobody may read: a model with nothing drawable in it is unusual and
	// legal.
	import.entities = voe_base_arena_push(
		arena, (size_t)(expected == 0 ? 1 : expected) *
			       sizeof(*import.entities));

	if (!upload_images(&import, error) ||
	    !upload_materials(&import, error) ||
	    !upload_geometry(&import, error) || !walk(&import, error))
		return false;

	*out = (voe_3d_import){
		.entities = import.entities,
		.entity_count = import.entity_count,
		.texture_count = import.texture_count,
		.material_count = model.material_count,
		.geometry_count = model.primitive_count,
	};
	return true;
}
