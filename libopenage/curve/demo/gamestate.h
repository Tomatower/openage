// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#pragma once

#include "config.h"

#include "../object.h"
#include "../continuous.h"
#include "../discrete.h"
#include "../../util/vector.h"

namespace openage {
namespace curvepong {

struct event {
	int player;
	enum state_e {
		UP, DOWN, START, IDLE, LOST
	} state;
	event(int id, state_e s) : player(id), state(s) {}
	event() : player(0), state(IDLE) {}
};

class PongPlayer : public curve::Object {
public:
	PongPlayer(openage::curve::TriggerFactory *f) :
		Object(f),
		speed(f),
		position(f),
		lives(f),
		state(f),
		size(f),
		y(0),
		id(0) {}

	curve::Discrete<float> speed;
	curve::Continuous<float> position;
	curve::Discrete<int> lives;
	curve::Discrete<event> state;
	curve::Discrete<float> size;
	float y;
	int id;
};

class PongBall : public curve::Object {
public:
	PongBall(curve::TriggerFactory *f) :
		Object(f),
		speed(this),
		position(this) {}
	curve::Discrete<util::Vector<2>> speed;
	curve::Continuous<util::Vector<2>> position;
};

class PongState : public curve::Object {
public:
	PongState(curve::TriggerFactory *f) :
		Object(f),
		p1(f),
		p2(f),
		ball(f) {}

	PongPlayer p1;
	PongPlayer p2;

	PongBall ball;

	util::Vector<2> resolution;
};

}} // openage::curvepong
