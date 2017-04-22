#include "pch.h"
#include "game.h"

void avoid_crowd(entity* self, entity_type type, float radius_factor) {
	for(auto e : g_world.entities) {
		if ((e == self) || (e->_flags & EF_DESTROYED) || (e->_type != type))
			continue;

		vec2  d     = self->_pos - e->_pos;
		float min_d = (self->_radius + e->_radius) * radius_factor;

		if (length_sq(d) > square(min_d))
			continue;

		float l = length(d);

		if (l < 0.001f)
			d = g_world.r.range(vec2(1.0f));
		else
			d /= l;

		self->_vel += d * 8.0f;
		e->_vel    -= d * 8.0f;
	}
}

vec2 get_center(entity_type type) {
	vec2 centre;
	int count = 0;

	for(auto e : g_world.entities) {
		if ((e->_flags & EF_DESTROYED) || (e->_type != type))
			continue;

		centre += e->_pos;
		count++;
	}

	if (count > 0)
		centre /= (float)count;

	return centre;
}

planet::planet() : entity(ET_PLANET) {
	_flags |= EF_PLANET;
	_radius = 50.0f;

	_vel = g_world.r.range(vec2(100.0f));
}

void planet::init() {
}

void planet::tick() {
	if (length_sq(_vel) > square(300.0f))
		_vel *= 0.99f;

	_vel += (get_center(_type) - _pos) * 0.001f;

	avoid_crowd(this, _type, 0.66f);

	entity_move_slide(this);
}

void planet::draw_bg(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	dc->shape(vec2(), 64, _radius + 2.0f, 0.0f, rgba(0.0f, 0.0f, 0.075f, 0.0f));
	dc->shape(vec2(), 64, _radius + 0.0f, 0.0f, rgba(0.0f, 0.0f, 0.15f, 0.0f));
	dc->shape(vec2(), 64, _radius - 2.0f, 0.0f, rgba(0.0f, 0.0f, 0.3f, 0.0f));
	dc->shape(vec2(), 64, _radius - 4.0f, 0.0f, rgba(1.4f, 0.3f, 1.6f, 0.0f));
}

void planet::draw_mg(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	dc->shape(vec2(), 64, _radius - 5.0f, 0.0f, rgba(0.0f, 0.0f, 0.0f, 1.0f));
}

void planet::draw_fg(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	dc->shape_outline(vec2(), 64, 5.0f, 0.0f, 0.5f, rgba(1.4f, 0.3f, 1.6f, 0.0f) * 0.1f);
	dc->shape_outline(vec2(), 64, 10.0f, 0.0f, 0.5f, rgba(1.4f, 0.3f, 1.6f, 0.0f) * 0.08f);
	dc->shape_outline(vec2(), 64, 15.0f, 0.0f, 0.5f, rgba(1.4f, 0.3f, 1.6f, 0.0f) * 0.06f);
}

vec2 planet::get_exit_point(vec2 start, vec2 end, float point_radius) {
	vec2 pos_to_start = start - _pos;
	vec2 pos_to_end   = end - _pos; 

	float min_radius = _radius - point_radius;

	if (length_sq(pos_to_start) > min_radius)
		return _pos + normalise(pos_to_start) * min_radius;

	if (length_sq(pos_to_end) <= min_radius)
		return _pos + normalise(pos_to_end) * min_radius;

	vec2  ray_dir = end - start;
	float a       = length_sq(ray_dir);

	if (a < 0.0001f)
		return start;

	float b = dot(pos_to_start, ray_dir) * 2.0f;
	float c = length_sq(pos_to_start) - square(min_radius);
	float d = (b * b) - (4.0f * a * c);

	if (d < 0.0f)
		return end;

	d = sqrtf(d);

	float t1 = (-b - d) / (2.0f * a);
	float t2 = (-b + d) / (2.0f * a);

	if ((t1 >= 0.0f) && (t1 <= 1.0f))
		return start + ray_dir * t1;

	if ((t2 >= 0.0f) && (t2 <= 1.0f))
		return start + ray_dir * t2;

	return end;
}