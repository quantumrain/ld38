#include "pch.h"
#include "game.h"

#define STAR_W 400.0f
#define STAR_H 200.0f

struct star {
	vec2 pos;
	float z;
};

array<star> g_stars;

void init_stars() {
	for(int i = 0; i < 1500; i++) {
		g_stars.push_back(star{ g_world.r.range(vec2(STAR_W, STAR_H)), square(g_world.r.range(0.0f, 1.0f)) });
	}
}

void update_stars(vec2 delta) {
	for(auto& star : g_stars) {
		star.pos -= delta * star.z * 0.75f;

		if (star.pos.x < -STAR_W) star.pos.x += STAR_W * 2.0f;
		if (star.pos.x > +STAR_W) star.pos.x -= STAR_W * 2.0f;
		if (star.pos.y < -STAR_H) star.pos.y += STAR_H * 2.0f;
		if (star.pos.y > +STAR_H) star.pos.y -= STAR_H * 2.0f;
	}
}

void render_stars(draw_context* dc) {
	dc->translate(g_world.camera_pos);

	for(auto& star : g_stars) {
		float z = square(star.z);
		float s = lerp(0.25f, 0.65f, z);
		float c = lerp(0.35f, 0.85f, square(z)) * 0.5f;

		dc->shape(star.pos, 4, s*1, g_world.r.range(PI), rgba(c * 0.5f, c * 0.5f, c, 0.0f)*2.5f);
		dc->shape(star.pos, 4, s*2, g_world.r.range(PI), rgba(c * 0.5f, c * 0.5f, c, 0.0f));
		dc->shape(star.pos, 4, s*5, g_world.r.range(PI), rgba(c * 0.5f, c * 0.5f, c, 0.0f)*.2f);
		//dc->rect(star.pos - s, star.pos + s, rgba(c * 0.5f, c * 0.5f, c, 0.0f));
	}
}