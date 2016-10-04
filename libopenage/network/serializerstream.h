// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <string>
#include <sstream>

namespace openage {
namespace network {

class SerializerStream {
public:
    SerializerStream() :
        size_{0}, is_write_{true} {}

    SerializerStream &operator << (const int &data);
    SerializerStream &operator >> (int &data);

    SerializerStream &operator << (const std::string &data);
    SerializerStream &operator >> (std::string &string);

    SerializerStream &write(const uint8_t *data, size_t length);
    SerializerStream &read(uint8_t *data, size_t length);

    SerializerStream &write(const int8_t *data, size_t length);
    SerializerStream &read(int8_t *data, size_t length);

    SerializerStream &write(const char *data, size_t length);
    SerializerStream &read(char *data, size_t length);

    SerializerStream &on_wire(int *data);
    SerializerStream &on_wire(std::string *data);

    const std::iostream &data();

    void clear();

    void set_data(int8_t *buffer, size_t size);

    void set_write_mode(bool write);

    bool is_write();

    size_t size();
private:
    std::stringstream stream;
    size_t size_;
    bool is_write_;
};

}
} // openage::network
