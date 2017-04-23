#include "pch.h"
#include "game.h"

bullet::bullet() : entity(ET_BULLET) {
	_flags |= EF_BULLET;
	_radius = 5.0f;
	_time = 60;
	_damp = 1.0f;
	_missile = false;
}

void bullet::init() {
}

void bullet::tick() {
	if (--_time <= 0)
		destroy_entity(this);

	_vel *= _damp;
	_vel += _acc;

	if (entity_move_slide(this))
		destroy_entity(this);

	if (_missile) {
		for(int i = 0; i < 4; i++) {
			float f = i / 3.0f;
			psys_spawn(lerp(_old_pos, _pos, f) + g_world.r.range(vec2(1.0f)), vec2(), 3.0f, 1.0f, 1.0f, 0.5f, rgba(0.5f, 0.0f), 10);
		}
	}

	if (entity* e = find_enemy_near_line(_old_pos, _pos, _radius)) {
		if (e->_type == ET_TRACKER) {
			tracker* t = (tracker*)e;

			t->_hurt = 1.0f;
			t->_health -= _missile ? 1 : 2;

			if (t->_health <= 0) {
				fx_explosion(e->_pos, 0.25f, 3, rgba(1.0f, 0.0f, 0.0f, 0.0f), 0.5f);
				sound_play(sfx::TRACKER_DIE, g_world.r.range(2.0f), g_world.r.range(2.0f));
				destroy_entity(e);
			}
			else
				sound_play(sfx::TRACKER_DIE, g_world.r.range(-10.0f, -12.0f), g_world.r.range(-4.0f, -6.0f));
		}

		destroy_entity(this);
	}
	//_rot = rotation_of(_vel);
}

void bullet::draw(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	if (_missile) {
		dc->scale_xy(1.0f, 0.5f);
		dc->shape_outline(vec2(), 3, _radius * 1.3f, 0.0f, 0.5f, _colour);
	}
	else {
		dc->shape_outline(vec2(), 4, _radius * 0.8f, 0.0f, 0.5f, _colour);
		dc->shape_outline(vec2(), 4, _radius * 0.8f, PI * 0.25f, 0.5f, _colour);
	}
}