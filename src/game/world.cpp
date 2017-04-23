#include "pch.h"
#include "game.h"

world g_world;

// world

world::world() { }

// entity

entity::entity(entity_type type) : _flags(), _type(type), _rot(0.0f), _radius(8.0f) { }
entity::~entity() { }

void entity::init() { }
void entity::tick() { }
void entity::draw(draw_context* dc) { dc->shape_outline(_pos, 8, _radius, _rot, 0.5f, _colour); }

// etc

entity* spawn_entity(entity* e, vec2 initial_pos) {
	world* w = &g_world;

	if (e) {
		w->entities.push_back(e);

		e->_pos     = initial_pos;
		e->_old_pos = initial_pos;

		e->init();
	}

	return e;
}

void destroy_entity(entity* e) {
	if (e)
		e->_flags |= EF_DESTROYED;
}

entity* get_entity(entity_handle h) {
	world* w = &g_world;

	if (entity** ep = w->handles.at(h.index)) {
		if (entity* e = *ep) {
			if (e->_handle_internal.index == h.index)
				return e;
		}
	}

	return 0;
}

entity_handle get_entity_handle(entity* e) {
	world* w = &g_world;

	if (e) {
		if (get_entity(e->_handle_internal))
			return e->_handle_internal;

		for(int i = 0; i < w->handles.size(); i++) {
			if (!w->handles[i]) {
				w->handles[i] = e;
				return entity_handle(i);
			}
		}

		if (w->handles.push_back(e)) {
			e->_handle_internal = entity_handle(w->handles.size() - 1);
			return e->_handle_internal;
		}
	}

	return entity_handle();
}

void entity_prune() {
	world* w = &g_world;

	for(int i = 0; i < w->entities.size(); ) {
		if (w->entities[i]->_flags & EF_DESTROYED) {
			int h = w->entities[i]->_handle_internal.index;

			if (h < w->handles.size()) {
				assert(w->handles[h] == w->entities[i]);
				w->handles[h] = 0;
			}

			w->entities.swap_remove(i);
		}
		else
			i++;
	}
}

int entity_move_slide(entity* e) {
	int clipped = 0;

	e->_old_pos = e->_pos;
	e->_pos += e->_vel * DT;

	return clipped;
}

planet* get_nearest_planet(vec2 pos) {
	planet* best   = 0;
	float   best_d = FLT_MAX;

	for_all([&](entity* e) {
		if (e->_type == ET_PLANET) {
			float d = length_sq(e->_pos - pos);

			if (d < best_d) {
				best   = (planet*)e;
				best_d = d;
			}
		}
	});

	return best;
}

planet* get_nearest_planet_unique(vec2 pos) {
	planet* best = 0;

	for(auto& e : g_world.entities) {
		if ((!(e->_flags & EF_DESTROYED)) && (e->_type == ET_PLANET)) {
			float d = length_sq(e->_pos - pos);

			if (d < square(e->_radius)) {
				if (best)
					return 0;

				best = (planet*)e;
			}
		}
	}

	return best;
}

void world_tick() {
	world* w = &g_world;

	// update

	for_all([](entity* e) { e->tick(); });

	entity_prune();

	// camera

	vec2 player_pos;
	for_all([&](entity* e) { if (e->_type == ET_PLAYER) player_pos = e->_pos; });

	vec2  target_pos   = player_pos;
	vec2  target_delta = target_pos - w->camera_target;
	float target_dist  = length(target_delta);

	if (target_dist > 0.01f) {
		target_delta /= target_dist;

		float d = max(dot(target_pos - w->camera_old_target, target_delta), 0.0f);

		d *= square(square(clamp((target_dist - 0.0f) / 90.0f, 0.0f, 1.0f)));

		w->camera_target += target_delta * d;
	}

	w->camera_old_target = target_pos;

	vec2 camera_delta = (w->camera_target - w->camera_pos) * 0.1f;

	if (planet* p = get_nearest_planet(player_pos))
		camera_delta += p->_vel * DT;

	w->camera_pos += camera_delta;
	update_stars(camera_delta);

	// view

	w->view_proj = top_down_proj_view(-w->camera_pos, 90.0f, w->view_size.x / w->view_size.y, 400.0f, 1.0f, 1000.0f);
}

void world_draw(draw_context* dc) {
	render_stars(&dc->copy());

	for_all([dc](entity* e) { if (e->_type == ET_PLANET) ((planet*)e)->draw_bg(&dc->copy()); });
	for_all([dc](entity* e) { if (e->_type == ET_PLANET) ((planet*)e)->draw_mg(&dc->copy()); });
	for_all([dc](entity* e) { if (e->_type == ET_PLANET) ((planet*)e)->draw_mg2(&dc->copy()); });
	for_all([dc](entity* e) { if (e->_type == ET_TETHER) e->draw(&dc->copy()); });
	for_all([dc](entity* e) { if (e->_type == ET_PLANET) ((planet*)e)->draw_fg(&dc->copy()); });

	for_all([dc](entity* e) { if (!(e->_flags & (EF_PLAYER | EF_PLANET | EF_TETHER))) e->draw(&dc->copy()); });
	for_all([dc](entity* e) { if (e->_flags & EF_PLAYER) e->draw(&dc->copy()); });
}