#include "pch.h"
#include "game.h"

/*

	Game ideas
	----------

	Shooting microbes in a petri dish
	Collect gems in a small world, PMCE style half level switching when one side is complete
	Shoot enemies to freeze, pulse to destroy, can't shoot after pulsing, remaining enemies get buffed
	Small floating circles are the level, shift between circles when they get close
	3d meta balls surface as world, constantly shifts, fly around surface and shoot stuff
	Eggs in a nest, protect them from a cuckoo who keeps switching them with her eggs, push out cuckoo eggs, fend off other creatures trying to eat your eggs
	Tether small planets together, construct a building on each planet, tether large numbers together to make a base you have to defend against waves of enemies, enemies shoot worlds/tethers to break your base into pieces
	Something with crazy weapons worry about small world later

*/

const wchar_t* g_win_name = L"LD38 - Small World!";

draw_list g_dl_world;
draw_list g_dl_ui;

void game_init() {
	g_dl_world.init(256 * 1024, 4096);
	g_dl_ui.init(256 * 1024, 4096);

	define_sound(sfx::DIT, "dit", 2, 2);

	init_stars();

	//for(int i = 0; i < 200; i++) {
	//	spawn_entity(new planet, g_world.r.range(vec2(5000.0f, 5000.0f)));
	//}

	//if (planet* p = get_nearest_planet(vec2())) {
	if (planet* p = spawn_entity(new planet, vec2())) {
		p->_captured = true;
		p->_desired_radius = 80.0f;

		if (player* pl = spawn_entity(new player, p->_pos)) {
			pl->_last_planet = get_entity_handle(p);
		}
	}
}

void game_frame(vec2 view_size) {
	world* w = &g_world;

	g_dl_world.reset();
	g_dl_ui.reset();

	g_request_mouse_capture = true;

	draw_context dc(g_dl_world);
	draw_context dc_ui(g_dl_ui);

	w->view_size = view_size;

	// update

	world_tick();

	// draw

	world_draw(&dc);

	// render

	gpu_set_pipeline(g_pipeline_sprite);
	gpu_set_const(0, w->view_proj);
	g_dl_world.render();

	gpu_set_const(0, fit_ui_proj_view(640.0f, 360.0f, w->view_size.x / w->view_size.y, -64.0f, 64.0f));
	g_dl_ui.render();

	debug_set_proj_view(w->view_proj);
}