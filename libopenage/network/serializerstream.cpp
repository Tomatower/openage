// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "serializerstream.h"

#include "../error/error.h"

#include <cassert>

namespace openage {
namespace network {


SerializerStream &SerializerStream::write(const uint8_t *data, size_t length) {
	assert(this->is_write());
	std::copy(data, data + length, std::back_inserter(this->buffer));

	this->logsink.log(MSG(warn) << "Write #" << length << "B (" << this->buffer.size() << ")");
	return *this;
}


SerializerStream &SerializerStream::read(uint8_t *data, size_t length) {
	assert(!this->is_write());

	if (length == 0) {
		return *this;
	}
	if (length > this->buffer.size()) {

		assert(("buffer.size >= length", false));
	}

	std::copy(this->buffer.begin(), this->buffer.begin() + length, data);
	this->buffer.erase(this->buffer.begin(), this->buffer.begin() + length);
	this->logsink.log(MSG(warn) << "Read " << length << "B (" << this->buffer.size() << ")");
	return *this;
}

uint8_t *SerializerStream::write_observable(uint8_t *data, size_t size) {
	if (size > 0) {
		//Store the currently last element
		uint8_t *tmp = &*buffer.rbegin();
		//write everything
		this->write(data, size);
		//go now to the beginning of the new element
		tmp ++;
		return tmp;
	} else {
		openage::log::message e = ERR << "Cannot observe types of size 0";
		throw new openage::error::Error(e);
	}
	return nullptr;
}


SerializerStream &SerializerStream::on_wire(Packet *p) {
	this->deque_on_wire(&p->inputs);
	this->deque_on_wire(&p->nyan_changes);
	this->deque_on_wire(&p->object_states);
	return *this;
}


SerializerStream &SerializerStream::on_wire(Packet::input *data) {
	this->on_wire(&data->player);
	this->on_wire(&data->target_id);
	this->on_wire(&data->action_id);
	this->map_on_wire(&data->kv_info);
	return *this;
}


SerializerStream &SerializerStream::on_wire(int32_t *data) {
	if (this->is_write()) {
		this->write((uint8_t *)data, sizeof(*data));
	} else {
		this->read((uint8_t *)data, sizeof(*data));
	}
	return *this;
}

SerializerStream &SerializerStream::on_wire(uint16_t *data) {
	if (this->is_write()) {
		this->write((uint8_t *)data, sizeof(*data));
	} else {
		this->read((uint8_t *)data, sizeof(*data));
	}
	return *this;
}

SerializerStream &SerializerStream::on_wire(std::string *data) {
	size_t length = this->is_write() ? data->length() : 0;

	this->on_wire((int *)&length);

	if (this->is_write()) {
		this->write((const uint8_t *)data->c_str(), length);
	} else {
		data->resize(length, ' ');
		this->read((uint8_t *)&(*data)[0], length); //TODO maybe use intermediate buffer
	}
	return *this;
}


SerializerStream &SerializerStream::on_wire(Packet::nyanchange *data) {
	this->on_wire(&data->nyan_patch);
	return *this;
}


SerializerStream &SerializerStream::on_wire(Packet::object_state *data) {
	this->on_wire(&data->id);
	this->on_wire(&data->x);
	this->on_wire(&data->y);
	this->map_on_wire(&data->kv_state);
	this->deque_on_wire(&data->trajectory);
	return *this;
}


SerializerStream &SerializerStream::on_wire(Packet::trajectory_element *data) {
	this->on_wire(&data->x);
	this->on_wire(&data->y);
	this->on_wire(&data->action);
	return *this;
}


SerializerStream &SerializerStream::map_on_wire(std::unordered_map<int32_t, int32_t> *data) {
	int32_t cnt = data->size();
	this->on_wire(&cnt);

	if (this->is_write()) {
		for (auto it = data->begin(); it != data->end(); ++it) {
			std::pair<int32_t, int32_t> p = *it;
			this->on_wire(&p.first);
			this->on_wire(&p.second);
		}
	} else {
		for (int i = 0; i < cnt; ++i) {
			std::pair<int32_t, int32_t> p;
			this->on_wire(&p.first);
			this->on_wire(&p.second);
			data->insert(p);
		}
	}
	return *this;
}


void SerializerStream::clear() {
	this->buffer.clear();
}


void SerializerStream::set_data(const std::vector<int8_t> &new_data) {
	this->clear();
//    buffer.reserve(new_data.size());
	std::copy(new_data.begin(), new_data.end(), std::back_inserter(this->buffer));
}


void SerializerStream::set_data(int8_t *start, size_t count) {
	this->clear();
//    buffer.reserve(count);
	std::copy(start, start + count, std::back_inserter(this->buffer));
}


size_t SerializerStream::get_data(std::vector<int8_t> &out_data) const {
	out_data.clear();
	out_data.reserve(this->buffer.size());
	std::copy(this->buffer.begin(), this->buffer.end(), std::back_inserter(out_data));
	return this->buffer.size();
}


size_t SerializerStream::get_data(int8_t *out, size_t max_len) {
	if (this->buffer.size() < max_len) {
		std::copy(this->buffer.begin(), this->buffer.end(), out);
		return this->buffer.size();
	} else {
		std::copy(this->buffer.begin(), this->buffer.begin() + max_len, out);
		return max_len;

		//TODO LOG
	}
}


void SerializerStream::set_write_mode(bool mode) {
	this->is_write_ = mode;
}


bool SerializerStream::is_write() const {
	return this->is_write_;
}


size_t SerializerStream::size() {
	return this->buffer.size();
}

}} // openage::network
