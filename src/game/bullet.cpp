#include "pch.h"
#include "game.h"

bullet::bullet() : entity(ET_BULLET) {
	_flags |= EF_BULLET;
	_radius = 3.0f;
	_time = 120;
}

void bullet::init() {
}

void bullet::tick() {
	if (--_time <= 0)
		destroy_entity(this);

	if (entity_move_slide(this))
		destroy_entity(this);

	//_rot = rotation_of(_vel);
}

void bullet::draw(draw_context* dc) {
	float r = 2.5f;

	dc->translate(_pos);

	dc->shape(vec2(), 32, r, 0.0f, rgba(1.0f, 1.0f));
}