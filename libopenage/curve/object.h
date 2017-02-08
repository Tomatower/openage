// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#pragma once

#include "curve.h"

namespace openage {
namespace curve {

template <class _T>
class CurveObject {
public:
	const _T &at(curve_time_t time);
};

}} // openage::curve
