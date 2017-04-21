#include "pch.h"
#include "game.h"

const wchar_t* g_win_name = L"LD38 - Base Code";

draw_list g_dl_world;
draw_list g_dl_ui;

struct player : entity {
	player() : entity(ET_PLAYER) {
	}

	virtual void tick() {
		vec2 pad_left = g_input.pad_left;

		if (is_key_down(g_input.bind_left )) pad_left.x -= 1.0f;
		if (is_key_down(g_input.bind_right)) pad_left.x += 1.0f;
		if (is_key_down(g_input.bind_up   )) pad_left.y -= 1.0f;
		if (is_key_down(g_input.bind_down )) pad_left.y += 1.0f;

		if (is_key_down(KEY_LEFT )) pad_left.x -= 1.0f;
		if (is_key_down(KEY_RIGHT)) pad_left.x += 1.0f;
		if (is_key_down(KEY_UP   )) pad_left.y -= 1.0f;
		if (is_key_down(KEY_DOWN )) pad_left.y += 1.0f;

		if (length_sq(pad_left) > 1.0f)
			pad_left = normalise(pad_left);

		_vel *= 0.8f;
		_vel += pad_left * 40.0f;

		entity_move_slide(this);
	}

	virtual void draw(draw_context* dc) {
		dc->set(g_sheet);
		draw_tile(*dc, _pos, _radius, _colour, 0, 0);
	}
};

void game_init() {
	g_dl_world.init(64 * 1024, 4096);
	g_dl_ui.init(64 * 1024, 4096);

	define_sound(sfx::DIT, "dit", 2, 2);

	spawn_entity(new player, vec2());
}

void game_frame(vec2 view_size) {
	world* w = &g_world;

	g_dl_world.reset();
	g_dl_ui.reset();

	g_request_mouse_capture = false;

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