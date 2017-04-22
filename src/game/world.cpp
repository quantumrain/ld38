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

int entity_move_slide(entity* e) {
	int clipped = 0;

	e->_old_pos = e->_pos;
	e->_pos += e->_vel * DT;

	return clipped;
}

void world_tick() {
	world* w = &g_world;

	for_all([](entity* e) { e->tick(); });

	w->view_proj = top_down_proj_view(vec2(), 90.0f, w->view_size.x / w->view_size.y, 360.0f, 1.0f, 1000.0f);

	for(int i = 0; i < w->entities.size(); ) {
		if (w->entities[i]->_flags & EF_DESTROYED)
			w->entities.swap_remove(i);
		else
			i++;
	}
}

void world_draw(draw_context* dc) {

	for_all([dc](entity* e) { if (!(e->_flags & EF_PLAYER)) e->draw(&dc->copy()); });
	for_all([dc](entity* e) { if (e->_flags & EF_PLAYER) e->draw(&dc->copy()); });

}