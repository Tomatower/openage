// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "serializerstream.h"

#include <cassert>

namespace openage {
namespace network {


SerializerStream &SerializerStream::write(const int8_t *data, size_t length) {
	assert(is_write());
	std::copy(data, data + length, std::back_inserter(buffer));

	return *this;
}


SerializerStream &SerializerStream::read(int8_t *data, size_t length) {
	assert(!is_write());
	std::copy(buffer.begin(), buffer.begin() + length, data);
	buffer.erase(buffer.begin(), buffer.begin() + length);
	return *this;
}


SerializerStream &SerializerStream::on_wire(int *data) {
	if (is_write()) {
		write((int8_t *)data, sizeof(*data));
	} else {
		read((int8_t *)data, sizeof(*data));
	}
	return *this;
}


SerializerStream &SerializerStream::on_wire(std::string *data) {
	size_t length = is_write() ? data->length() : 0;

	on_wire((int *)&length);

	if (is_write()) {
		write((int8_t *)&(*data)[0], length);
	} else {
		data->resize(length, ' ');
		read((int8_t *)&(*data)[0], length);
	}
	return *this;
}


void SerializerStream::clear() {
	buffer.clear();
}


void SerializerStream::set_data(const std::vector<int8_t> &new_data)  {
	clear();
//    buffer.reserve(new_data.size());
	std::copy(new_data.begin(), new_data.end(), std::back_inserter(buffer));
}


void SerializerStream::get_data(std::vector<int8_t> &out_data) const {
	out_data.clear();
	out_data.reserve(buffer.size());
	std::copy(buffer.begin(), buffer.end(), std::back_inserter(out_data));
}


void SerializerStream::set_write_mode(bool mode) {
	is_write_ = mode;
}


bool SerializerStream::is_write() const {
	return is_write_;
}


size_t SerializerStream::size() {
	return buffer.size();
}

}
} // openage::network
