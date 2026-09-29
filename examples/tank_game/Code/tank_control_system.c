// The tank control's step: registers tank_control, adds it to the first
// hull, then reads the keyboard, mouse and lowest connected pad into it.
//
// Which device is in use (tank_control.h): the pad when touched, the
// keyboard when touched, the keyboard on a tie, else as it was; no pad
// connected is the keyboard. The whole row is written each step.
//
// Constraints: the runtime-only marker is taken by address at run time,
// because a project library imports it on Windows (0245). A full structural
// queue tries again next step.
#include "tank_control.h"
#include "tank_hull.h"

#include <base/assert.h>

#include <ecs/component.h>
#include <ecs/structure.h>

#include <game/project.h>

#include <platform/input.h>

#include <math.h>

#define TANK_CONTROL_DEAD_ZONE 0.2f
#define TANK_CONTROL_TRIGGER 0.5f

const struct voe_ecs_key tank_control_key = { "tank_control" };

bool tank_control_register(voe_ecs_world *world)
{
	VOE_BASE_ASSERT(world != NULL, "registering the tank control in no world");
	const bool ok = voe_game_project_component(world,
		&(voe_game_project_type){
			&tank_control_key, sizeof(tank_control), 1,
			&voe_ecs_runtime_only, NULL, NULL });

	return ok;
}

typedef struct {
	float x;
	float y;
} tank_control_stick;

// The stick through the radial dead zone: under it 0, past it the same
// direction at (length - zone) / (1 - zone), at most 1.
static tank_control_stick tank_control_dead_zone(float x, float y)
{
	VOE_BASE_ASSERT(isfinite(x) && isfinite(y), "a stick off the scale");
	const float length = sqrtf(x * x + y * y);

	if (length < TANK_CONTROL_DEAD_ZONE)
		return (tank_control_stick){ 0.0f, 0.0f };
	const float scaled = fminf((length - TANK_CONTROL_DEAD_ZONE) /
					   (1.0f - TANK_CONTROL_DEAD_ZONE),
				   1.0f);
	const tank_control_stick out = { x / length * scaled,
					 y / length * scaled };

	VOE_BASE_DEBUG_ASSERT(sqrtf(out.x * out.x + out.y * out.y) < 1.01f,
			      "a stick past one");
	return out;
}

// The lowest connected slot's pad, or one not connected.
static voe_platform_gamepad tank_control_pad(voe_platform_window *window)
{
	VOE_BASE_ASSERT(window != NULL, "reading a pad with no window");
	for (int slot = 0; slot < VOE_PLATFORM_GAMEPAD_SLOTS; slot++) {
		const voe_platform_gamepad pad =
			voe_platform_input_gamepad(window, slot);

		if (pad.connected)
			return pad;
	}
	return (voe_platform_gamepad){ 0 };
}

static bool tank_control_pad_touched(const voe_platform_gamepad *pad,
				     tank_control_stick left,
				     tank_control_stick right)
{
	VOE_BASE_ASSERT(pad != NULL, "touching no pad");
	if (left.x != 0.0f || left.y != 0.0f || right.x != 0.0f ||
	    right.y != 0.0f || pad->left_trigger > TANK_CONTROL_TRIGGER ||
	    pad->right_trigger > TANK_CONTROL_TRIGGER)
		return true;
	for (int b = 0; b < VOE_PLATFORM_GAMEPAD_BUTTON_COUNT; b++)
		if (pad->buttons[b])
			return true;
	return false;
}

static bool tank_control_keyboard_touched(voe_platform_window *window,
					  const tank_control *last,
					  voe_platform_pointer pointer)
{
	VOE_BASE_ASSERT(window != NULL && last != NULL,
			"touching no keyboard");
	const voe_platform_key keys[] = {
		VOE_PLATFORM_KEY_W, VOE_PLATFORM_KEY_A, VOE_PLATFORM_KEY_S,
		VOE_PLATFORM_KEY_D, VOE_PLATFORM_KEY_SPACE,
	};

	for (unsigned k = 0; k < sizeof(keys) / sizeof(keys[0]); k++)
		if (voe_platform_input_key_down(window, keys[k]))
			return true;
	for (int b = 0; b < VOE_PLATFORM_BUTTON_COUNT; b++)
		if (voe_platform_input_button_down(window,
						   (voe_platform_button)b))
			return true;
	return pointer.x != last->x || pointer.y != last->y;
}

// -1, 0 or 1 from a pair of opposite keys.
static float tank_control_keys(voe_platform_window *window,
			       voe_platform_key minus, voe_platform_key plus)
{
	VOE_BASE_ASSERT(window != NULL, "reading keys with no window");
	return (voe_platform_input_key_down(window, plus) ? 1.0f : 0.0f) -
	       (voe_platform_input_key_down(window, minus) ? 1.0f : 0.0f);
}

// This step's row from the window, given the row as it was.
static tank_control tank_control_read(voe_platform_window *window,
				      const tank_control *last)
{
	VOE_BASE_ASSERT(window != NULL && last != NULL, "reading no controls");
	const voe_platform_gamepad pad = tank_control_pad(window);
	const tank_control_stick left =
		tank_control_dead_zone(pad.left_x, pad.left_y);
	const tank_control_stick right =
		tank_control_dead_zone(pad.right_x, pad.right_y);
	const voe_platform_pointer pointer = voe_platform_input_pointer(window);
	tank_control row = { .pad = last->pad, .x = pointer.x, .y = pointer.y };

	if (!pad.connected ||
	    tank_control_keyboard_touched(window, last, pointer))
		row.pad = false;
	else if (tank_control_pad_touched(&pad, left, right))
		row.pad = true;
	if (row.pad) {
		row.drive = left.y;
		row.turn = -left.x;
		row.aim_x = right.x;
		row.aim_y = right.y;
	} else {
		row.drive = tank_control_keys(window, VOE_PLATFORM_KEY_S,
					      VOE_PLATFORM_KEY_W);
		row.turn = tank_control_keys(window, VOE_PLATFORM_KEY_D,
					     VOE_PLATFORM_KEY_A);
	}
	row.fire = voe_platform_input_button_down(window,
						  VOE_PLATFORM_BUTTON_LEFT) ||
		   voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SPACE) ||
		   pad.right_trigger > TANK_CONTROL_TRIGGER;
	VOE_BASE_DEBUG_ASSERT(fabsf(row.drive) <= 1.0f && fabsf(row.turn) <= 1.0f,
			      "a control past one");
	return row;
}

void tank_control_run(const voe_game_project_step *step)
{
	VOE_BASE_ASSERT(step != NULL && step->world != NULL,
			"reading the tank control in no world");
	voe_ecs_world *world = step->world;
	const voe_ecs_type type =
		voe_ecs_component_type(world, &tank_control_key);
	const voe_ecs_type hull_type =
		voe_ecs_component_type(world, &tank_hull_key);

	if (step->window == NULL)
		return;
	if (voe_ecs_component_count(world, type) == 0) {
		if (voe_ecs_component_count(world, hull_type) == 0)
			return;
		const voe_platform_pointer pointer =
			voe_platform_input_pointer(step->window);
		const tank_control first = { .x = pointer.x, .y = pointer.y };

		(void)voe_ecs_structure_add(
			world, type, voe_ecs_component_entities(world, hull_type)[0],
			&first);
		return;
	}
	const voe_ecs_entity entity = voe_ecs_component_entities(world, type)[0];
	const tank_control row = tank_control_read(
		step->window, voe_ecs_component_rows(world, type));
	const bool written = voe_ecs_component_set(world, type, entity, &row);

	VOE_BASE_ASSERT(written, "the tank control row went missing");
}
