// The game's whole run: a window, the cooked scene built into a fresh world,
// and frames until the window closes. A game tree's main.c is one call:
//
//     int main(void)
//     {
//             return voe_game_run("My game");
//     }
//
// THE ORDER: two arenas, voe_app_new at VOE_GAME_WIDTH by VOE_GAME_HEIGHT with
// `title`, the world (game/world.h), the project's voe_game_project_register
// (game/project.h), voe_game_scene_build (game/scene.h), the built-in shapes
// uploaded, then once a frame, skipping a minimised window, until it is
// closing: the frame's elapsed seconds into voe_game_steps_run, which runs
// voe_game_project_systems_run and voe_game_project_systems_after_move once
// per fixed step (game/steps.h), then
// voe_game_frame with the lag it returns.
//
// NO QUIT KEY (0234). Escape is the game's own, for its menus; the window's
// close, the system's close key, or the editor's Stop ends the run.
//
// Constraints: the window size is fixed until a project setting names one
// (0234). Links only in a tree that has a cooked scene.c defining
// voe_game_scene_build and a project defining the three entry points of
// game/project.h (register, systems_run, systems_after_move); run.c is the
// one file that names them.
#pragma once

// The window the game opens in, whatever the editor's size (0234).
#define VOE_GAME_WIDTH 1280
#define VOE_GAME_HEIGHT 720

// 0 when the window was closed. 1 with a line on stderr when the window or
// device would not open, the scene did not fit its world, the shapes did not
// fit the device, or the device stopped answering. `title` is the window's.
int voe_game_run(const char *title);
