#include "pch.h"
#include "game.h"

const wchar_t* g_win_name = L"LD38 - Small World!";

draw_list g_dl_world;
draw_list g_dl_ui;

void game_init() {
	g_dl_world.init(256 * 1024, 4096);
	g_dl_ui.init(256 * 1024, 4096);

	define_sound(sfx::DIT,                      "dit",                    2, 2);
	define_sound(sfx::PLANET_DIE,               "planet_die",             2, 2);
	define_sound(sfx::PLANET_HURT,              "planet_hurt",            2, 2);
	define_sound(sfx::PLANET_GROW,              "planet_grow",            2, 2);
	define_sound(sfx::PLANET_HEAL,              "planet_heal",            2, 2);
	define_sound(sfx::PLAYER_FIRE,              "player_fire",            8, 0);
	define_sound(sfx::PLAYER_FIRE_ALT,          "player_fire_alt",        8, 0);
	define_sound(sfx::PLAYER_TETHER_READY,      "player_tether_ready",    1, 20);
	define_sound(sfx::PLAYER_BUILD_READY,       "player_build_ready",     1, 20);
	define_sound(sfx::PLAYER_BUILD_TURRET,      "player_build_turret",    1, 20);
	define_sound(sfx::PLAYER_BUILD_GENERATOR,   "player_build_generator", 1, 20);
	define_sound(sfx::TRACKER_DIE,              "tracker_die",            5, 1);

	init_stars();
	psys_init(1000);
}

void start_game() {
	world* w = &g_world;

	w->entities.free();
	w->handles.free();

	if (planet* p = spawn_entity(new planet, vec2())) {
		p->_captured = true;
		p->_desired_radius = 60.0f;
		p->_radius = 10.0f;
		p->_grown = true;

		if (player* pl = spawn_entity(new player, p->_pos)) {
			pl->_last_planet = get_entity_handle(p);
		}
	}

	w->game_time = 0;
	w->player_dead_time = 0;

	w->num_planets_created = 0;
	w->num_buildings_created = 0;
	w->num_planets_hurt = 0;

	w->level = 0;
	w->level_timer = 0;
}

enum game_state {
	STATE_MENU,
	STATE_GAME,
	STATE_RESULTS
};

game_state g_state;

void game_frame(vec2 view_size) {
	world* w = &g_world;

	g_dl_world.reset();
	g_dl_ui.reset();

	g_request_mouse_capture = g_state == STATE_GAME;

	draw_context dc(g_dl_world);
	draw_context dc_ui(g_dl_ui);

	w->view_size = view_size;

	// update

	if (g_state == STATE_MENU) {

		draw_string(dc_ui, vec2(320.0f, 80.0f), vec2(4.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba(), "cell division");
		draw_string(dc_ui, vec2(320.0f, 110.0f), vec2(0.75f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.6f), "created for ludum dare 38 by stephen cakebread @quantumrain");

		draw_string(dc_ui, vec2(320.0f, 160.0f), vec2(1.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.8f), "controls - mouse");
		draw_string(dc_ui, vec2(320.0f, 175.0f), vec2(1.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.8f), "fullscreen - F11");

		draw_string(dc_ui, vec2(320.0f, 220.0f), vec2(1.0f), TEXT_CENTRE | TEXT_VCENTRE, rgba(1.0f), "playing with a mouse is highly recommended");
		draw_string(dc_ui, vec2(320.0f, 230.0f), vec2(1.0f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.6f), "if you're using a trackpad you may use");
		draw_string(dc_ui, vec2(320.0f, 240.0f), vec2(1.0f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.6f), "Z and X as left / right mouse buttons");
		draw_string(dc_ui, vec2(320.0f, 250.0f), vec2(1.0f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.6f), "if you have neither, arrow keys do work");


		draw_string(dc_ui, vec2(320.0f, 300.0f), vec2(1.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba(), "left click to start");

		if (g_input.start) {
			g_state = STATE_GAME;
			start_game();
		}
	}
	else if (g_state == STATE_GAME) {
		if (is_key_pressed(KEY_ESCAPE)) {
			g_state = STATE_MENU;

			for(auto& e : w->entities)
				destroy_entity(e);
		}

		if (!get_player()) {
			g_state = STATE_RESULTS;
		}
	}
	else if (g_state == STATE_RESULTS) {
		draw_string(dc_ui, vec2(320.0f, 80.0f), vec2(2.0f), TEXT_CENTRE | TEXT_VCENTRE, rgba(), "cell death");

		draw_stringf(dc_ui, vec2(320.0f, 120.0f), vec2(1.0f), TEXT_CENTRE | TEXT_VCENTRE, rgba(), "you survived %.1f seconds", w->player_dead_time / 60.0f);

		if (g_input.start) {
			g_state = STATE_MENU;

			for(auto& e : w->entities)
				destroy_entity(e);
		}
	}

	world_tick(g_state == STATE_MENU);

	psys_update();

	// camera

	if (player* pl = get_player()) {
		vec2 player_pos = pl->_pos;

		vec2  target_pos   = player_pos;
		vec2  target_delta = target_pos - w->camera_target;
		float target_dist  = length(target_delta);

		if (target_dist > 0.01f) {
			target_delta /= target_dist;

			float d = max(dot(target_pos - w->camera_old_target, target_delta), 0.0f);

			d *= square(square(clamp((target_dist - 0.0f) / 90.0f, 0.0f, 1.0f)));

			w->camera_target += target_delta * d;
		}

		w->camera_old_target = target_pos;

		vec2 camera_delta = (w->camera_target - w->camera_pos) * 0.1f;

		if (planet* p = get_nearest_planet(player_pos))
			camera_delta += p->_vel * DT;

		w->camera_pos += camera_delta;
		update_stars(camera_delta);
	}

	// view

	w->view_proj = top_down_proj_view(-w->camera_pos, 90.0f, w->view_size.x / w->view_size.y, 400.0f, 1.0f, 1000.0f);

	// draw

	world_draw(&dc);

	if (1) {
		static float shoot_vis;
		static float move_vis;
		static float heal_vis;

		if (player* p = get_player()) {
			shoot_vis += (((p->_shots < 10) ? 1.0f : 0.0f) - shoot_vis) * 0.2f;
			move_vis += (((p->_moves < 50.0f) ? 1.0f : 0.0f) - move_vis) * 0.2f;
			heal_vis += ((((w->num_planets_hurt > 35) && (p->_heals_done < 25)) ? 1.0f : 0.0f) - heal_vis) * 0.2f;

			if (move_vis > 0.1f)
				draw_string(dc_ui, vec2(320.0f, 320.0f), vec2(1.0f), TEXT_CENTRE, rgba() * move_vis, "move with the mouse");
			else if ((p->_tether_shot_visible > 0.1f) && (g_world.num_planets_created < 3))
				draw_string(dc_ui, vec2(320.0f, 320.0f), vec2(1.0f), TEXT_CENTRE, rgba() * p->_tether_shot_visible, "right click to grow");
			else if ((p->_build_visible > 0.1f) && (g_world.num_buildings_created < 3))
				draw_string(dc_ui, vec2(320.0f, 320.0f), vec2(1.0f), TEXT_CENTRE, rgba() * p->_build_visible, "right click to build");
			else if (shoot_vis > 0.1f)
				draw_string(dc_ui, vec2(320.0f, 320.0f), vec2(1.0f), TEXT_CENTRE, rgba() * shoot_vis, "left click to fire");
			else if (heal_vis > 0.1f)
				draw_string(dc_ui, vec2(320.0f, 320.0f), vec2(1.0f), TEXT_CENTRE, rgba() * heal_vis, "cells heal while you're inside them");
		}
		else {
			shoot_vis = 0.0f;
			move_vis = 0.0f;
			heal_vis = 0.0f;
		}
	}


	// render

	gpu_set_pipeline(g_pipeline_sprite);
	gpu_set_const(0, w->view_proj);
	g_dl_world.render();

	gpu_set_const(0, fit_ui_proj_view(640.0f, 360.0f, w->view_size.x / w->view_size.y, -64.0f, 64.0f));
	g_dl_ui.render();

	debug_set_proj_view(w->view_proj);
}