#include "pch.h"
#include "game.h"

entity* get_nearest_enemy(vec2 pos) {
	entity* best   = 0;
	float   best_d = FLT_MAX;

	for(auto& e : g_world.entities) {
		if ((e->_flags & EF_DESTROYED) || !(e->_flags & EF_ENEMY))
			continue;

		float d = length_sq(e->_pos - pos);

		if (d < best_d) {
			best   = e;
			best_d = d;
		}
	}

	return best;
}

vec2 get_center(entity_type type) {
	vec2 centre;
	int count = 1;

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
	_radius = 3.0f;
	_captured = false;
	_connector = false;
	_grown = false;
	_pulse = g_world.r.rand(1.0f);
	_pulse_t = g_world.r.range(PI);
	_hurt = 0.0f;
	_healed = 0.0f;
	_health = MAX_PLANET_HEALTH;
	_has_focus = 0.0f;
	_turret_tick = 0;
	_building = BT_NONE;

	_vel = g_world.r.range(vec2(100.0f));
}

void planet::init() {
}

void planet::tick() {
	if (length_sq(_vel) > square(300.0f))
		_vel *= 0.99f;

	_radius += (_desired_radius - _radius) * 0.01f;

	if (_radius > (_desired_radius * 0.6f))
		_grown = true;

	_wander *= 0.95f;
	_wander += g_world.r.range(vec2(0.5f));

	if ((g_world.num_planets_created > 0) && (_tethered_to.size() == 0))
		_vel *= 0.95f;

	_vel += _wander;
	_vel += (get_center(_type) - _pos) * 0.0001f;

	_hurt *= 0.8f;
	_healed *= 0.9f;
	_has_focus *= 0.8f;

	for(int i = 0; i < _tethered_to.size(); ) {
		if (!get_entity(_tethered_to[i])) {
			_tethered_to.swap_remove(i);
		}
		else
			i++;
	}

	if (_building == BT_TURRET) {
		_turret_tick--;

		if (_turret_tick < 0) {
			int num_gens = 0;

			for(auto& e : g_world.entities)
				if ((e->_type == ET_PLANET) && (((planet*)e)->_building == BT_GENERATOR))
					num_gens++;

			_turret_tick = 15 - num_gens;

			if (entity* e = get_nearest_enemy(_pos)) {
				vec2  delta = e->_pos - _pos;
				float dist  = length(delta);

				if (dist < 200.0f) {
					vec2 dir = delta / dist;

					for(int i = 0; i < 1; i++) {
						bullet* b = (bullet*)spawn_entity(new bullet, _pos);

						vec2 vel = dir * 250.0f;

						b->_vel = perp(vel) * g_world.r.range(0.25f) + _vel;
						b->_rot = rotation_of(vel);
						b->_acc = vel * 0.25f;
						b->_damp = 0.975f;
						b->_radius = 3.0f;
						b->_colour = rgba(1.5f, 0.8f, 0.5f, 1.0f);
						b->_missile = true;
					}
				}
			}
		}
	}

	if (_connector) {
		if (g_world.r.chance(1, 30)) {
			if (_tethered_to.size() <= 1) {
				fx_explosion(_pos, sqrtf(_radius) * 0.1f, 10, rgba(0.1f, 0.1f, 1.0f, 0.0f), 0.7f);
				sound_play(sfx::PLANET_DIE, g_world.r.range(2.0f), g_world.r.range(-5.0f, -8.0f));
				destroy_entity(this);
			}
		}
	}

	for(auto e : g_world.entities) {
		if ((e == this) || (e->_flags & EF_DESTROYED) || (e->_type != ET_PLANET))
			continue;

		vec2  delta  = _pos - e->_pos;
		float sr     = planet_radius(this, (planet*)e);
		float er     = planet_radius((planet*)e, this);
		float cr     = sr + er;

		//debug_circle(_pos, sr, rgba(0.5, 0.0, 0.0, 0.5), 1);
		//debug_circle(e->_pos, er, rgba(0.0, 0.5, 0.0, 0.5), 1);

		if (length_sq(delta) > square(cr))
			continue;

		float d = length(delta);

		if (d < 0.001f)
			delta = g_world.r.range(vec2(1.0f));
		else
			delta /= d;

		float f = (cr - d) * 0.5f;

		//debug_line(_pos, e->_pos, rgba(), 1);
		//debug_line(_pos, _pos + delta*f, rgba(1,0,0,1), 1);
		//debug_line(e->_pos, e->_pos - delta*f, rgba(0,1,0,1), 1);

		_vel    += delta * f;
		e->_vel -= delta * f;

		if (_captured && ((planet*)e)->_captured) {
			if (d < (cr * 0.8f)) {
				if (dot(   _vel, -delta) > 0.0f) {    _vel -= -delta * dot(   _vel, -delta); }
				if (dot(e->_vel,  delta) > 0.0f) { e->_vel -=  delta * dot(e->_vel,  delta); }
			}
		}
	}

	entity_move_slide(this);

	_pulse_t += DT * 5.0f;
	_pulse = 0.1f + (1.0f + cosf(_pulse_t)) * 0.5f;
}

void planet::draw_bg(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	float f = clamp(1.0f - (_health / (float)MAX_PLANET_HEALTH), 0.0f, 1.0f);

	rgba c = lerp(rgba(0.0f, 0.0f, 1.0f, 0.0f), rgba(1.0f, 0.0f, 0.0f, 0.0f), f);

	dc->shape(vec2(), 64, _radius + 2.0f, 0.0f, c * 0.075f);
	dc->shape(vec2(), 64, _radius + 0.0f, 0.0f, c * 0.15f);
	dc->shape(vec2(), 64, _radius - 2.0f, 0.0f, c * 0.3f);
	dc->shape(vec2(), 64, _radius - 4.0f, 0.0f, lerp(rgba(0.5f, 0.3f, 1.6f, 0.0f), rgba(1.0f, 0.3f, 0.1f, 0.0f), f) + rgba(2.0f, 0.0f) * _hurt + rgba(0.0f, 1.0f, 0.0f, 0.0f) * _healed);
}

void planet::draw_mg(draw_context* dc) {
	dc->translate(_pos);
	dc->rotate_z(_rot);

	dc->shape(vec2(), 64, _radius - 5.0f, 0.0f, rgba(0.0f, 0.0f, 0.0f, 1.0f));
}

void planet::draw_mg2(draw_context* dc) {
	if (_captured) {
		dc->translate(_pos);
		dc->rotate_z(_rot);

		dc->scale(clamp(_radius / _desired_radius, 0.0f, 1.0f));

		if (!_connector) {
			dc->shape_outline(vec2(), 64, 20.0f, 0.0f, 0.5f, rgba(0.5f, 0.0f, 0.5f, 1.0f) * 0.5f * (0.5f + _pulse * 0.5f) + rgba(1.0f, 0.0f) * _has_focus);
		}
		else {
			dc->shape(vec2(), 64, 5.0f, 0.0f, rgba(0.5f, 0.0f, 0.6f, 0.5f) * _pulse * 2.75f);
		}
	}
}

void planet::draw_fg(draw_context* dc) {
	if (_captured) {
		dc->translate(_pos);
		dc->rotate_z(_rot);

		dc->scale(clamp(_radius / _desired_radius, 0.0f, 1.0f));

		if (_connector) {
			dc->shape(vec2(), 64, 4.0f, 0.0f, rgba(0.0f, 1.0f));
		}
		else {
			dc->shape(vec2(), 64, 15.0f, 0.0f, rgba(0.1f, 0.0f, 0.1f, 1.0f));

			if (_building == BT_NONE) {
				dc->shape_outline(vec2(), 64, 5.0f, 0.0f, 0.5f, rgba(0.5f, 0.0f, 0.6f, 0.5f) * (1.2f - _pulse) * 2.75f);
			}

			dc->shape_outline(vec2(), 64, 15.0f, 0.0f, 0.5f, rgba(0.5f, 0.0f, 0.6f, 0.5f) * _pulse * 2.75f);

			if (_building == BT_GENERATOR) {
				float s0 = lerp(13.0f, 15.0f, _pulse);
				float s1 = lerp(15.0f, 14.0f, _pulse);

				dc->shape(vec2(), 4, s0, PI * 0.0f, rgba(0.0f, 0.2f, 0.0f, 1.0f));
				dc->shape(vec2(), 4, s1, PI * 0.25f, rgba(0.0f, 0.2f, 0.0f, 1.0f));

				dc->shape_outline(vec2(), 4, s0, PI * 0.0f, 0.5f, rgba(0.4f, 1.5f, 0.2f, 0.0f));
				dc->shape_outline(vec2(), 4, s1, PI * 0.25f, 0.5f, rgba(0.4f, 1.5f, 0.2f, 0.0f));

				dc->shape_outline(vec2(), 16, lerp(5.0f, 10.0f, _pulse), 0.0f, 0.5f, rgba(0.4f, 1.0f, 0.2f, 0.0f));
				dc->shape_outline(vec2(), 16, lerp(10.0f, 2.0f, _pulse), 0.0f, 0.5f, rgba(0.4f, 1.0f, 0.2f, 0.0f));
				dc->shape_outline(vec2(), 16, lerp(1.0f, 3.0f, _pulse), 0.0f, 0.5f, rgba(0.4f, 1.0f, 0.2f, 0.0f));
			}
			else if (_building == BT_TURRET) {
				float s0 = lerp(13.0f, 15.0f, _pulse) * 1.1f;
				float s1 = lerp(15.0f, 14.0f, _pulse) * 1.1f;

				dc->shape(vec2(), 3, s0, PI * 0.0f, rgba(0.2f, 0.0f, 0.0f, 1.0f));
				dc->shape(vec2(), 3, s1, PI * 0.33f, rgba(0.2f, 0.0f, 0.0f, 1.0f));

				dc->shape_outline(vec2(), 3, s0, PI * 0.0f, 0.5f, rgba(1.5f, 0.33f, 0.1f, 1.0f));
				dc->shape_outline(vec2(), 3, s1, PI * 0.33f, 0.5f, rgba(1.5f, 0.33f, 0.1f, 1.0f));

				for(int i = 0; i < 4; i++) {
					dc->copy().rotate_z(TAU * i / 4.0f + _pulse * TAU).shape_outline(vec2(3.0f, 0.0f), 3, 3.0f, 0.0f, 0.5f, rgba(1.5f, 0.33f, 0.1f, 1.0f));
				}
			}
		}
	}
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

void planet::take_hit() {
	_hurt = 1.0f;
	_health--;

	g_world.num_planets_hurt++;

	if (_health < 0) {
		fx_explosion(_pos, sqrtf(_radius) * 0.2f, _connector ? 10 : 50, rgba(0.1f, 0.1f, 1.0f, 0.0f), _connector ? 0.7f : 1.5f);
		sound_play(sfx::PLANET_DIE, g_world.r.range(2.0f), g_world.r.range(-5.0f, -8.0f));
		destroy_entity(this);
	}
	else
		sound_play(sfx::PLANET_HURT, g_world.r.range(2.0f), g_world.r.range(-10.0f, -15.0f));
}

float planet_radius(planet* self, planet* other) {
	float r = self->_radius;

	for(auto& h : self->_tethered_to) {
		if (get_entity(h) == other) {
			if (self->_connector)
				return 1.0f;

			return r - 10.0f;
		}
	}

	return r + (!self->_connector ? 3.0f : 0.0f);
}