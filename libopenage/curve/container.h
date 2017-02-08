// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#pragma once

#include "curve.h"
#include "container_iterator.h"

#include <list>
#include <memory>

#include <cassert>

namespace openage {
namespace curve {

template <class _T>
class Container {
public:
	typedef std::list<std::shared_ptr<_T> > container_t;
	typedef timed_iterator<_T, container_t> iterator;

	typedef std::function<curve_time_t(const _T&)> time_translator_f;
private:
	container_t container;
	time_translator_f time_translator; 

	typename container_t::iterator after(curve_time_t time);

public:
	Container(time_translator_f t) : 
		time_translator(t) 
	{ }

	void insert(curve_time_t time, std::shared_ptr<_T>);
	
	bool empty(curve_time_t from, curve_time_t to) const;

	bool count(curve_time_t from, curve_time_t to) const;

	const std::shared_ptr<_T> get(curve_time_t time_before) const;

	iterator iterate(curve_time_t from, curve_time_t to) const;

private:
	//The after-cache optimizes the performance of the after-function by a whole lot.
	mutable class after_cache_t
	{
	  private:
		struct cache_t {
			cache_t(const curve_time_t &p, const typename container_t::const_iterator &w) :
				when (p),
				what(w)
			{ };
			cache_t() : when(-1), what() {};
			curve_time_t when;
			typename container_t::const_iterator what;
		};
		static const int cache_size = 64;
		cache_t cache[cache_size];
	  public:
		bool request(const curve_time_t &when, typename container_t::const_iterator *out);
		void set(const curve_time_t &when, const typename container_t::const_iterator &what);
	} after_cache;
};

template<class _T>
typename Container<_T>::container_t::iterator Container<_T>::after(curve_time_t time) const {
	typename container_t::const_iterator it;
	if (after_cache.request(time, &it)) {
		return it;
	}

	//The Data cannot be in the container
	if (container.empty()) {
		return container.end();
	}
	if (container.last()->getTime() <= time) {
		return container.end();
	}

	// Start the tedious game of iterating over the container and checking if the
	// searched value might be there. 
	// TODO: Check if we do it from newest element to oldest element for performance reasons
	int i = container.size();
	it = container.begin();

	while (i-- > 0 && (*it)->getTime() <= time) {
		++it;
	}

	if (i >= 0) {
		after_cache.set(time, it);
	} else {
		return container.end();
	}
	return it;
}

template <class _T>
void Container<_T>::insert(curve_time_t time, std::shared_ptr<_T> t) {
	auto it = after(time);
	container.insert(it, t)
}
	
template <class _T>
bool Container<_T>::empty(curve_time_t from, curve_time_t to) const {
	auto start = after(from);
	return start == after(to);
}

template <class _T>
bool Container<_T>::count(curve_time_t from, curve_time_t to) const {
	assert(from < to);
	auto start = after(from);
	auto end = after(to);
	int i = 0;
	while(start != end) {
		++start;
		++i;
	}
	return i;
}

template <class _T>
const std::shared_ptr<_T> Container<_T>::get(curve_time_t time_before) const {
	auto it = after(time_before);
	if (it == container.end()) {
		return std::shared_ptr<_T>(nullptr);
	} else {
		return *it;
	}
}

template <class _T>
typename Container<_T>::iterator Container<_T>::iterate(curve_time_t from, curve_time_t to) const {
	return container_iterator(after(from), from, to, time_translator);
}

}} // openage::curve
