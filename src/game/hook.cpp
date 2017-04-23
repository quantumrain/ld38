#include "pch.h"
#include "game.h"

hook::hook() : entity(ET_HOOK) {
	_flags |= EF_HOOK;
	_radius = 8.0f;
	_time = 15;
}

void hook::init() {
}

void hook::tick() {
	if (--_time <= 0)
		destroy_entity(this);

	_vel *= 0.95f;

	if (entity_move_slide(this))
		destroy_entity(this);

	planet* from = (planet*)get_entity(_from);

	if (!from) {
		destroy_entity(this);
		return;
	}

	for(auto& e : g_world.entities) {
		if (!(e->_flags & EF_DESTROYED) && (e->_type == ET_PLANET) && (from != e)) {
			planet* other = (planet*)e;

			if (other->_captured)
				continue;

			if (length_sq(_pos - other->_pos) < square(other->_radius + _radius)) {

				if (!has_tether(from, e)) {
					if ((from->_tethered_to.size() < 3) && (other->_tethered_to.size() < 3)) {
						if (tether* t = spawn_entity(new tether, vec2())) {
							t->_from = get_entity_handle(from);
							t->_to   = get_entity_handle(other);

							from ->_captured = true;
							other->_captured = true;

							from ->_tethered_to.push_back(get_entity_handle(other));
							other->_tethered_to.push_back(get_entity_handle(from ));
						}
					}
				}

				destroy_entity(this);

				break;
			}
		}
	}

	/*if (was_p && p && (was_p != p)) {
		
	}*/
}

void hook::draw(draw_context* dc) {
	entity* from = get_entity(_from);

	if (!from)
		return;

	for(int i = 0; i < 3; i++) {
		dc->shape_outline(_pos, 3, 10 + i * 5.0f, g_world.r.range(PI), 0.5f, rgba(1.5f, 0.0f, 1.5f, 1.0f) * (1.0f - i * 0.4f));
	}

	dc->line(from->_pos, _pos, 5.0f, rgba(1.5f, 0.0f, 1.5f, 1.0f));
	dc->shape(from->_pos, 16, 5.0f, 0.0f, rgba(1.5f, 0.0f, 1.5f, 1.0f));
	dc->shape(_pos, 16, 5.0f, 0.0f, rgba(1.5f, 0.0f, 1.5f, 1.0f));

	dc->line(from->_pos, _pos, 4.0f, rgba(0.0f, 1.0f));
	dc->shape(from->_pos, 16, 4.0f, 0.0f, rgba(0.0f, 1.0f));
	dc->shape(_pos, 16, 4.0f, 0.0f, rgba(0.0f, 1.0f));

	dc->line(from->_pos, _pos, 0.5f, rgba(2.0f, 0.5f, 2.0f, 1.0f));
}