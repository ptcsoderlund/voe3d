// A heights, paint, foliage or look stroke made with copies of its path
// and values, freed, and written back through the model store either way. See strokes.h
// for why it is its own memory beside the undo line's text.
#include "strokes.h"

#include <base/assert.h>

#include <stdlib.h>
#include <string.h>

// How many points `rect` covers; 0 for an empty one.
static size_t voe_editor_stroke_count(voe_3d_landscape_rect rect)
{
	if (rect.x1 <= rect.x0 || rect.z1 <= rect.z0)
		return 0;
	return (size_t)(rect.x1 - rect.x0) * (size_t)(rect.z1 - rect.z0);
}

// A malloced copy of `size` bytes; one byte of room for none, so the pointer
// is never NULL.
static void *voe_editor_stroke_copy(const void *values, size_t size)
{
	void *copy = malloc(size > 0 ? size : 1);

	VOE_BASE_ASSERT(copy != NULL, "out of memory copying a stroke's values");
	if (size > 0)
		memcpy(copy, values, size);
	return copy;
}

// A stroke with `path` copied and no values yet.
static voe_editor_stroke *voe_editor_stroke_made(const char *path,
						 voe_3d_landscape_rect rect)
{
	voe_editor_stroke *stroke;
	size_t length;

	VOE_BASE_ASSERT(path != NULL, "a stroke on no path");
	length = strlen(path);
	VOE_BASE_ASSERT(length < VOE_3D_MODEL_PATH, "a stroke's path too long");
	stroke = calloc(1, sizeof(*stroke));
	VOE_BASE_ASSERT(stroke != NULL, "out of memory making a stroke");
	memcpy(stroke->path, path, length + 1);
	stroke->rect = rect;
	return stroke;
}

voe_editor_stroke *voe_editor_stroke_new(const char *path,
					 voe_3d_landscape_rect rect,
					 const float *before, const float *after)
{
	size_t count = voe_editor_stroke_count(rect);
	voe_editor_stroke *stroke;

	VOE_BASE_ASSERT(count == 0 || (before != NULL && after != NULL),
			"a stroke with no heights");
	stroke = voe_editor_stroke_made(path, rect);
	stroke->before = voe_editor_stroke_copy(before, count * sizeof(float));
	stroke->after = voe_editor_stroke_copy(after, count * sizeof(float));
	return stroke;
}

voe_editor_stroke *voe_editor_stroke_new_paint(const char *path,
					       voe_3d_landscape_rect rect,
					       const uint8_t *before,
					       const uint8_t *after)
{
	size_t count = voe_editor_stroke_count(rect);
	voe_editor_stroke *stroke;

	VOE_BASE_ASSERT(count == 0 || (before != NULL && after != NULL),
			"a paint stroke with no weights");
	stroke = voe_editor_stroke_made(path, rect);
	stroke->paint = true;
	stroke->paint_before = voe_editor_stroke_copy(before, count * 8);
	stroke->paint_after = voe_editor_stroke_copy(after, count * 8);
	return stroke;
}

voe_editor_stroke *
voe_editor_stroke_new_foliage(const char *path, bool erase,
			      const voe_assets_landscape_copy *copies,
			      uint32_t count)
{
	voe_editor_stroke *stroke;

	VOE_BASE_ASSERT(count == 0 || copies != NULL,
			"a foliage stroke with no copies");
	stroke = voe_editor_stroke_made(path, (voe_3d_landscape_rect){ 0 });
	stroke->foliage = true;
	stroke->erase = erase;
	stroke->copies = voe_editor_stroke_copy(copies,
						(size_t)count * sizeof(*copies));
	stroke->copy_count = count;
	return stroke;
}

voe_editor_stroke *voe_editor_stroke_new_look(const char *path, uint32_t kind,
					      voe_assets_landscape_look before,
					      voe_assets_landscape_look after)
{
	voe_editor_stroke *stroke;

	VOE_BASE_ASSERT(kind < VOE_ASSETS_LANDSCAPE_KINDS,
			"a look stroke on no kind");
	stroke = voe_editor_stroke_made(path, (voe_3d_landscape_rect){ 0 });
	stroke->look = true;
	stroke->kind = kind;
	stroke->look_before = before;
	stroke->look_after = after;
	VOE_BASE_ASSERT(stroke->before == NULL && stroke->copies == NULL,
			"a look stroke holding values");
	return stroke;
}

void voe_editor_stroke_destroy(voe_editor_stroke *stroke)
{
	if (stroke == NULL)
		return;
	free(stroke->before);
	free(stroke->after);
	free(stroke->paint_before);
	free(stroke->paint_after);
	free(stroke->copies);
	free(stroke);
}

void voe_editor_stroke_apply(const voe_editor_stroke *stroke,
			     voe_editor_models *models, bool forward)
{
	VOE_BASE_ASSERT(stroke != NULL, "applying no stroke");
	VOE_BASE_ASSERT(models != NULL, "applying a stroke to no store");
	if (stroke->look) {
		voe_editor_models_look_set(models, stroke->path, stroke->kind,
					   forward ? stroke->look_after :
						     stroke->look_before);
		return;
	}
	if (stroke->foliage) {
		// A scatter's copies go in forward, an erase's come back out.
		voe_editor_models_put_copies(models, stroke->path, stroke->copies,
					     stroke->copy_count,
					     forward != stroke->erase);
		return;
	}
	if (voe_editor_stroke_count(stroke->rect) == 0)
		return;
	if (stroke->paint)
		voe_editor_models_put_paint(models, stroke->path, stroke->rect,
					    forward ? stroke->paint_after :
						      stroke->paint_before);
	else
		voe_editor_models_put(models, stroke->path, stroke->rect,
				      forward ? stroke->after : stroke->before);
}
