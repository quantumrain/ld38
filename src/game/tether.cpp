#include "pch.h"
#include "game.h"

tether::tether() : entity(ET_TETHER) {
	_flags |= EF_TETHER;
	_radius = 0.0f;
}

void tether::init() {
}

void tether::tick() {
	planet* from = (planet*)get_entity(_from);
	planet* to   = (planet*)get_entity(_to);

	if (!from || !to)
		destroy_entity(this);

	vec2  delta = from->_pos - to->_pos;
	float dist  = length(delta);
	float d     = planet_radius(from, to) + planet_radius(to, from);

	if (dist > d) {
		delta /= dist;

		float f = (dist - d) * 0.5f;

		from->_vel -= delta * f;
		to  ->_vel += delta * f;

		vec2 mix = from->_vel + to->_vel;
		
		vec2 fv = from->_vel;
		vec2 tv = to  ->_vel;

		from->_vel += tv * 0.01f;
		to  ->_vel += fv * 0.01f;

		from->_vel *= 0.99f;
		to  ->_vel *= 0.99f;
	}
}

void tether::draw(draw_context* dc) {
	planet* from = (planet*)get_entity(_from);
	planet* to   = (planet*)get_entity(_to);

	if (from && to) {
		rgba c0 = rgba(0.5f, 0.0f, 0.6f, 0.5f) * from->_pulse * 2.75f;
		rgba c1 = rgba(0.5f, 0.0f, 0.6f, 0.5f) * to  ->_pulse * 2.75f;

		dc->line(from->_pos, to->_pos, 5.0f, c0, c1);
		dc->line(from->_pos, to->_pos, 4.0f, rgba(0.0f, 1.0f), rgba(0.0f, 1.0f));
	}
}

bool has_tether(entity* a, entity* b) {
	for(auto& e : g_world.entities) {
		if (e->_type == ET_TETHER) {
			tether* t = (tether*)e;

			entity* p = get_entity(t->_from);
			entity* q = get_entity(t->_to);

			if (((p == a) && (q == b)) || ((p == b) && (q == a)))
				return true;
		}
	}

	return false;
}