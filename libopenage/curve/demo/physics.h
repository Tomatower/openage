// Copyright 2015-2017 the openage authors. See copying.md for legal info.

#pragma once

#include "config.h"
#include "gamestate.h"

#include "../curve.h"
#include "../events.h"

#include <vector>

namespace openage {
namespace curvepong {

class Physics {
public:

	static void init(PongState &, curve::EventQueue *, const curve::curve_time_t &);
	void processInput(PongState &, PongPlayer &, std::vector<event> &input, curve::EventQueue *, const curve::curve_time_t &now);
protected:

	static void reset(PongState &, curve::EventQueue *, const curve::curve_time_t &);


	static void ball_reflect_wall(PongState &, curve::EventQueue *, const curve::curve_time_t &);
	static void ball_reflect_panel(PongState &, curve::EventQueue *, const curve::curve_time_t &);
	static curve::curve_time_t predict_reflect_wall(PongState &, curve::EventQueue *, const curve::curve_time_t &);
	static curve::curve_time_t predict_reflect_panel(PongState &, curve::EventQueue *, const curve::curve_time_t &);
};

}} // openage::curvepong
