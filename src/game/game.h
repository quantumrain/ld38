#pragma once

#include "base.h"
#include "sys.h"
#include "tile_map.h"

#define DT (1.0f / 60.0f)

struct world;

enum entity_flag : u16 {
	EF_DESTROYED = 0x1
};

enum entity_type : u16 {
	ET_PLAYER
};

struct entity {
	entity(entity_type type);
	virtual ~entity();

	virtual void init();
	virtual void tick();
	virtual void draw(draw_context* dc);

	u16         _flags;
	entity_type _type;

	vec2 _pos;
	vec2 _old_pos;
	vec2 _vel;
	f32  _rot;
	f32  _radius;
	rgba _colour;
};

struct world {
	random r;

	list<entity> entities;

	vec2 view_size;
	mat44 view_proj;

	world();
};

extern world g_world;

entity* spawn_entity(entity* e, vec2 initial_pos);
void destroy_entity(entity* e);

int entity_move_slide(entity* e);

void world_tick();
void world_draw(draw_context* dc);

template<typename T> T* spawn_entity(T* e, vec2 initial_pos) {
	return (T*)spawn_entity(static_cast<entity*>(e), initial_pos);
}

template<typename F> void for_all(F&& f) {
	auto& entities = g_world.entities;

	for(int i = 0; i < entities.size(); i++) {
		entity* e = entities[i];

		if (!(e->_flags & EF_DESTROYED))
			f(e);
	}
}