#include "pch.h"
#include "game.h"

player::player() : entity(ET_PLAYER) {
	_flags |= EF_PLAYER;
	_firing = false;
	_reload_time = 0;
	_radius = 5.0f;
	_tether_shot_visible = 0.0f;
	_build_visible = 0.0f;
	_tether_reload = 60;
	_heal_tick = 0;
	_tether_ready = 0;
	_building = false;
	_shots = 0;
	_moves = 0.0f;
	_build_todo = BT_NONE;
	_heals_done = 0;
}

void player::init() {
}

void player::tick() {
	if (_reload_time > 0)
		_reload_time--;

	if (_heal_tick > 0)
		_heal_tick--;

	vec2 key_move = g_input.pad_left;

	if (is_key_down(g_input.bind_left )) key_move.x -= 1.0f;
	if (is_key_down(g_input.bind_right)) key_move.x += 1.0f;
	if (is_key_down(g_input.bind_up   )) key_move.y -= 1.0f;
	if (is_key_down(g_input.bind_down )) key_move.y += 1.0f;

	if (is_key_down(KEY_LEFT )) key_move.x -= 1.0f;
	if (is_key_down(KEY_RIGHT)) key_move.x += 1.0f;
	if (is_key_down(KEY_UP   )) key_move.y -= 1.0f;
	if (is_key_down(KEY_DOWN )) key_move.y += 1.0f;

	if (length_sq(key_move) > 1.0f)
		key_move = normalise(key_move);

	key_move += to_vec2(g_input.mouse_rel) * 0.025f;

	bool firing      = (g_input.pad_buttons & PAD_A) || ((g_input.mouse_buttons & 1) != 0) || is_key_down(KEY_Z);
	bool rmb_pressed = (g_input.pad_buttons_pressed & PAD_B) || ((g_input.mouse_buttons_pressed & 2) != 0) || is_key_pressed(KEY_X);
	bool rmb_down    = (g_input.pad_buttons & PAD_B) || ((g_input.mouse_buttons & 2) != 0) || is_key_down(KEY_X);

	if (firing && (g_world.game_time < 10))
		firing = false;

	_firing = firing;

	_vel *= 0.8f;

	if (!_building) {
		_vel += key_move * 50.0f;
		_moves += length(key_move);
	}

	if (length_sq(_vel) > square(1000.0f))
		_vel = normalise(_vel) * 1000.0f;

	if (length(key_move) > 0.001f) {
		float rate = clamp(length(key_move), 0.0f, 1.0f);

		_rot = normalise_radians(_rot + normalise_radians(rotation_of(_vel) - _rot) * 0.2f * rate);
	}

	planet* was_p = (planet*)get_entity(_last_planet);

	if (!was_p) {
		if (planet* p = get_nearest_planet(_pos)) {
			if (length_sq(p->_pos - _pos) < square(p->_radius + 40.0f))
				was_p = p;
		}

		if (!was_p) {
			g_world.player_dead_time = g_world.game_time;
			destroy_entity(this);
		}
	}

	if (was_p) {
		// TODO: consider tracking our current planet, so we only switch reference frames when we definitively leave a previous one
		//       maybe switch reference frame when we totally exit our previous one?
		//       maybe switch reference frame if we move towards the centre of a different one?
		_vel += was_p->_vel;
	}

	entity_move_slide(this);

	if (was_p) {
		_vel -= was_p->_vel;
	}

	if (planet* p = get_nearest_planet(_pos)) {
		if (was_p && (was_p != p)) {
			if (!p->_captured || !p->_grown) {
				p = was_p;
			}
			else {
				vec2 end_pt = was_p->get_exit_point(_old_pos, _pos, _radius);

				if (length_sq(end_pt - p->_pos) > square(p->_radius - _radius))
					p = was_p;
			}
		}

		vec2  delta = _pos - p->_pos;
		float d     = p->_radius - _radius;
		float dist  = length(delta);

		if (dist > d) {
			delta /= dist;
			_pos = p->_pos + delta * d;
			_vel = cancel(_vel, delta);
		}

		_last_planet = get_entity_handle(p);

		if (p->_health < MAX_PLANET_HEALTH) {
			if (_heal_tick <= 0) {
				_heal_tick = 10;
				p->_health = clamp(p->_health + 2, 0, MAX_PLANET_HEALTH);
				p->_healed = 1.0f;
				_heals_done++;
				sound_play(sfx::PLANET_HEAL, -20.0f, -10.0f);
				//fx_explosion(_pos, 0.5f, 1, rgba(0.1f, 0.6f, 0.1f, 0.0f), 0.5f);
			}
		}
	}

	if (firing) {
		if (_reload_time <= 0) {
			_reload_time = 8;
			_shots++;

			sound_play(sfx::PLAYER_FIRE, g_world.r.range(40.0f, 42.0f), -g_world.r.range(20.0f, 25.0f));
			sound_play(sfx::PLAYER_FIRE, -g_world.r.range(30.0f, 32.0f), -g_world.r.range(10.0f, 15.0f));

			vec2 base_v;

			if (planet* p = get_nearest_planet(_pos))
				base_v = p->_vel;

			vec2 pos = _pos;
			vec2 dir = rotation(_rot);

			for(int i = -0; i <= 0; i++) {
				bullet* b = (bullet*)spawn_entity(new bullet, pos);

				vec2 vel = dir * 600.0f;

				vel *= !i ? 1.0f : 0.99f;
				vel += perp(dir) * (20.0f * i);

				b->_vel = vel + base_v;
				b->_rot = rotation_of(vel);
				b->_radius = 5.0f;
				b->_colour = rgba(1.5f, 1.0f, 0.5f, 1.0f);
			}

			int num_gens = 0;

			for(auto& e : g_world.entities)
				if ((e->_type == ET_PLANET) && (((planet*)e)->_building == BT_GENERATOR))
					num_gens++;

			for(int i = 0; i < num_gens; i++) {
				bullet* b = (bullet*)spawn_entity(new bullet, pos);

				vec2 vel = dir * 250.0f;

				b->_vel = -vel * g_world.r.range(2.0f, 2.5f) + perp(vel) * g_world.r.range(1.0f) + base_v;
				b->_rot = rotation_of(vel);
				b->_acc = vel * 0.25f;
				b->_damp = 0.975f;
				b->_radius = 3.0f;
				b->_colour = rgba(1.5f, 0.8f, 0.5f, 1.0f);
				b->_missile = true;
			}
		}
	}

	bool tether_visible = false;
	bool build_visible = false;
	bool was_building = _building;

	_tether_ready *= 0.95f;
	_building = false;

	if (planet* p = get_nearest_planet_unique(_pos)) {
		if (p == was_p) {
			if (!p->_connector) {
				if (length_sq(p->_pos - _pos) > square(p->_radius - _radius * 4.0f)) {
					if (_tether_reload <= 0) {
						tether_visible = true;

						if (p->_tethered_to.size() < MAX_TETHERS) {
							if (_tether_shot_visible < 0.5f) {
								sound_play(sfx::PLAYER_TETHER_READY);

								if (_tether_ready < 0.1f)
									_tether_ready = 1.0f;
							}

							if (rmb_pressed) {
								_tether_reload = 60 * 15;
								planet* parts[10];

								int count = g_world.r.range(4, 8);

								float dd = p->_radius;

								for(int i = 0; i < count; i++) {
									float r = (i < (count - 1)) ? 23.0f : g_world.r.range(50.0f, 70.0f);

									dd += 1.0f;

									parts[i] = spawn_entity(new planet, p->_pos + normalise(_pos - p->_pos) * dd);

									parts[i]->_desired_radius = r;
									parts[i]->_connector = (i < (count - 1));
									parts[i]->_vel = vec2();

									if (i > 0)
										make_tether(parts[i - 1], parts[i]);
								}

								make_tether(p, parts[0]);

								sound_play(sfx::PLANET_GROW, -10.0f, -5.0f);
						
								g_world.num_planets_created++;
							}
						}
					}
				}
				else if (length_sq(p->_pos - _pos) < square(20.0f)) {
					if (p->_building == BT_NONE) {
						build_visible = true;

						p->_has_focus = 1.0f;

						if (_build_visible < 0.5f) {
							if (g_world.game_time > 10)
								sound_play(sfx::PLAYER_BUILD_READY, 0.0f, -5.0f);
						}

						if (was_building || rmb_pressed) {
							if (!was_building) {
								_build_move = 0.0f;
								_build_todo = BT_NONE;
							}

							_build_move += key_move;

							_build_move.x = clamp(_build_move.x, -10.0f, 10.0f);

							if (_build_move.x < -5.0f)
								_build_todo = BT_TURRET;
							else if (_build_move.x > 5.0f)
								_build_todo = BT_GENERATOR;
							else
								_build_todo = BT_NONE;

							_vel *= 0.5f;
							_building = rmb_down;

							if (!_building) {
								g_world.num_buildings_created++;
								p->_building = _build_todo;
								if (_build_todo == BT_TURRET)         sound_play(sfx::PLAYER_BUILD_TURRET);
								else if (_build_todo == BT_GENERATOR) sound_play(sfx::PLAYER_BUILD_GENERATOR);
							}
						}
					}
				}
			}
		}
	}

	if (_tether_reload > 0) {
		if (--_tether_reload <= 0)
		{
			sound_play(sfx::PLAYER_TETHER_READY, 0.0f, -5.0f);
			_tether_ready = 1.0f;
		}
	}

	_tether_shot_visible += ((tether_visible ? 1.0f : 0.0f) - _tether_shot_visible) * 0.2f;
	_build_visible += ((build_visible ? 1.0f : 0.0f) - _build_visible) * 0.2f;
}

void player::draw(draw_context* dc) {
	float r = 7.0f;

	draw_context dcc = dc->copy();

	dc->translate(_pos);
	dc->rotate_z(_rot);

#if 0
	rgba outline = rgba(0.7f, 0.9f, 1.0f, 1.0f);
	rgba black   = rgba(0.0f, 1.0f);

	dc->shape(vec2(2.0f, 0.0f), 3, r, 0.0f, outline);
	dc->shape(vec2(-2.0f, 0.0f), 3, r * 1.25f, 0.0f, outline);

	dc->shape(vec2(2.0f, 0.0f), 3, r - 2.0f, 0.0f, black);
	dc->shape(vec2(-2.0f, 0.0f), 3, r * 1.25f - 2.0f, 0.0f, black);

	dc->shape(vec2(-5.0f, -3.5f), 3, r * 0.25f, 0.0f, outline);
	dc->shape(vec2(-5.0f,  0.0f), 3, r * 0.55f, 0.0f, outline);
	dc->shape(vec2(-5.0f,  3.5f), 3, r * 0.25f, 0.0f, outline);

	dc->shape(vec2(-7.0f, -3.5f), 3, r * 0.25f, 0.0f, black);
	dc->shape(vec2(-7.0f,  0.0f), 3, r * 0.55f, 0.0f, black);
	dc->shape(vec2(-7.0f,  3.5f), 3, r * 0.25f, 0.0f, black);

	dc->shape(vec2(0.0f, 0.0f), 5, r * 0.5f, 0.0f, outline);
	dc->shape(vec2(0.0f, 0.0f), 5, r * 0.5f - 1.5f, 0.0f, black);
#else
	//dc->shape_outline(vec2(), 64, _radius, 0.0f, 0.15f, rgba());

	static float t;
	t+=DT * 15.0f;
	float a = lerp(0.75f, 1.0f, (1.0f + cosf(t + 0.0f)) * 0.5f);
	float b = lerp(0.75f, 1.0f, (1.0f + cosf(t + 2.0f)) * 0.5f);
	float c = lerp(0.75f, 1.0f, (1.0f + cosf(t + 4.0f)) * 0.5f);

	for(int i = 0; i < 3; i++) {
		dc->shape_outline(vec2(2.0f, 0.0f), 3, r * 1.00f * a, g_world.r.range(0.2f), 0.25f, rgba() * 1.50f);
		dc->shape_outline(vec2(1.0f, 0.0f), 4, r * 1.25f * b, g_world.r.range(0.2f), 0.25f, rgba() * 1.25f);
		dc->shape_outline(vec2(0.0f, 0.0f), 5, r * 1.00f * c, g_world.r.range(0.2f), 0.25f, rgba() * 1.00f);
	}

	if (_tether_ready > 0.01f) {
		dc->shape_outline(vec2(), 32, 8.0f * (1.0f - _tether_ready), 0.0f, 0.5f, rgba(1.0f, 1.0f, 1.0f, 0.0f) * _tether_ready);
		dc->shape_outline(vec2(), 32, 16.0f * (1.0f - _tether_ready), 0.0f, 0.5f, rgba(1.0f, 1.0f, 1.0f, 0.0f) * _tether_ready);
		dc->shape_outline(vec2(), 32, 32.0f * (1.0f - _tether_ready), 0.0f, 0.5f, rgba(1.0f, 1.0f, 1.0f, 0.0f) * _tether_ready);
	}
#endif

	if (planet* p = get_nearest_planet(_pos)) {
		float mr = p->_radius - _radius * 4.0f;

		if (length_sq(p->_pos - _pos) > square(mr)) {
			vec2 delta = normalise(_pos - p->_pos);

			float alpha = clamp((length(_pos - p->_pos) - mr) / (_radius * 2.0f), 0.0f, 1.0f);

			static float pulse;
			pulse += DT * 15.0f;

			vec2 ap = p->_pos + delta * (p->_radius + 10.0f + 4.0f * cosf(pulse));

			rgba c0 = rgba(1.3f, 0.2f, 1.3f, 0.0f);
			rgba c1 = rgba(0.3f, 0.1f, 0.3f, 0.0f);

			if (p->_tethered_to.size() >= MAX_TETHERS) {
				c0 = rgba(0.5f, 1.0f);
				c1 = rgba(0.2f, 1.0f);
			}

			dcc.shape_outline(ap, 3, 8.0f, rotation_of(delta), 0.5f, c0 * _tether_shot_visible * alpha);
			dcc.shape_outline(ap, 3, 16.0f, rotation_of(delta), 0.5f, c1 * _tether_shot_visible * alpha);
		}
	}

	if (_building) {
		if (planet* p = (planet*)get_entity(_last_planet)) {
			dcc.translate(p->_pos);

			dcc.shape(vec2(), 64, 32.0f, 0.0f, rgba(0.25f, 0.75f));
			dcc.shape_outline(vec2(), 64, 32.0f, 0.0f, 0.5f, rgba());

			float build_turret    = (_build_todo == BT_TURRET   ) ? 1.0f : 0.0f;
			float build_generator = (_build_todo == BT_GENERATOR) ? 1.0f : 0.0f;

			float disable_turret    = ((_build_todo != BT_NONE) && (_build_todo != BT_GENERATOR)) ? 1.0f : 0.3f;
			float disable_generator = ((_build_todo != BT_NONE) && (_build_todo != BT_TURRET   )) ? 1.0f : 0.3f;

			dcc.shape(vec2(10.0f, 0.0f), 3, 16.0f, 0.0f, rgba(build_generator, 0.0f));
			dcc.shape(vec2(-10.0f, 0.0f), 3, 16.0f, -PI, rgba(build_turret, 0.0f));

			dcc.shape_outline(vec2(10.0f, 0.0f), 3, 16.0f, 0.0f, 0.5f, rgba());
			dcc.shape_outline(vec2(-10.0f, 0.0f), 3, 16.0f, -PI, 0.5f, rgba());

			dcc.line(vec2(-32.0f, 0.0f), vec2(-48.0f, 0.0f), 0.5f, rgba() * disable_turret);
			dcc.shape(vec2(-80.0f, 0.0f), 64, 32.0f, 0.0f, rgba(0.5f, 0.1f, 0.1f, 0.75f) * disable_turret);
			dcc.shape_outline(vec2(-80.0f, 0.0f), 64, 32.0f, 0.0f, 0.5f, rgba() * disable_turret);

			dcc.line(vec2(32.0f, 0.0f), vec2(48.0f, 0.0f), 0.5f, rgba() * disable_generator);
			dcc.shape(vec2(80.0f, 0.0f), 64, 32.0f, 0.0f, rgba(0.1f, 0.5f, 0.1f, 0.75f) * disable_generator);
			dcc.shape_outline(vec2(80.0f, 0.0f), 64, 32.0f, 0.0f, 0.5f, rgba() * disable_generator);

			draw_string(dcc, vec2(-80.0f, 0.0f), vec2(0.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba() * disable_turret, "TURRET");
			draw_string(dcc, vec2(80.0f, 0.0f), vec2(0.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba() * disable_generator, "GENERATOR");
		}
	}
}