#include "pch.h"
#include "game.h"

player::player() : entity(ET_PLAYER) {
	_flags |= EF_PLAYER;
	_firing = false;
	_reload_time = 0;
	_radius = 5.0f;
}

void player::init() {
}

void player::tick() {
	if (_reload_time > 0)
		_reload_time--;

	vec2 key_move = g_input.pad_left;

	if (is_key_down(g_input.bind_left )) key_move.x -= 1.0f;
	if (is_key_down(g_input.bind_right)) key_move.x += 1.0f;
	if (is_key_down(g_input.bind_up   )) key_move.y -= 1.0f;
	if (is_key_down(g_input.bind_down )) key_move.y += 1.0f;

	if (is_key_down(KEY_LEFT )) key_move.x -= 1.0f;
	if (is_key_down(KEY_RIGHT)) key_move.x += 1.0f;
	if (is_key_down(KEY_UP   )) key_move.y -= 1.0f;
	if (is_key_down(KEY_DOWN )) key_move.y += 1.0f;

	if (length_sq(key_move) > 1.0f)
		key_move = normalise(key_move);

	key_move += to_vec2(g_input.mouse_rel) * 0.025f;

	bool firing = (g_input.pad_buttons & PAD_A) || (length_sq(g_input.pad_right) > 0.1f) || ((g_input.mouse_buttons & 1) != 0);

	_firing = firing;

	_vel *= 0.8f;
	_vel += key_move * 50.0f;

	if (length_sq(_vel) > square(1000.0f))
		_vel = normalise(_vel) * 1000.0f;

	if (length(key_move) > 0.001f) {
		float rate = clamp(length(key_move), 0.0f, 1.0f);

		_rot = normalise_radians(_rot + normalise_radians(rotation_of(_vel) - _rot) * 0.2f * rate);
	}

	planet* was_p = get_nearest_planet(_pos);

	if (was_p) {
		// TODO: consider tracking our current planet, so we only switch reference frames when we definitively leave a previous one
		//       maybe switch reference frame when we totally exit our previous one?
		//       maybe switch reference frame if we move towards the centre of a different one?
		_vel += was_p->_vel;
	}

	entity_move_slide(this);

	if (was_p) {
		_vel -= was_p->_vel;
	}

	if (planet* p = get_nearest_planet(_pos)) {
		if (was_p && (was_p != p)) {
			vec2 end_pt = was_p->get_exit_point(_old_pos, _pos, _radius);

			if (length_sq(end_pt - p->_pos) > square(p->_radius - _radius))
				p = was_p;
		}

		vec2  delta = _pos - p->_pos;
		float d     = p->_radius - _radius;
		float dist  = length(delta);

		if (dist > d) {
			delta /= dist;
			_pos = p->_pos + delta * d;
			_vel = cancel(_vel, delta);
		}
	}

	if (firing) {
		if (_reload_time <= 0) {
			_reload_time = 8;

			vec2 base_v;

			if (planet* p = get_nearest_planet(_pos))
				base_v = p->_vel;

			vec2 pos = _pos;
			vec2 dir = rotation(_rot);

			for(int i = -0; i <= 0; i++) {
				bullet* b = (bullet*)spawn_entity(new bullet, pos);

				vec2 vel = dir * 600.0f;

				vel *= !i ? 1.0f : 0.99f;
				vel += perp(dir) * (20.0f * i);

				b->_vel = vel + base_v;
				b->_rot = rotation_of(vel);
			}

			for(int i = -1; i <= 1; i++) {
				bullet* b = (bullet*)spawn_entity(new bullet, pos);

				vec2 vel = dir * 250.0f;

				vel *= !i ? 1.0f : 0.99f;
				vel += perp(dir) * (20.0f * i);

				b->_vel = -vel * g_world.r.range(2.0f, 2.5f) + perp(vel) * g_world.r.range(1.0f) + base_v;
				b->_rot = rotation_of(vel);
				b->_acc = vel * 0.25f;
				b->_damp = 0.975f;
				b->_radius = 3.0f;
			}
		}
	}
}

void player::draw(draw_context* dc) {
	float r = 7.0f;

	dc->translate(_pos);
	dc->rotate_z(_rot);

	rgba outline = rgba(0.7f, 0.9f, 1.0f, 1.0f);
	rgba black   = rgba(0.0f, 1.0f);

	dc->shape(vec2(2.0f, 0.0f), 3, r, 0.0f, outline);
	dc->shape(vec2(-2.0f, 0.0f), 3, r * 1.25f, 0.0f, outline);

	dc->shape(vec2(2.0f, 0.0f), 3, r - 2.0f, 0.0f, black);
	dc->shape(vec2(-2.0f, 0.0f), 3, r * 1.25f - 2.0f, 0.0f, black);

	dc->shape(vec2(-5.0f, -3.5f), 3, r * 0.25f, 0.0f, outline);
	dc->shape(vec2(-5.0f,  0.0f), 3, r * 0.55f, 0.0f, outline);
	dc->shape(vec2(-5.0f,  3.5f), 3, r * 0.25f, 0.0f, outline);

	dc->shape(vec2(-7.0f, -3.5f), 3, r * 0.25f, 0.0f, black);
	dc->shape(vec2(-7.0f,  0.0f), 3, r * 0.55f, 0.0f, black);
	dc->shape(vec2(-7.0f,  3.5f), 3, r * 0.25f, 0.0f, black);

	dc->shape(vec2(0.0f, 0.0f), 5, r * 0.5f, 0.0f, outline);
	dc->shape(vec2(0.0f, 0.0f), 5, r * 0.5f - 1.5f, 0.0f, black);
}