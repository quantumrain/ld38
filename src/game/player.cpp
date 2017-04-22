#include "pch.h"
#include "game.h"

player::player() : entity(ET_PLAYER) {
	_flags |= EF_PLAYER;
	_firing = false;
	_reload_time = 0;
	_radius = 6.0f;
}

void player::init() {
}

void player::tick() {
	if (_reload_time > 0)
		_reload_time--;

	bool firing = (g_input.pad_buttons & PAD_A) || (length_sq(g_input.pad_right) > 0.1f);

	_firing = firing;

	_vel *= 0.85f;
	_vel += g_input.pad_left * 40.0f;

	entity_move_slide(this);

	if (firing) {
		if (_reload_time <= 0) {
			_reload_time = 8;

			vec2 pos = _pos;
			vec2 dir;

			if (length_sq(g_input.pad_right) > 0.1f)
				dir = normalise(g_input.pad_right);
			else
				dir = vec2(0.0f, 1.0f);

			for(int i = -1; i <= 1; i++) {
				bullet* b = (bullet*)spawn_entity(new bullet, pos);

				vec2 vel = dir * 300.0f;

				vel *= !i ? 1.0f : 0.99f;
				vel += perp(dir) * (20.0f * i);

				b->_vel		= vel;
				b->_rot		= rotation_of(vel);
			}
		}
	}
}

void player::draw(draw_context* dc) {
	dc->translate(_pos);

	rgba fill = rgba(1.0f, 1.0f, 1.0f, 0.0f);
	rgba outline = rgba(0.7f, 0.9f, 1.0f, 1.0f);

	dc->shape_outline(vec2(), 32, _radius, 0.0f, 0.5f, outline);
}