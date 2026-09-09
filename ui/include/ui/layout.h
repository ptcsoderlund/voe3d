// Layout: nested rows and columns of boxes, in millimetres, and a rectangle for
// every one of them. The whole of what this folder offers today.
//
//     voe_ui_context *ui = voe_ui_context_new(arena, (voe_ui_capacities){
//             .nodes = 256 });
//
//     voe_ui_frame_begin(ui, frame_arena);
//     voe_ui_node panel = voe_ui_column_begin(ui, (voe_ui_container){
//             .size = { .along = { VOE_UI_SIZE_FIXED, 60.0f },
//                       .across = { VOE_UI_SIZE_FIXED, 40.0f } },
//             .along = VOE_UI_ALONG_START,
//             .across = VOE_UI_ACROSS_FILL,
//             .gap = 2.0f, .pad = 4.0f });
//     voe_ui_node title = voe_ui_box(ui, (voe_math_float2){ 20.0f, 6.0f },
//                                    (voe_ui_sizing){ 0 });
//     voe_ui_end(ui);
//     if (!voe_ui_frame_end(ui))
//             ...                                  // the frame wanted more nodes
//
//     voe_ui_rect where = voe_ui_node_rect(ui, title);
//
// IT LAYS OUT AND IT DOES NOTHING ELSE. It does not draw, does not read input
// and does not know what a widget is: there is no button here, no state, no
// identity, no hit-testing and nothing emitted to a graphics card. A rectangle
// is the output and the caller decides what to do with it. Where this surface
// sits in the world is one matrix and it is not this folder's — nothing here
// knows about pixels, metres, cameras or layers.
//
// NOTHING IS LAID OUT UNTIL voe_ui_frame_end, AND THAT IS THE LOAD-BEARING
// DECISION. An immediate-mode call cannot know how big a row is until the row's
// children have been called, which is why immediate-mode windows are famous for
// jumping on their first frame. So the calls between frame_begin and frame_end
// build a tree and lay out nothing; frame_end measures it bottom-up and arranges
// it top-down, once, and every size is right on the first frame.
//
// WHICH IS WHY A CALL RETURNS A HANDLE AND NOT A SIZE. The size does not exist
// yet. A begin or a box call hands back a voe_ui_node — an index into this
// frame's tree — and the rectangle is read through it with voe_ui_node_rect
// after frame_end has returned.
//
// AND WHY NOTHING SURVIVES A FRAME. The tree lives in the arena handed to
// frame_begin and is gone when the caller rewinds it. Rebuilding it every frame
// is a few hundred bytes and a couple of sweeps; keeping it would be a cache
// with an invalidation problem, which is how an interface comes to show
// yesterday's arrangement. Handles from last frame name this frame's nodes, so
// do not keep one across a frame_begin.
//
// ---- THE SPACE ----
//
// MILLIMETRES, TWO DIMENSIONS, X RIGHT, Y DOWN, ORIGIN AT THE PANEL'S TOP-LEFT
// CORNER. That is voe_render_element's space exactly — see its header in
// render/include/render/device.h — and matching it is the whole point: a
// rectangle out of here IS an element's `bounds`, its min the xy and its size
// the zw, with no arithmetic in between.
//
// THE WORLD IS STILL +Y UP AND THIS IS NOT A DEPARTURE FROM IT. The engine's
// axes describe the world a panel stands in, in metres, the right way up; what
// is fixed here is the parameter space of one flat surface, which the world's
// handedness never described. There is still exactly one Y flip in this engine
// and it is in the viewport, and the element path's sign lives in
// voe_render_element_transform — nothing in this folder negates anything.
//
// A COLUMN LAYS ITS CHILDREN FROM THE TOP DOWN AND Y INCREASES AS IT GOES — the
// first child called has the SMALLEST Y — so a column reads in call order, which
// is what everyone expects. The rule the rest of it falls out of is this: START
// is left and top, END is right and bottom, on either axis and in a row and a
// column alike. So a row runs rightwards from the left edge and a column runs
// downwards from the top edge, and `across` in a row counts START as the top.
//
// ---- THE VOCABULARY ----
//
// `along` is the direction the container flows and `across` is the other one.
// A row flows along X and a column along Y, and the direction is in the call:
// there is no direction setting, and no borrowed layout vocabulary anywhere on
// this page. Along a row and across a row mean what they say, in a row and in a
// column alike, and that is the whole of the naming.
//
// A CHILD IS ONE OF THREE THINGS ALONG THE FLOW: natural, fixed or grow. One
// number, never three: there is no shrink and no basis. Across the flow it is
// natural or fixed, and the container's `across` decides where it sits or
// whether it is stretched.
//
// OVERFLOW IS NOT SHRUNK. Children that do not fit keep their true sizes and
// stick out past their container's rectangle, and the rectangles reported say
// so. Clipping belongs to the element record that draws them, not to layout,
// and shrinking would be a third number on every child.
#pragma once

#include <base/arena.h>
#include <math/float2.h>

#include <stdint.h>

// The layout context. Long-lived: it holds the capacities and the state of the
// frame being built, and nothing that outlives a frame.
typedef struct voe_ui_context voe_ui_context;

// What one frame may hold. Fixed at creation, because an arena does not
// reallocate and a bounded interface is the deliberate shape: the memory a
// program needs is a number a person chose.
//
// A node is a container or a box, so `nodes` is how many of both together one
// frame may build — count the begins and the boxes, not the ends. A panel is
// tens of them and a whole interface is hundreds; ask for the number the
// interface needs and no more, and see voe_ui_frame_end for what happens when a
// frame wants more than it was given. A node is under a hundred bytes, so the
// 256 above is some twenty-four kilobytes of the frame's arena — which is why
// the number is a capacity to be chosen once rather than something to economise
// on per panel.
typedef struct {
	uint32_t nodes;
} voe_ui_capacities;

// A rectangle in the panel's space: millimetres, X right, Y down, origin at the
// panel's top-left corner. So `min` is the rectangle's own top-left corner and
// min + size is its bottom-right — which is the shape voe_render_element.bounds
// is in, deliberately.
typedef struct {
	voe_math_float2 min;
	voe_math_float2 size;
} voe_ui_rect;

// How a node sizes itself on one axis.
//
// NATURAL is the default and it is what a zeroed voe_ui_sizing means: a box's
// natural size is the content it declared, and a container's is what its
// children came to.
typedef enum {
	// Measured. `value` is unused and must be nought.
	VOE_UI_SIZE_NATURAL = 0,
	// `value` millimetres, whatever the content came to.
	VOE_UI_SIZE_FIXED,
	// Share what is left over in the container, in proportion to `value`.
	// Along the flow only, and the weight must be greater than nought.
	VOE_UI_SIZE_GROW,
} voe_ui_size_kind;

typedef struct {
	voe_ui_size_kind kind;
	// Millimetres for FIXED, a weight for GROW, nought for NATURAL.
	float value;
} voe_ui_size;

// A node's size along and across its container's flow. For the root, which has
// no container, both are read against its own flow.
//
// GROW IS ALONG THE FLOW ONLY. Across it, a child is natural or fixed and the
// container's `across` is what stretches it; a grow across is the caller's bug
// and asserts.
typedef struct {
	voe_ui_size along;
	voe_ui_size across;
} voe_ui_sizing;

// How the run of children is placed along the flow, when there is space left
// over. START is left in a row and top in a column.
typedef enum {
	VOE_UI_ALONG_START = 0,
	VOE_UI_ALONG_CENTER,
	VOE_UI_ALONG_END,
	// The free space goes between the children, none at the ends. With one
	// child, or with children that already overflow, there is nothing to put
	// between anything and this is START.
	VOE_UI_ALONG_SPREAD,
} voe_ui_along;

// Where each child sits across the flow. Set on the container and applied to
// every child of it. START is top in a row and left in a column.
typedef enum {
	VOE_UI_ACROSS_START = 0,
	VOE_UI_ACROSS_CENTER,
	VOE_UI_ACROSS_END,
	// Stretch the child to the container's width across the flow. A child
	// that declared a fixed size across keeps it — see voe_ui_row_begin.
	VOE_UI_ACROSS_FILL,
} voe_ui_across;

// A container: how big it is inside its own container, and how it treats the
// children called between its begin and its voe_ui_end.
//
// `size` is this container's own size, the same declaration a box makes.
// `along`, `across`, `gap` and `pad` are about its children. So `size.along` is
// how long this container is and `along` is where its children sit in it, and
// the two words mean different things on purpose: the vocabulary names the
// axis, and what is being said about the axis is the field.
//
// `gap` is between children and `pad` is inside every edge, both in
// millimetres. One number each: there is no per-edge padding and no padding
// that differs by axis.
typedef struct {
	voe_ui_sizing size;
	voe_ui_along along;
	voe_ui_across across;
	float gap;
	float pad;
} voe_ui_container;

// A node in the tree being built: an index into it, valid until the next
// voe_ui_frame_begin. An index and not a pointer, so that nothing here hands
// out a pointer into an arena the caller is about to rewind.
typedef uint32_t voe_ui_node;

// What a call that could not fit another node hands back. The frame carries on
// and voe_ui_frame_end says it was refused; reading a rectangle through this is
// the caller's bug and asserts.
//
// WHICH IS WHY THE CALLS THAT RETURN A HANDLE ARE NOT [[nodiscard]] AND
// voe_ui_frame_end IS. Rule 13 asks for the attribute on everything that can
// fail, and the one thing that can fail here is the frame: a refused node is
// reported once, by frame_end, so the handle is not the failure channel and
// nothing is unchecked. Making every handle nodiscard would instead put a
// (void) in front of every spacer and every container whose rectangle the
// caller has no use for, which is most of them.
#define VOE_UI_NODE_NONE UINT32_MAX

// The context lives in the arena it is handed and there is no
// voe_ui_context_destroy: it is freed by rewinding or destroying that arena, and
// never one allocation at a time (rule 11). `nodes` must be greater than nought;
// nought is the caller's bug and asserts.
//
// Never NULL — the arena aborts rather than failing.
voe_ui_context *voe_ui_context_new(voe_base_arena *arena,
				   voe_ui_capacities capacities);

// DEVIATION: card 033's `frame_begin(context, frame_arena, root sizing)`. The
// root's sizing is given on the root's own begin call instead, and frame_begin
// takes the arena and nothing else. The card also decides that the direction is
// in the call and that the root is a container like any other, and a frame_begin
// carrying the root's size can honour neither: it would have to take a direction
// beside it, which is the direction setting ADR-0095 removed, or leave the
// `size` field of the first begin call meaning nothing at the top level. One
// call says how big the root is and it is the call that opens it.
//
// Starts a frame. `arena` is where this frame's tree goes: the node array is
// pushed once, here, because two arena pushes are not guaranteed to be next to
// each other and an array has to be one of them. Nothing the context keeps
// comes out of it, so the caller may rewind it as soon as the rectangles have
// been read.
//
// The first call after this must be voe_ui_row_begin or voe_ui_column_begin and
// that container is the root. There is one root per frame.
void voe_ui_frame_begin(voe_ui_context *ui, voe_base_arena *arena);

// Measures the tree and arranges it, and after this the rectangles are readable.
// The root sits with its `min` at the origin, at the size its own declaration
// came to: fixed is a panel of a known size whose children grow into it, natural
// is a panel that fits its children.
//
// FALSE WHEN THE FRAME WANTED MORE NODES THAN THE CONTEXT WAS CREATED WITH, and
// that is the one way this fails. It is a refusal and not a fatal error: the
// call that could not fit said so on stderr, naming both numbers, every
// rectangle in the refused frame is nought, and the next frame lays out
// normally. Every container begun must have been ended; an unbalanced frame is
// the caller's bug and asserts.
[[nodiscard]] bool voe_ui_frame_end(voe_ui_context *ui);

// Opens a container. Everything called until the matching voe_ui_end is a child
// of it, and children are placed in call order: left to right in a row, top to
// bottom in a column.
//
// A CHILD'S OWN FIXED SIZE ACROSS THE FLOW BEATS THE CONTAINER'S FILL, because
// the more specific statement should win — a caller who wrote a number meant it,
// and a FILL is a rule about everything in the container. Such a child sits at
// the START of the axis, since there is no stretching left to do with it.
//
// VOE_UI_NODE_NONE when the frame has no room for another node. It must still be
// matched by a voe_ui_end, so a caller need not check.
voe_ui_node voe_ui_row_begin(voe_ui_context *ui, voe_ui_container container);
voe_ui_node voe_ui_column_begin(voe_ui_context *ui, voe_ui_container container);

// Closes the container most recently begun.
void voe_ui_end(voe_ui_context *ui);

// A box: a leaf with nothing inside it. `content` is what it is worth in the
// panel's own axes — x rightwards, y upwards, millimetres — and it is what
// NATURAL reads on either axis. A box whose size is FIXED or GROW on an axis
// ignores `content` on that axis.
//
// CONTENT IS THE HOOK EVERYTHING MEASURED WILL COME THROUGH. A label's content
// is the size of the string, measured by whoever knows how; this folder does not
// measure text and has no text leaf. Nothing else about a box exists here — no
// colour, no identity, no behaviour.
//
// VOE_UI_NODE_NONE when the frame has no room for another node. A box may not be
// the root: the root is a container, and a box outside one is the caller's bug
// and asserts.
voe_ui_node voe_ui_box(voe_ui_context *ui, voe_math_float2 content,
		       voe_ui_sizing sizing);

// Where a node came to sit. Readable after voe_ui_frame_end has returned, until
// the next voe_ui_frame_begin or until the caller rewinds the frame's arena.
// Reading one before the frame has ended, or through VOE_UI_NODE_NONE, is the
// caller's bug and asserts.
voe_ui_rect voe_ui_node_rect(const voe_ui_context *ui, voe_ui_node node);
