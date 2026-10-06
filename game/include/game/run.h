// The game's whole run: a window, the cooked scene built into a fresh world,
// and frames until the window closes. A game tree's main.c is one call:
//
//     int main(void)
//     {
//             voe_game_window window = { 1280, 720, false };
//
//             return voe_game_run("My game", window);
//     }
//
// THE WINDOW IS HANDED IN (0291). The editor's game tree writes it into the
// generated main.c from the project's settings, so the game reads no project
// text (0236). Fullscreen takes the screen and ignores the size. The window
// can be resized to any shape; the world is drawn at the window's aspect,
// never stretched.
//
// THE ORDER: four arenas, a start log begun (app/start_log.h),
// voe_app_new with `window` and `title`, the interface (game/interface.h),
// splashscreen.png read from the program's folder (the plain screen
// and a stderr line when it will not read), then voe_game_starting_wait
// (game/starting.h) showing the progress on it while one worker, in its
// own scratch (0370 point 4), runs voe_game_starting_shaders with the
// game's pipeline cache (app/pipeline_cache.h, "pipelines_game.cache"),
// makes the world (game/world.h) in an arena of its own with the project's
// voe_game_project_register (game/project.h) and voe_game_scene_build
// (game/scene.h) as "Loading scene", uploads the built-in shapes, and
// makes the model store (3d/models.h) and runs voe_game_models_update
// (game/models.h) from the program's folder with the progress. A window closed
// during the wait ends the run as a close does, whatever the worker made
// released. After it, on the main thread: the splash's texture given back,
// the mixer (audio/mixer.h) on the program's folder and the sound device
// (platform/sound.h), then once a frame, skipping a minimised window,
// until it is closing: on a restart asked, the world's arena cleared and
// the world, register and scene made again, the bank zeroed and the models
// read, both asks false; unless paused, the frame's elapsed seconds and
// the cooked prefabs (game/prefabs.h) into voe_game_steps_run, which runs
// voe_game_project_systems_run and voe_game_project_systems_after_move once
// per fixed step (game/steps.h), then voe_game_models_update again for paths
// the step named, then voe_game_interface_run with voe_game_project_interface
// and the asks in the cleared scratch, the mixer paused as the asks
// say and pumped into an open device, then voe_game_frame with the lag
// (the last one while paused), the store and the interface's context;
// after the first, the log's steps ("window and device", "interface",
// "preparing shaders", "world and scene", "shapes and models", "sound",
// "first frame"; the middle three taken by the worker) go to stderr as
// program "game". The store is cleared and destroyed before the window
// closes. A game with no sound device, or whose device fails, runs silent;
// a model that will not read draws as nothing.
//
// NO QUIT KEY (0234), BUT THE PROJECT'S INTERFACE MAY END THE RUN (0259).
// Escape is the game's own, for its menus; the window's close, the system's
// close key, the editor's Stop, or voe_game_project_interface answering false
// ends the run. The interface may also pause the run and start the level
// again (0333, game/project.h).
//
// Constraints: links only in a tree with a cooked scene.c defining
// voe_game_scene_build, a cooked prefabs.c defining voe_game_prefabs_cooked,
// and a project defining game/project.h's four entry points (register,
// systems_run, systems_after_move, interface), which only run.c names.
#pragma once

#include <stdbool.h>

// The window the game opens in, whatever the editor's size: `width` by
// `height` pixels, both above nought, or the whole screen when `fullscreen`.
typedef struct voe_game_window {
	int width;
	int height;
	bool fullscreen;
} voe_game_window;

// 0 when the window was closed or the project's interface ended the run. 1
// with a line on stderr when the window, device or interface would not open,
// the shaders would not prepare, the scene did not fit its world (at the start or a restart), the shapes
// did not fit the device, or the device stopped answering. `title` is the
// window's.
int voe_game_run(const char *title, voe_game_window window);
