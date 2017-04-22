#include "pch.h"
#include "game.h"

bullet::bullet() : entity(ET_BULLET) {
	_flags |= EF_BULLET;
	_radius = 6.0f;
	_time = 120;
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

	//_rot = rotation_of(_vel);
}

void bullet::draw(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	dc->shape(vec2(), 3, _radius, 0.0f, rgba(1.0f, 1.0f));
}