#include "pch.h"
#include "game.h"

void avoid_crowd(entity* self) {
	for(auto& e : g_world.entities) {
		if ((e == self) || (e->_flags & (EF_DESTROYED)))// || (e->_type != self->_type))
			continue;

		vec2  delta           = self->_pos    - e->_pos;
		float combined_radius = self->_radius + e->_radius;

		if (length_sq(delta) > square(combined_radius))
			continue;

		float dist = length(delta);

		if (dist < 0.0001f)
			delta = vec2(1.0f, 0.0f);
		else
			delta /= dist;

		float fs = 2.0f + (combined_radius - dist) * 2.0f;
		float fe = 2.0f + (combined_radius - dist) * 2.0f;

		if (e->_type == ET_PLANET)
			fe *= 0.1f;

		self->_vel += delta * fs;
		e->_vel -= delta * fe;
	}
}

tracker::tracker() : entity(ET_TRACKER) {
	_flags |= EF_ENEMY;
	_radius = 6.0f;
	_hit_timer = 0;
	_health = 2;
	_hurt = 0.0f;
}

void tracker::init() {
}

void tracker::tick() {
	_vel *= 0.95f;

	if (_hit_timer > 0)
		_hit_timer--;

	_hurt *= 0.8f;

	if (planet* p = get_nearest_planet(_pos)) {
		vec2  delta = p->_pos - _pos;
		float dist  = length(delta);
		float r     = p->_radius + _radius;

		_vel += delta * (10.0f / dist);

		//p->_vel += g_world.r.range(vec2(1.0f)) - delta * 0.001f;

		if (dist < r) {
			_pos = p->_pos - delta * (r / dist);

			if (_hit_timer <= 0) {
				p->take_hit();
				_hit_timer = 60;
			}
		}
	}

	avoid_crowd(this);

	entity_move_slide(this);
}

void tracker::draw(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	for(int i = 0; i < 3; i++) {
		dc->shape_outline(vec2(), 3, _radius, g_world.r.range(PI), 0.5f, rgba(1.0f, 0.2f, 0.2f, 0.0f) + rgba(1.0f, 0.0f) * _hurt);
	}
}