// The game's interface, as game/include/game/interface.h gives: font, theme
// and context made once, and each frame begun with the pointer in the
// surface's millimetres before the project's entry point is called.
#include <game/interface.h>

#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>

#include <platform/input.h>

#include <text/font.h>

#include <ui/theme.h>
#include <ui/widgets.h>

#include <stddef.h>

struct voe_game_interface {
	voe_text_font *font;
	// ui keeps a pointer to it, so it lives here and not on a stack.
	voe_ui_theme theme;
	voe_ui_context *ui;
};

voe_game_interface *voe_game_interface_new(voe_render_device *device,
					   voe_base_arena *arena)
{
	voe_base_error error = VOE_BASE_OK;
	voe_ui_theme_inputs inputs = voe_ui_theme_default_inputs();
	voe_game_interface *interface;
	struct voe_base_arena_mark mark;
	voe_text_font *font;

	VOE_BASE_ASSERT(device != NULL, "an interface on no device");
	VOE_BASE_ASSERT(arena != NULL, "an interface in no arena");

	// The font's outlines and pixels are scratch, rewound before anything
	// that stays is pushed.
	mark = voe_base_arena_mark(arena);
	font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, device, arena,
				 &error);
	voe_base_arena_rewind(arena, mark);
	if (font == NULL) {
		VOE_BASE_ERROR("game", "the interface's font was refused: %s",
			       voe_base_error_string(error));
		return NULL;
	}

	interface = voe_base_arena_push(arena, sizeof(*interface));
	interface->font = font;
	interface->theme = voe_ui_theme_derive(&inputs, font);
	interface->ui = voe_ui_context_new(
		arena, (voe_ui_capacities){
			       .nodes = VOE_GAME_INTERFACE_NODES,
			       .elements = VOE_GAME_INTERFACE_ELEMENTS });
	voe_ui_font_set(interface->ui, font);
	voe_ui_theme_set(interface->ui, &interface->theme);
	return interface;
}

void voe_game_interface_destroy(voe_game_interface *interface)
{
	VOE_BASE_ASSERT(interface != NULL, "destroying no interface");
	voe_text_font_destroy(interface->font);
}

// Pixels per millimetre: the window's height holds VOE_GAME_SURFACE_HIGH.
static float pixels_per_millimetre(voe_platform_size size)
{
	VOE_BASE_ASSERT(size.width > 0 && size.height > 0,
			"an interface surface with no area");
	return (float)size.height / VOE_GAME_SURFACE_HIGH;
}

voe_math_float2 voe_game_interface_surface(voe_platform_size size)
{
	return voe_render_element_surface_size(size,
					       pixels_per_millimetre(size));
}

bool voe_game_interface_run(
	voe_game_interface *interface, voe_base_arena *frame_arena,
	voe_ecs_world *world, voe_platform_window *window,
	voe_platform_size size, voe_game_project_asks *asks,
	bool (*project_interface)(const voe_game_project_frame *frame))
{
	float scale = pixels_per_millimetre(size);
	voe_ui_pointer pointer = { 0 };

	VOE_BASE_ASSERT(interface != NULL, "running no interface");
	VOE_BASE_ASSERT(asks != NULL, "an interface with no asks");
	VOE_BASE_ASSERT(project_interface != NULL, "no project interface");

	voe_ui_frame_begin(interface->ui, frame_arena);
	if (window != NULL) {
		voe_platform_pointer at = voe_platform_input_pointer(window);

		pointer = (voe_ui_pointer){
			.at = { at.x / scale, at.y / scale },
			.over = at.over,
			.down = voe_platform_input_button_down(
				window, VOE_PLATFORM_BUTTON_LEFT),
		};
	}
	voe_ui_pointer_set(interface->ui, pointer);

	return project_interface(&(voe_game_project_frame){
		.world = world,
		.window = window,
		.ui = interface->ui,
		.size = voe_game_interface_surface(size),
		.asks = asks,
	});
}

voe_ui_context *voe_game_interface_context(voe_game_interface *interface)
{
	VOE_BASE_ASSERT(interface != NULL, "no interface");
	return interface->ui;
}
