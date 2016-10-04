// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "serializerstream.h"

#include <cassert>

namespace openage {
namespace network {

//TODO: Check !is_write in write methods

SerializerStream &SerializerStream::operator << (const int &data) {
    assert(is_write());
    stream.write((char*)&data, sizeof(data));
    return *this;
}


SerializerStream &SerializerStream::operator >> (int &data) {
    assert(!is_write());
    stream >> data;
    return *this;
}


SerializerStream &SerializerStream::operator << (const std::string &data) {
    assert(is_write());
    stream << data.length();
    stream.write(data.data(), data.length());

    return *this;
}


SerializerStream &SerializerStream::operator >> (std::string &data) {
    assert(!is_write());
    size_t length;
    stream >> length;
    data.resize(length);
    stream.read(&data[0], length);
    return *this;
}


SerializerStream &SerializerStream::write(const uint8_t *data, size_t length) {
    assert(is_write());
    stream.write((char*)data, length);
    return *this;
}


SerializerStream &SerializerStream::read(uint8_t *data, size_t length)  {
    assert(!is_write());
    stream.read((char*)data, length);
    return *this;
}


SerializerStream &SerializerStream::write(const int8_t *data, size_t length) {
    assert(is_write());
    stream.write((char*)data, length);
    return *this;
}


SerializerStream &SerializerStream::read(int8_t *data, size_t length) {
    assert(!is_write());
    stream.read((char*)data, length);
    return *this;
}


SerializerStream &SerializerStream::write(const char *data, size_t length) {
    assert(is_write());
    stream.write((char*)data, length);
    return *this;
}


SerializerStream &SerializerStream::read(char *data, size_t length) {
    assert(!is_write());
    stream.read((char*)data, length);
    return *this;
}


SerializerStream &SerializerStream::on_wire(int *data) {
    if (is_write()) {
        (*this) << *data;
    } else {
        (*this) >> *data;
    }
    return *this;
}


SerializerStream &SerializerStream::on_wire(std::string *data) {
    if (is_write()) {
        (*this) << *data;
    } else {
        (*this) >> *data;
    }
    return *this;
}


const std::iostream &SerializerStream::data() {
    assert(is_write());
    return stream;
}


void SerializerStream::clear() {
    stream.clear();
}


void SerializerStream::set_data(int8_t *buffer, size_t size)  {
    assert(!is_write());
    clear();
    write((char*)buffer, size);
}


void SerializerStream::set_write_mode(bool mode) {
    is_write_ = mode;
}


bool SerializerStream::is_write() {
    return is_write_;
}

size_t SerializerStream::size() {
    size_t cur = stream.tellg(); //Current position
    stream.seekg(0, std::ios::end);
    size_t size = stream.tellg();  //End
    stream.seekg(cur, std::ios::beg);
    return size;
}

}
} // openage::network
