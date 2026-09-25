// The projection matrix, and the only place in the engine where a field of view
// becomes clip space; and the render view, where a pose and a lens become what
// is drawn with.
//
// THE CAMERA IS THE LENS (0222). The camera component holds a field of view and
// two planes; where it is and how it faces is its entity's transform.
//
// voe_3d_view IS THE ONE PLACE A POSE AND A LENS BECOME A voe_render_view (0223):
// the value `render`'s pass, the outline, the gizmo and the pick are all handed.
// The world's camera and an editor view's orbit both come through it, so they
// cannot disagree about what a pose looks like.
//
// EVERY RENDER VIEW IS EYE-RELATIVE (0250): its view matrix has no translation
// and its `eye` is zero, and the eye's world position travels beside it as a
// voe_math_double3, which every object's matrix is taken about.
//
// IT IS HERE AND NOT IN math OR IN scene, AND BOTH HALVES OF THAT ARE RULES.
// `math` may not know about a graphics API (CLAUDE.md, Conventions), and `scene`
// holds the numbers a person would author — a field of view in radians and two
// distances in metres — without knowing what a clip volume is. This folder is
// the one allowed to know both, so this is where the two meet.
//
// DEPTH RUNS BACKWARDS AND THIS FUNCTION IS WHERE IT IS WRITTEN DOWN AS
// ARITHMETIC. The near plane comes out at 1.0 and the far plane at 0.0, which is
// the opposite of every tutorial. The reason is precision: a float depth buffer
// has most of its resolution near zero, and putting the far plane there is what
// stops distant geometry from fighting over the same handful of values. The
// other two halves of the convention are elsewhere and all three have to agree —
// the buffer is cleared to 0 and the comparison is GREATER, both in `render`.
//
// THERE IS NO Y NEGATION IN IT, DELIBERATELY. Vulkan's clip space is Y-down and
// this engine is Y-up, and the whole of the reconciliation is a negative
// viewport height inside `render`. Negating a row here as well would flip twice,
// which looks exactly like not flipping at all until something is culled.
#pragma once

#include <math/float4x4.h>
#include <render/device.h>
#include <scene/camera_component.h>
#include <scene/transform_component.h>

// aspect is width over height of the target being drawn into, which is the
// target's business and not the camera's — the same camera in a wider window
// sees more, it does not see the same thing stretched.
//
// A zero or negative aspect, or a camera whose planes are the wrong way round,
// is the caller's bug and asserts: none of it can come out of a file.
voe_math_float4x4 voe_3d_projection(voe_scene_camera lens, float aspect);

// The view of a camera at `pose` with `lens`: the view matrix from
// voe_scene_camera_view(pose), the projection from voe_3d_projection(lens,
// aspect), the eye zero because the view is about the pose's own position, and
// `reserved` nought. False, with `out` untouched, when the pose sees nothing
// (its matrix has no inverse).
[[nodiscard]] bool voe_3d_view(voe_scene_transform pose, voe_scene_camera lens,
			       float aspect, voe_render_view *out);
