// A stroke made with copies of its path and heights, freed, and written back
// through the model store either way. See strokes.h for why it is its own
// memory beside the undo line's text.
#include "strokes.h"

#include <base/assert.h>

#include <stdlib.h>
#include <string.h>

// How many heights `rect` covers; 0 for an empty one.
static size_t voe_editor_stroke_count(voe_3d_landscape_rect rect)
{
	if (rect.x1 <= rect.x0 || rect.z1 <= rect.z0)
		return 0;
	return (size_t)(rect.x1 - rect.x0) * (size_t)(rect.z1 - rect.z0);
}

// A malloced copy of `count` heights; one float of room for none, so the
// pointer is never NULL.
static float *voe_editor_stroke_copy(const float *values, size_t count)
{
	float *copy = malloc((count > 0 ? count : 1) * sizeof(*copy));

	VOE_BASE_ASSERT(copy != NULL, "out of memory copying a stroke's heights");
	if (count > 0)
		memcpy(copy, values, count * sizeof(*copy));
	return copy;
}

voe_editor_stroke *voe_editor_stroke_new(const char *path,
					 voe_3d_landscape_rect rect,
					 const float *before, const float *after)
{
	size_t count = voe_editor_stroke_count(rect);
	size_t length;
	voe_editor_stroke *stroke;

	VOE_BASE_ASSERT(path != NULL, "a stroke on no path");
	VOE_BASE_ASSERT(count == 0 || (before != NULL && after != NULL),
			"a stroke with no heights");
	length = strlen(path);
	VOE_BASE_ASSERT(length < VOE_3D_MODEL_PATH, "a stroke's path too long");

	stroke = malloc(sizeof(*stroke));
	VOE_BASE_ASSERT(stroke != NULL, "out of memory making a stroke");
	memcpy(stroke->path, path, length + 1);
	stroke->rect = rect;
	stroke->before = voe_editor_stroke_copy(before, count);
	stroke->after = voe_editor_stroke_copy(after, count);
	return stroke;
}

void voe_editor_stroke_destroy(voe_editor_stroke *stroke)
{
	if (stroke == NULL)
		return;
	free(stroke->before);
	free(stroke->after);
	free(stroke);
}

void voe_editor_stroke_apply(const voe_editor_stroke *stroke,
			     voe_editor_models *models, bool forward)
{
	VOE_BASE_ASSERT(stroke != NULL, "applying no stroke");
	VOE_BASE_ASSERT(models != NULL, "applying a stroke to no store");
	if (voe_editor_stroke_count(stroke->rect) == 0)
		return;
	voe_editor_models_put(models, stroke->path, stroke->rect,
			      forward ? stroke->after : stroke->before);
}
