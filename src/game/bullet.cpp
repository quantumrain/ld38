#include "pch.h"
#include "game.h"

bullet::bullet() : entity(ET_BULLET) {
	_flags |= EF_BULLET;
	_radius = 5.0f;
	_time = 60;
	_damp = 1.0f;
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

	if (entity* e = find_enemy_near_line(_old_pos, _pos, _radius)) {
		if (e->_type == ET_TRACKER) {
			tracker* t = (tracker*)e;

			t->_hurt = 1.0f;

			if (--t->_health <= 0) {
				fx_explosion(e->_pos, 0.25f, 3, rgba(1.0f, 0.0f, 0.0f, 0.0f), 0.5f);
				destroy_entity(e);
			}
		}

		destroy_entity(this);
	}
	//_rot = rotation_of(_vel);
}

void bullet::draw(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	dc->shape(vec2(), 3, _radius * 1.3f, 0.0f, rgba(1.0f, 1.0f));
}