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
//             .gap = 2.0f, .pad = { 4.0f, 4.0f, 4.0f, 4.0f } });
//     voe_ui_node title = voe_ui_box(ui, (voe_math_float2){ 20.0f, 6.0f },
//                                    (voe_ui_sizing){ 0 });
//     voe_ui_end(ui);
//     if (!voe_ui_frame_end(ui))
//             ...                                  // the frame wanted more nodes
//
//     voe_ui_rect where = voe_ui_node_rect(ui, title);
//     voe_math_float2 wanted = voe_ui_node_measured(ui, panel);
//
// THIS HEADER LAYS OUT AND IT DOES NOTHING ELSE. It does not draw, does not read
// input and does not know what a widget is: there is no button here, no state,
// no identity, no hit-testing and nothing emitted to a graphics card. A
// rectangle is the output and the caller decides what to do with it. Where this
// surface sits in the world is one matrix and it is not this folder's — nothing
// here knows about pixels, metres, cameras or layers.
//
// A BUTTON IS ui/widgets.h AND IT IS BUILT ON EXACTLY WHAT IS ON THIS PAGE. It
// takes the same context and the same frame, its widgets ARE rows and columns
// and boxes, and a widget's rectangle is one of these. So the two headers are
// not two systems: this is the arrangement and that is what the arrangement is
// of. Every rule below binds a widget the same way it binds a box.
//
// NOTHING IS LAID OUT UNTIL voe_ui_frame_end, AND THAT IS THE LOAD-BEARING
// DECISION. An immediate-mode call cannot know how big a row is until the row's
// children have been called, which is why immediate-mode windows are famous for
// jumping on their first frame. So the calls between frame_begin and frame_end
// build a tree and lay out nothing; frame_end measures it bottom-up and arranges
// it top-down, and every size is right on the first frame.
//
// IT DOES THAT ONE AXIS AT A TIME: X FOR THE WHOLE TREE, THEN Y FOR THE WHOLE
// TREE. Every width is settled before any height is worked out, which is what
// lets a row that wraps be as tall as its lines — it knows its width by the time
// its height is asked. The rule that falls out, and binds everything added here
// later: NOTHING MAY NEED A HEIGHT TO KNOW A WIDTH.
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
// and shrinking would be a third number on every child. A container may wrap
// instead, and nothing that does not ask to — see `wrap` below.
//
// ---- A RUN THAT WRAPS ----
//
// A CONTAINER WITH `wrap` SET PUTS CHILDREN THAT DO NOT FIT ONTO FURTHER LINES,
// along its flow: a row's lines stack downwards and a column's rightwards. It is
// opt-in and nothing else changes: a container that does not ask lays out as it
// always did, and one that asks and fits is exactly that same layout.
//
// A LINE BREAKS BEFORE THE CHILD THAT WOULD TAKE IT PAST THE INNER LENGTH,
// counting each child's natural length and a gap before every child but a
// line's first. THE FIRST CHILD ON A LINE NEVER BREAKS, so a child longer than
// the whole line has that line to itself, at its full size, sticking out.
//
// EACH LINE IS A RUN OF ITS OWN ALONG THE FLOW: grow children share what THAT
// line leaves, and `along` distributes within that line. Across the flow a line
// is as thick as its thickest child, lines stack from the start with `gap`
// between them, and space left over across the container is shared equally
// between the lines, each growing by the same amount; `across` then places each
// child within its line, FILL stretching to the line. One line is the inner size
// across, which is today's answer.
//
// A NATURAL LENGTH ALONG THE FLOW IS ONE LINE LONG, so such a container never
// wraps, and that is not an error. Wrapping needs a length from outside: fixed,
// grow, or stretched by a parent's FILL.
//
// A WRAPPING ROW IS AS TALL AS ITS LINES, which is why X runs first. A WRAPPING
// COLUMN DOES NOT WIDEN AS IT WRAPS: its width was settled as one line before its
// height was known, so the extra lines it breaks into sit to the right of its
// rectangle, outside it, and move its children without resizing them — a FILL
// child keeps the one-line width it was given. Give such a column the width its
// lines need, or put it where overflowing to the right is wanted.
//
// Anchored children take no part in lines, exactly as they take no part in a
// run.
//
// ---- SPACE INSIDE AN EDGE, AND THE ONE THIS FOLDER REFUSES ----
//
// PADDING IS FOUR NUMBERS AND THEY ARE NAMED BY ABSOLUTE SIDE: left, top, right,
// bottom, millimetres, inside every edge of a container. They are deliberately
// NOT named by the flow — there is no `along_start` on this page — because
// flow-relative padding changes which edge it means the day a row becomes a
// column, and putting the direction in the call rather than in a setting was the
// whole point of the vocabulary above. `pad.top` is the top in a row and in a
// column alike, and it is the top of the space this header has already fixed:
// Y down from the panel's top-left corner.
//
// AND THERE IS NO MARGIN. A child carries no outer spacing of its own, on any
// axis, and there will not be one. This is a refusal and not an omission: with
// both, two sources of space meet between every pair of children and the system
// has to say whether they add or collapse — CSS collapses them, and it is the
// most-complained-about rule in layout. With padding only there is exactly one
// source of space between two children, which is the container's `gap`, and one
// inside an edge, which is its `pad`, and neither interacts with anything.
//
// SO DO THIS INSTEAD, because somebody will want one: a single child that needs
// space of its own is wrapped in a container with padding, and that container IS
// that child's margin. An unusual gap between one pair of children is a
// fixed-size box put between them as a spacer.
//
// ---- A CHILD THAT LEAVES THE FLOW ----
//
// AN ANCHORED CHILD IS NOT IN THE ROW OR THE COLUMN AT ALL, and that — not a new
// kind of alignment — is the whole of what anchoring is. Its parent's run
// neither reserves space for it nor counts it in its own size; it is pinned to
// its parent's edges by its own two anchors instead. What it buys is a child
// OVER its siblings — a floating panel, a badge, a corner inspector — which is
// the one thing the flow cannot express, since snapping to both sides is already
// `across: FILL` and right-alignment is already a grow spacer.
//
// IT IS SPELLED AS A FIELD ON THE CONTAINER — voe_ui_container.anchor — and a
// zeroed one is an ordinary child in the flow, so nothing that does not ask for
// anchoring pays anything for it and no second set of begin calls exists. A
// panel is anchored by the very call that opens it.
//
// ITS TWO AXES ARE X AND Y AND NOT `along` AND `across`, because a child that is
// out of the flow has no flow for those two words to be relative to — and for
// the same reason the padding above is absolute. An anchor that changed which
// edge it meant when its parent turned from a row into a column would be that
// defect a second time. `anchor.x` is the horizontal one in a row and in a
// column alike.
//
// ITS ALIGNMENT IS voe_ui_across: THE SAME FOUR VALUES, MEANING THE SAME FOUR
// THINGS. START is the near edge — left on X, top on Y — END is the far one,
// CENTER is the middle, and FILL is both edges at once. Four values on each of
// two axes is sixteen combinations, and that is the point: between them they
// hold all nine of the corner, edge-middle and centre positions, every stretch
// case, and the margins. The enum keeps the name it already has rather than
// gaining a twin with the same four values in it, because one idea spelled two
// ways is the thing worth avoiding.
//
// THE OFFSET IS ONE NUMBER PER AXIS AND A POSITIVE ONE ALWAYS MOVES THE CHILD
// INWARD. At START it is the gap from the near edge, at END the gap from the far
// edge, at CENTER a displacement in the positive direction — rightwards or
// downwards — and at FILL an inset taken off BOTH edges, so the derived size is
// the parent's content box less twice it. A negative offset therefore moves a
// child outward, and nothing stops one landing wholly outside its parent: this
// folder reports true rectangles and clipping belongs to the element record, so
// a mistyped offset draws a panel somewhere surprising rather than being
// quietly corrected.
//
// A CHILD'S OWN FIXED SIZE BEATS FILL HERE TOO, AND IT IS THE SAME RULE, not a
// second answer to one question: the more specific statement wins, the child
// keeps the size it named, and it sits at the START of that axis — exactly what
// VOE_UI_ACROSS_FILL already promises a child in a row.
//
// AN ANCHORED CHILD'S `size` IS READ ABSOLUTELY, to match its anchors:
// `size.along` is its size on X and `size.across` is its size on Y, in the same
// order as anchor.x and anchor.y and as the two components of a float2. In the
// flow those two fields are relative to the parent's direction; out of it there
// is no direction, so they are not.
//
// GROW IS MEANINGLESS FOR AN ANCHORED CHILD. There is no run and no leftover to
// share, so asking for it is the caller's bug and asserts.
//
// IT CONTRIBUTES NOTHING TO ITS PARENT'S NATURAL SIZE, exactly as a grow child
// contributes nothing along the flow. AND HERE IS THE TRAP THAT FOLLOWS, said
// once so that it is a surprise once rather than a bug forever: a
// fit-to-children container holding ONLY anchored children has no natural size
// at all, and comes out at nothing but its own padding. That is the same trap
// CSS has, and it is the honest consequence of a floating badge not being
// allowed to inflate the thing it floats over.
//
// ANCHORS ARE MEASURED AGAINST THE PARENT'S CONTENT BOX, INSIDE ITS PADDING. It
// is the classic ambiguity in every system that has anchors and both answers are
// defensible; this one is decided, so a child anchored to END on X in a
// container with a right padding of 6 stops 6 short of that container's edge.
//
// ANCHORED CHILDREN ARE ARRANGED AFTER THEIR IN-FLOW SIBLINGS, AND AMONG
// THEMSELVES IN CALL ORDER. Submission order is paint order on the element path,
// so an anchored panel PAINTS OVER the siblings it floats above, which is what
// a floating panel is for. A parent still comes before every one of its
// children, so a panel's background is still behind its own contents.
//
// AN ANCHORED CHILD IS A CONTAINER LIKE ANY OTHER and holds rows, columns and
// further anchored children, to any depth.
//
// ---- WHAT IT WANTED, BESIDE WHERE IT WENT ----
//
// voe_ui_node_rect says where a node came to sit. voe_ui_node_measured says what
// the measure pass computed its content to be, and THE WHOLE POINT IS THE
// COMPARISON BETWEEN THE TWO: a container whose measured size along the flow
// exceeds its arranged size holds content that did not fit, and that subtraction
// is what a scroll area acts on. There is no overflowed() predicate here,
// because it would have to choose a tolerance nobody has asked for yet.
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
	// And how many element records one frame may emit — see widgets.h,
	// which is what emits them. A panel or a button is one, a label is one
	// per character that draws and none for a space, so a screenful of
	// interface with writing on it is hundreds. A caller that only wants
	// rectangles asks for none and pays for none.
	uint32_t elements;
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
	// The free space goes between the children and none at the ends.
	VOE_UI_ALONG_SPREAD,
	// An equal share of the free space in every gap, THE ONES AT THE ENDS
	// INCLUDED — so three children make four equal gaps where SPREAD makes
	// two. Not the flexbox `around`, whose end gaps are half its inner ones,
	// which is the value people reliably get wrong.
	//
	// SPREAD AND EVENLY DEGENERATE THE SAME WAY AND IT IS THE SAME SENTENCE:
	// with one child, or with children that already overflow, there is no
	// free space to distribute and both are START.
	VOE_UI_ALONG_EVENLY,
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

// THESE FOUR ALSO NAME AN ANCHORED CHILD'S ALIGNMENT ON EACH OF ITS TWO AXES,
// meaning the same four things there: START the near edge, END the far one,
// CENTER the middle, FILL both edges at once. One enum and not two, because two
// spellings of one idea is what a reader has to learn twice.
//
// `offset` is millimetres and a positive one always moves the child INWARD: the
// gap from the near edge at START, the gap from the far edge at END, a
// displacement rightwards or downwards at CENTER, and an inset off BOTH edges at
// FILL. A negative one moves it outward, and out of its parent if it is large
// enough, which is allowed.
typedef struct {
	voe_ui_across align;
	float offset;
} voe_ui_anchor_axis;

// A child pinned to its parent's content box instead of laid out in its run.
//
// `anchored` IS THE WHOLE SWITCH, and a zeroed voe_ui_anchor is a child in the
// flow — so a container that says nothing about anchoring is an ordinary one.
// The axes are X and Y and not `along` and `across`, because a child out of the
// flow has no flow to be relative to; see the top of this header.
typedef struct {
	bool anchored;
	voe_ui_anchor_axis x;
	voe_ui_anchor_axis y;
} voe_ui_anchor;

// Space inside a container's four edges, in millimetres.
//
// NAMED BY ABSOLUTE SIDE AND NEVER BY THE FLOW, so `top` is the top in a row and
// in a column alike; the reason is at the top of this header, and so is the
// reason there is no margin to go with it. The order is left, top, right,
// bottom — the two X sides then the two Y sides, near edge before far — so that
// a positional initialiser reads in the same order as a rectangle's min and max.
typedef struct {
	float left;
	float top;
	float right;
	float bottom;
} voe_ui_pad;

// A container: how big it is inside its own container, and how it treats the
// children called between its begin and its voe_ui_end.
//
// `size` is this container's own size, the same declaration a box makes.
// `along`, `across`, `gap` and `pad` are about its children. So `size.along` is
// how long this container is and `along` is where its children sit in it, and
// the two words mean different things on purpose: the vocabulary names the
// axis, and what is being said about the axis is the field.
//
// `gap` is one number between children; `pad` is four, one inside each edge.
// Both are millimetres, and there is no margin — see the top of this header for
// why that is a refusal rather than a gap in the model.
//
// `anchor` takes this container OUT of its parent's run and pins it to its
// parent's content box instead. Left zeroed — which is what a container that
// never mentions it is — it is an ordinary child in the flow, and every field
// above keeps its ordinary meaning. Anchored, `along`, `across`, `gap` and `pad`
// still describe THIS container's own children exactly as before; it is only
// where this container itself sits that changes.
typedef struct {
	voe_ui_sizing size;
	voe_ui_along along;
	voe_ui_across across;
	float gap;
	voe_ui_pad pad;
	voe_ui_anchor anchor;
	// Nought is one line, as a run always was. See A RUN THAT WRAPS at the
	// top of this header.
	bool wrap;
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
// FALSE WHEN THE FRAME WANTED MORE THAN THE CONTEXT WAS CREATED WITH, and there
// are three ways to want that: more nodes, more element records, or the same
// widget key twice (see widgets.h). All three are refusals and not fatal errors:
// whichever call could not be honoured said so on stderr, naming what it was,
// every rectangle in a refused frame is nought, and the next frame lays out
// normally. Every container begun must have been ended; an unbalanced frame is
// the caller's bug and asserts.
//
// THE WIDGET PASS IS INSIDE THIS CALL, after the arrangement and before this
// returns: the pointer is tested against the rectangles arrange has just worked
// out, and the element records are built from them. That is the only order in
// which a click is tested against the arrangement the person was looking at,
// which is why it is not a separate call a caller could make at the wrong
// moment.
[[nodiscard]] bool voe_ui_frame_end(voe_ui_context *ui);

// Opens a container. Everything called until the matching voe_ui_end is a child
// of it, and children are placed in call order: left to right in a row, top to
// bottom in a column.
//
// A CHILD'S OWN FIXED SIZE ACROSS THE FLOW BEATS THE CONTAINER'S FILL, because
// the more specific statement should win — a caller who wrote a number meant it,
// and a FILL is a rule about everything in the container. Such a child sits at
// the START of the axis, since there is no stretching left to do with it. An
// anchored child answers that same question the same way, on either of its axes.
//
// A CONTAINER WHOSE `anchor.anchored` IS SET LEAVES ITS PARENT'S RUN. It is
// arranged against its parent's content box after every in-flow sibling and in
// call order among the other anchored ones, so it paints over them; it adds
// nothing to its parent's natural size; its own `size` is read absolutely, along
// as X and across as Y; and a GROW on either axis is the caller's bug and
// asserts, there being no run to take a share of. The root may not be anchored,
// having nothing to be anchored to, and that asserts too.
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
// A BOX IS ALWAYS IN THE FLOW. Anchoring lives on voe_ui_container, so a leaf
// that wants to float is wrapped in an anchored row or column holding it — which
// is one node, and it keeps one way of spelling an anchor rather than two.
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

// What the measure pass computed this node's own content to be, in millimetres
// on each axis: for a box the content it declared, and for a container its
// children, its gaps and its padding.
//
// FOR A CONTAINER THAT WRAPS IT IS THE CONTENT AFTER WRAPPING: its longest line
// along the flow, and every line and the gaps between them across it, plus
// padding. So a wrapping column's measured width is every column it broke into,
// wider than its rectangle, which is the one place that shows.
//
// IT DELIBERATELY IGNORES THIS NODE'S OWN FIXED OR GROW DECLARATION, because
// that declaration is what voe_ui_node_rect already reports and the entire value
// of this accessor is the difference between the two. So a GROW child's measured
// size is what its content wanted rather than the share it was given, and a
// FIXED container's is what its children came to rather than the size it was
// told to be — which is exactly the case that says something overflowed.
//
// THE COMPARISON IS THE POINT AND THE CALLER MAKES IT: measured along the flow
// greater than arranged along the flow means the content did not fit. There is
// no predicate here to do the subtraction, because one would have to pick a
// tolerance nobody has asked for.
//
// Readable in the same window as voe_ui_node_rect and refused in the same three
// ways: before the frame has ended, through VOE_UI_NODE_NONE, or through a node
// this frame never made, all of them the caller's bug.
voe_math_float2 voe_ui_node_measured(const voe_ui_context *ui,
				     voe_ui_node node);
