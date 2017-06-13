// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#pragma once

#include <iterator>
#include <limits>

#include "curve.h"
#include "iterator.h"

namespace openage {
namespace curve {

/**
 * A filtering operator to iterate over all elements of a queue whose elements
 * exist at exactly one point of time, the range where to iterate is given at
 * construction.
 *
 * It depends on val_t as its value type, container_t is the container
 * to operate on and the function valid_f, that checks if an element is alive.
 */
template <class val_t,
          class container_t>
class QueueFilterIterator :
		public CurveIterator<val_t, container_t>
{
public:
	typedef typename container_t::iterator iterator_t;

	/**
	 * Construct the iterator from its boundary conditions: time and container
	 */
	QueueFilterIterator(const iterator_t &base,
	                    const iterator_t &container_end,
	                    const curve_time_t &from,
	                    const curve_time_t &to) :
		CurveIterator<val_t, container_t>{base, container_end, from, to} {}

	virtual bool valid() const override {
		if (this->base != this->container_end) {
			return this->base->time() >= this->from && this->base->time() < this->to;
		}
		return false;
	}

	val_t &value() const override {
		return this->base->value;
	}
};

}} // openage::curve