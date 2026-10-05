// The scroll area: the table of offsets that outlives a frame, the bars over
// its content, and every way one is moved. See include/ui/widgets.h for the
// promises; widgets.c makes the keys, the frame and the records this hangs off,
// button.c holds the press this answers for a thumb, and context.h says what
// each of these files offers the others.
//
// THE TABLE IS NEVER EDITED IN PLACE, IT IS REWRITTEN. During a frame each
// scroll_begin looks its key up in last frame's table and hands layout what it
// finds; at frame_end the table is written again from this frame's areas, in call
// order, with the offset layout used. An area not called is not written back, and
// that is the whole of forgetting — no sweep, no age, and no slot a stale key can
// hold while a new one wants it. The lookup is a linear walk: a frame holds a
// handful of areas, and a hash would be a second collision rule for nothing.
//
// DEVIATION: spec 001 task 3 says the table stores the offset layout used; a
// refused frame has no layout. The narrowest reading: an area called in a refused
// frame keeps the offset it was handed, and one not called is still dropped.
//
// A MOVE IS APPLIED TO THE TABLE AFTER IT IS WRITTEN, so it lands in the next
// frame's layout: a thumb drag, then a track press, then the pointer's scroll.
// The pointer's goes through scroll_by, which starts at an area and passes what
// that area cannot take to the next area outward — the one function a program's
// voe_ui_scroll_by will expose when focus gives it a caller. An area's outer
// neighbour is found by the tree's shape and not by a stored parent: areas are
// listed in call order, so the nearest earlier one whose subtree holds this one
// is the one around it. A program's voe_ui_scroll_centre, called after
// frame_end, is the last way: it sets one area's offset and passes nothing on.
//
// THE BAR PAINTS STRAIGHT AFTER THE LAST NODE OF ITS AREA'S SUBTREE, which is
// where "after the area's children" is in paint order: a subtree fills the
// consecutive paint positions from its root's, so its last is the root's position
// plus its size less one. Emission and the hit test both walk paint order and ask
// bar_after at every position, so the bar is in front of the content for the
// pointer exactly as far as it is in front for the eye, and a panel anchored over
// the area is in front of both. Where nested areas end at one position the inner
// bar comes first and the outer paints over it.
//
// A THUMB DRAG MOVES THE OFFSET BY THE POINTER'S TRAVEL TIMES MEASURED OVER
// ARRANGED, measured from the press and not from last frame, so a thumb dragged
// past the end and back comes back under the pointer instead of lagging behind a
// clamp it hit on the way.
#include "context.h"

#include <base/assert.h>
#include <base/report.h>
#include <math/float2.h>
#include <math/float4.h>

// A scroll area's bar, in millimetres. The thumb is never shorter than the
// minimum, so a very long list still has something to take hold of; where the
// track itself is shorter, the thumb is the track.
#define SCROLL_THICKNESS 1.5f
#define SCROLL_THUMB_MIN 5.0f

// No scroll area, where an index into this frame's areas is expected.
#define NO_AREA UINT32_MAX

// One axis of a pair, `y` saying which, as layout.c reads them. Only the scroll
// area's arithmetic is the same on both axes, so only it uses these.
static float component(voe_math_float2 v, bool y)
{
	return y ? v.y : v.x;
}

static void component_set(voe_math_float2 *v, bool y, float value)
{
	if (y)
		v->y = value;
	else
		v->x = value;
}

static bool scrolls_on(voe_ui_scroll_axes axes, bool y)
{
	return y ? axes.y : axes.x;
}

// What is left of `rect` inside `within`, size nought on an axis with nothing.
static voe_ui_rect intersect(voe_ui_rect rect, voe_ui_rect within)
{
	voe_ui_rect out = { 0 };

	for (int axis = 0; axis < 2; axis++) {
		bool y = axis == 1;
		float low = component(rect.min, y);
		float high = low + component(rect.size, y);
		float within_low = component(within.min, y);
		float within_high = within_low + component(within.size, y);

		if (low < within_low)
			low = within_low;
		if (high > within_high)
			high = within_high;
		component_set(&out.min, y, low);
		component_set(&out.size, y, high > low ? high - low : 0.0f);
	}
	return out;
}

// Last frame's offset for this key, nought on an axis the area does not scroll
// and on both for a key last frame did not have.
static voe_math_float2 remembered(const voe_ui_context *ui, uint64_t key,
				  voe_ui_scroll_axes axes)
{
	for (uint32_t i = 0; i < ui->scroll_remembered; i++) {
		voe_math_float2 offset = ui->scroll_memory[i].offset;

		if (ui->scroll_memory[i].key != key)
			continue;
		return (voe_math_float2){ axes.x ? offset.x : 0.0f,
					  axes.y ? offset.y : 0.0f };
	}
	return (voe_math_float2){ 0.0f, 0.0f };
}

voe_ui_node voe_ui_scroll_begin(voe_ui_context *ui, const char *name,
				uint32_t index, voe_ui_container container,
				voe_ui_scroll_axes axes)
{
	voe_math_float2 handed;
	voe_ui_node node;
	uint64_t key;
	const voe_ui_theme *theme = voe_ui_theme_current(ui);

	VOE_BASE_ASSERT(ui != NULL, "opening a scroll area on no context");
	VOE_BASE_ASSERT(name != NULL,
			"a scroll area with no name has no identity");
	VOE_BASE_ASSERT(theme != NULL,
			"a scroll area needs a theme, its bar may be drawn "
			"later in this very frame: see "
			"voe_ui_theme_set/voe_ui_theme_push");
	VOE_BASE_ASSERT(container.overflow.x == VOE_UI_OVERFLOW_VISIBLE &&
				container.overflow.y == VOE_UI_OVERFLOW_VISIBLE,
			"a scroll area's overflow is its own; it clips on both "
			"axes, so leave `overflow` zeroed");
	VOE_BASE_ASSERT(container.scroll.x == 0.0f && container.scroll.y == 0.0f,
			"a scroll area's offset is its own and remembered under "
			"its key, so leave `scroll` zeroed");

	key = voe_ui_widget_claim(ui, name, index);
	handed = remembered(ui, key, axes);

	container.overflow = (voe_ui_overflow){ VOE_UI_OVERFLOW_CLIP,
						VOE_UI_OVERFLOW_CLIP };
	container.scroll = handed;
	node = voe_ui_column_begin(ui, container);

	// Refused as a node past capacity is: named once, the frame carries on
	// balanced, and frame_end says no. The column is still open, so the
	// caller's voe_ui_end still matches it.
	if (ui->scroll_count == ui->capacities.scrolls) {
		if (!ui->scroll_overrun)
			VOE_BASE_ERROR("ui",
				       "scroll area %u refused, this context was "
				       "created with room for %u",
				       ui->scroll_count + 1,
				       ui->capacities.scrolls);
		ui->scroll_overrun = true;
	} else {
		ui->scroll_areas[ui->scroll_count++] =
			(struct voe_ui_scroll_area){ .key = key,
						     .node = node,
						     .axes = axes,
						     .handed = handed };
	}

	if (node != VOE_UI_NODE_NONE) {
		ui->widgets[node].kind = VOE_UI_WIDGET_SCROLL;
		ui->widgets[node].key = key;
		ui->widgets[node].keyed = true;
		ui->widgets[node].theme = theme;
	}

	return node;
}

// The table, out of the context's own arena because it outlives every frame.
// Called from widgets.c's voe_ui_widgets_init and from nowhere else.
void voe_ui_scrolls_init(voe_ui_context *ui, voe_base_arena *arena)
{
	// A context that asked for no scroll areas remembers none, and an arena
	// push of nothing has no caller.
	ui->scroll_memory = NULL;
	if (ui->capacities.scrolls > 0)
		ui->scroll_memory = voe_base_arena_push(
			arena, (size_t)ui->capacities.scrolls *
				       sizeof(*ui->scroll_memory));
	ui->scroll_remembered = 0;
}

// How far an area's content reaches past its rectangle on one axis, and nought
// where it does not. The range layout clamped the offset to.
static float scroll_range(const voe_ui_context *ui, uint32_t node, bool y)
{
	const struct voe_ui_node_record *n = &ui->nodes[node];
	float range =
		component(n->content_natural, y) - component(n->rect.size, y);

	return range > 0.0f ? range : 0.0f;
}

static float clamp_offset(float offset, float range)
{
	if (offset > range)
		offset = range;
	if (offset < 0.0f)
		offset = 0.0f;
	return offset;
}

static bool bar_shows(const voe_ui_context *ui,
		      const struct voe_ui_scroll_area *area, bool y)
{
	return scrolls_on(area->axes, y) && scroll_range(ui, area->node, y) > 0.0f;
}

// Where one of an area's bars is this frame, unclipped: `y` is the bar that
// scrolls Y, along the right edge, and otherwise the one along the bottom.
struct voe_ui_scrollbar {
	bool shows;
	voe_ui_rect track;
	voe_ui_rect thumb;
};

static struct voe_ui_scrollbar scrollbar(const voe_ui_context *ui,
					 const struct voe_ui_scroll_area *area,
					 bool y)
{
	const struct voe_ui_node_record *n = &ui->nodes[area->node];
	struct voe_ui_scrollbar bar = { 0 };
	float measured = component(n->content_natural, y);
	float arranged = component(n->rect.size, y);
	float track;
	float thumb;

	if (!bar_shows(ui, area, y))
		return bar;

	// Short of the corner by the other bar's thickness when both show, so
	// the two never overlap and neither is under the other for the pointer.
	track = arranged - (bar_shows(ui, area, !y) ? SCROLL_THICKNESS : 0.0f);
	if (track < 0.0f)
		track = 0.0f;
	// Measured is greater than arranged here, and so greater than nought.
	thumb = track * arranged / measured;
	if (thumb < SCROLL_THUMB_MIN)
		thumb = SCROLL_THUMB_MIN;
	if (thumb > track)
		thumb = track;

	bar.shows = true;
	component_set(&bar.track.min, y, component(n->rect.min, y));
	component_set(&bar.track.size, y, track);
	component_set(&bar.track.min, !y,
		      component(n->rect.min, !y) + component(n->rect.size, !y) -
			      SCROLL_THICKNESS);
	component_set(&bar.track.size, !y, SCROLL_THICKNESS);

	// Proportional to the offset over the range, so the thumb's far end
	// meets the track's at the content's end whatever the minimum did.
	bar.thumb = bar.track;
	component_set(&bar.thumb.min, y,
		      component(n->rect.min, y) +
			      (track - thumb) * component(n->scrolled, y) /
				      scroll_range(ui, area->node, y));
	component_set(&bar.thumb.size, y, thumb);

	return bar;
}

// The next scroll area below index `from` whose bar paints straight after paint
// position `at`, or NO_AREA. Counting down puts an inner area before the outer
// one when both end at the same position. See this file's header.
static uint32_t bar_after(const voe_ui_context *ui, uint32_t at, uint32_t from)
{
	while (from-- > 0) {
		const struct voe_ui_node_record *n =
			&ui->nodes[ui->scroll_areas[from].node];

		if (n->paint + n->subtree - 1 == at)
			return from;
	}
	return NO_AREA;
}

// One of an area's bars against the pointer, tested against what of it is seen.
static void hit_bar(const voe_ui_context *ui, uint32_t area, bool y,
		    struct voe_ui_hit *hit)
{
	const struct voe_ui_scroll_area *a = &ui->scroll_areas[area];
	voe_ui_rect visible = ui->nodes[a->node].visible;
	struct voe_ui_scrollbar bar = scrollbar(ui, a, y);
	voe_math_float2 at = ui->pointer.at;

	if (!bar.shows || !voe_ui_inside(intersect(bar.track, visible), at))
		return;

	*hit = (struct voe_ui_hit){
		.kind = voe_ui_inside(intersect(bar.thumb, visible), at)
				? VOE_UI_HIT_THUMB
				: VOE_UI_HIT_TRACK,
		.area = area,
		.bar = y ? VOE_UI_BAR_Y : VOE_UI_BAR_X,
		.towards = component(at, y) < component(bar.thumb.min, y)
				   ? -1.0f
				   : 1.0f,
	};
}

// Every bar that paints straight after paint position `at` against the pointer,
// which is where one is in front of the content for the pointer exactly as far
// as it is in front for the eye. Both bars of each, the later winning, as the
// walk in button.c's hit test does for widgets.
void voe_ui_scroll_hit(const voe_ui_context *ui, uint32_t at,
		       struct voe_ui_hit *hit)
{
	for (uint32_t s = bar_after(ui, at, ui->scroll_count); s != NO_AREA;
	     s = bar_after(ui, at, s)) {
		hit_bar(ui, s, false, hit);
		hit_bar(ui, s, true, hit);
	}
}

// What a press on a bar does, on the same edge that arms a widget: a thumb is
// held under its area's key, with where the pointer was and the offset it was
// at kept so the drag follows the press; a track holds nothing and pages once.
void voe_ui_scroll_press(voe_ui_context *ui, const struct voe_ui_hit *hit)
{
	if (hit->kind == VOE_UI_HIT_THUMB) {
		const struct voe_ui_scroll_area *a =
			&ui->scroll_areas[hit->area];
		bool y = hit->bar == VOE_UI_BAR_Y;

		ui->held = a->key;
		ui->held_set = true;
		ui->held_thumb = hit->bar;
		ui->thumb_press_at = ui->pointer.at;
		ui->thumb_press_offset =
			component(ui->nodes[a->node].scrolled, y);
	} else if (hit->kind == VOE_UI_HIT_TRACK) {
		// Holds nothing, and pages once: the press is the edge.
		ui->page = hit->bar;
		ui->page_area = hit->area;
		ui->page_towards = hit->towards;
	}
}

// A solid record over `bounds`, clipped to `within`, and not pushed when nothing
// of it is left.
static void push_solid_within(voe_ui_context *ui, voe_ui_rect bounds,
			      voe_ui_rect within, voe_math_float4 colour)
{
	voe_ui_rect clip = intersect(bounds, within);

	if (clip.size.x <= 0.0f || clip.size.y <= 0.0f)
		return;

	voe_ui_push_element(ui, (voe_render_element){
				 .bounds = voe_ui_bounds_of(bounds),
				 .clip = voe_ui_bounds_of(clip),
				 .colour = colour,
				 .kind = VOE_RENDER_ELEMENT_SOLID,
			 });
}

// One of an area's bars, track and then thumb, clipped to the area's visible
// rectangle. The track is `ground`, the thumb a button's own three states —
// control, control_hovered, and `inverse` while held, the thumb being dragged
// drawn inverted as any other held control is (ADR-0196). Held beats hovered,
// as it does on a button.
static void push_scrollbar(voe_ui_context *ui, uint32_t area, bool y)
{
	const struct voe_ui_scroll_area *a = &ui->scroll_areas[area];
	const voe_ui_theme *theme = ui->widgets[a->node].theme;
	struct voe_ui_scrollbar bar = scrollbar(ui, a, y);
	enum voe_ui_bar which = y ? VOE_UI_BAR_Y : VOE_UI_BAR_X;
	voe_ui_rect visible = ui->nodes[a->node].visible;
	voe_math_float4 thumb = theme->control;

	if (!bar.shows)
		return;

	if (ui->held_set && ui->held_thumb == which && ui->held == a->key)
		thumb = theme->inverse;
	else if (ui->hovered_thumb == which && ui->hovered_thumb_area == area)
		thumb = theme->control_hovered;

	push_solid_within(ui, bar.track, visible, theme->ground);
	push_solid_within(ui, bar.thumb, visible, thumb);
}

// Every bar that paints straight after paint position `at`, which is where
// "after the area's children" is in paint order — see this file's header.
void voe_ui_scroll_emit(voe_ui_context *ui, uint32_t at)
{
	for (uint32_t s = bar_after(ui, at, ui->scroll_count); s != NO_AREA;
	     s = bar_after(ui, at, s)) {
		push_scrollbar(ui, s, false);
		push_scrollbar(ui, s, true);
	}
}

// ------------------------------------------------------ moving a scroll area

// This frame's areas written back as the table, in call order, so that entry i
// of both is one area from here to the next frame_begin. See this file's header,
// and its DEVIATION, for what a refused frame writes.
void voe_ui_scrolls_remember(voe_ui_context *ui, bool laid_out)
{
	for (uint32_t i = 0; i < ui->scroll_count; i++) {
		const struct voe_ui_scroll_area *a = &ui->scroll_areas[i];

		ui->scroll_memory[i] = (struct voe_ui_scroll_memory){
			.key = a->key,
			.offset = laid_out ? ui->nodes[a->node].scrolled
					   : a->handed,
		};
	}
	ui->scroll_remembered = ui->scroll_count;
}

// The nearest area around `area`, or NO_AREA. Areas are in call order, so one
// that holds this one comes before it, and holding is a subtree range.
static uint32_t outward(const voe_ui_context *ui, uint32_t area)
{
	uint32_t inner = ui->scroll_areas[area].node;

	while (area-- > 0) {
		uint32_t outer = ui->scroll_areas[area].node;

		if (outer < inner && inner < outer + ui->nodes[outer].subtree)
			return area;
	}
	return NO_AREA;
}

// Scrolls by a length, starting at `area`: each area takes what its clamp allows
// on each axis it scrolls and passes the rest outward, and what nobody takes is
// dropped. Into the table, so it lands in the next frame's layout.
//
// THE ONE PATH A LENGTH TAKES. The pointer's scroll comes through here today, and
// voe_ui_scroll_by is this with a node for `area` once focus gives it a caller.
static void scroll_by(voe_ui_context *ui, uint32_t area, voe_math_float2 length)
{
	for (int axis = 0; axis < 2; axis++) {
		bool y = axis == 1;
		float left = component(length, y);

		for (uint32_t s = area; s != NO_AREA && left != 0.0f;
		     s = outward(ui, s)) {
			const struct voe_ui_scroll_area *a = &ui->scroll_areas[s];
			float wanted;
			float kept;

			if (!scrolls_on(a->axes, y))
				continue;

			wanted = component(ui->scroll_memory[s].offset, y) + left;
			kept = clamp_offset(wanted, scroll_range(ui, a->node, y));
			component_set(&ui->scroll_memory[s].offset, y, kept);
			// Nought exactly when nothing was clamped away.
			left = wanted - kept;
		}
	}
}

// The innermost area whose visible rectangle holds the pointer — the last in
// paint order, which for nested areas is the inner — or NO_AREA.
static uint32_t area_under_pointer(const voe_ui_context *ui)
{
	uint32_t best = NO_AREA;

	for (uint32_t i = 0; i < ui->scroll_count; i++) {
		const struct voe_ui_node_record *n =
			&ui->nodes[ui->scroll_areas[i].node];

		if (!voe_ui_inside(n->visible, ui->pointer.at))
			continue;
		if (best == NO_AREA ||
		    n->paint > ui->nodes[ui->scroll_areas[best].node].paint)
			best = i;
	}
	return best;
}

// The held thumb's area follows the pointer's travel since the press, times
// measured over arranged.
static void thumb_drag(voe_ui_context *ui)
{
	bool y = ui->held_thumb == VOE_UI_BAR_Y;

	for (uint32_t i = 0; i < ui->scroll_count; i++) {
		const struct voe_ui_scroll_area *a = &ui->scroll_areas[i];
		const struct voe_ui_node_record *n = &ui->nodes[a->node];
		float arranged = component(n->rect.size, y);
		float measured = component(n->content_natural, y);
		float travel;
		float offset;

		if (a->key != ui->held)
			continue;
		// Nothing arranged has nothing to be a proportion of.
		if (arranged <= 0.0f)
			return;

		travel = component(ui->pointer.at, y) -
			 component(ui->thumb_press_at, y);
		offset = ui->thumb_press_offset + travel * measured / arranged;
		component_set(&ui->scroll_memory[i].offset, y,
			      clamp_offset(offset, scroll_range(ui, a->node, y)));
		return;
	}
}

// A track pressed this frame: one arranged length towards the pointer.
static void page(voe_ui_context *ui)
{
	const struct voe_ui_scroll_area *a = &ui->scroll_areas[ui->page_area];
	bool y = ui->page == VOE_UI_BAR_Y;
	float offset = component(ui->scroll_memory[ui->page_area].offset, y) +
		       ui->page_towards *
			       component(ui->nodes[a->node].rect.size, y);

	component_set(&ui->scroll_memory[ui->page_area].offset, y,
		      clamp_offset(offset, scroll_range(ui, a->node, y)));
}

// From the offset this frame was laid out at, which is where the rectangles read
// here are, and straight into the table: a program's centring passes nothing
// outward, so it is not scroll_by.
void voe_ui_scroll_centre(voe_ui_context *ui, voe_ui_node area, voe_ui_node node,
			  voe_ui_scroll_axes axes)
{
	voe_ui_rect rect = voe_ui_node_rect(ui, node);
	voe_ui_rect visible = voe_ui_node_visible(ui, area);
	uint32_t s = 0;

	while (s < ui->scroll_count && ui->scroll_areas[s].node != area)
		s++;
	VOE_BASE_ASSERT(s < ui->scroll_count,
			"centring in a node that is not a scroll area this "
			"frame made");

	for (int axis = 0; axis < 2; axis++) {
		bool y = axis == 1;
		const struct voe_ui_scroll_area *a = &ui->scroll_areas[s];
		float low = component(rect.min, y);
		float high = low + component(rect.size, y);
		float seen_low = component(visible.min, y);
		float seen_high = seen_low + component(visible.size, y);
		float offset;

		if (!scrolls_on(axes, y) || !scrolls_on(a->axes, y) ||
		    (low >= seen_low && high <= seen_high))
			continue;
		offset = component(ui->nodes[area].scrolled, y) +
			 (low + high) / 2.0f - (seen_low + seen_high) / 2.0f;
		component_set(&ui->scroll_memory[s].offset, y,
			      clamp_offset(offset, scroll_range(ui, area, y)));
	}
}

void voe_ui_scrolls_move(voe_ui_context *ui)
{
	uint32_t under;

	if (ui->held_set && ui->held_thumb != VOE_UI_BAR_NONE)
		thumb_drag(ui);
	if (ui->page != VOE_UI_BAR_NONE)
		page(ui);

	if (!ui->pointer.over ||
	    (ui->pointer.scroll.x == 0.0f && ui->pointer.scroll.y == 0.0f))
		return;
	under = area_under_pointer(ui);
	if (under != NO_AREA)
		scroll_by(ui, under, ui->pointer.scroll);
}
