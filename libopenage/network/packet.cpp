// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "packet.h"

#include <cassert>

namespace openage {
namespace network {

Packet::Packet(Host *host, SerializerStream &data) :
    source{host} {
    assert(!data.is_write());
    on_wire(data);
}

Packet::Packet () :
    source{nullptr} {

}

void Packet::to_stream(SerializerStream &data) {
    data.set_write_mode(true);
    on_wire(data);
}

void Packet::from_stream(SerializerStream &data) {
    clear();
    data.set_write_mode(false);
    on_wire(data);
}

void Packet::on_wire(SerializerStream &stream, int *data) {
    stream.on_wire(data);
}


void Packet::on_wire(SerializerStream &stream, std::string *data) {
    stream.on_wire(data);
}


void Packet::on_wire(SerializerStream &stream, input *data) {
    on_wire(stream, &data->player);
    on_wire(stream, &data->target_id);
    on_wire(stream, &data->action_id);
    map_on_wire(stream, &data->kv_info);
}


void Packet::on_wire(SerializerStream &stream, nyanchange *data) {
    on_wire(stream, &data->nyan_patch);
}


void Packet::on_wire(SerializerStream &stream, static_state *data) {
    on_wire(stream, &data->id);
    on_wire(stream, &data->x);
    on_wire(stream, &data->y);
    map_on_wire(stream, &data->kv_state);
}


void Packet::on_wire(SerializerStream &stream, trajectory_element *data) {
    on_wire(stream, &data->x);
    on_wire(stream, &data->y);
    on_wire(stream, &data->action);
}


void Packet::on_wire(SerializerStream &stream, dynamic_state *data) {
    on_wire(stream, &data->id);
    on_wire(stream, &data->x);
    on_wire(stream, &data->y);
    map_on_wire(stream, &data->kv_state);
    deque_on_wire(stream, &data->trajectory);
}

void Packet::map_on_wire(SerializerStream &stream, std::unordered_map<int, int> *data) {
    int cnt = data->size();
    on_wire(stream, &cnt);

    if (stream.is_write()) {
        for (auto it = data->begin(); it != data->end(); ++it) {
            std::pair<int, int> p = *it;
            on_wire(stream, &p.first);
            on_wire(stream, &p.second);
        }
    } else {
        for (int i = 0; i < cnt; ++i) {
            std::pair<int, int> p;
            on_wire(stream, &p.first);
            on_wire(stream, &p.second);
            data->insert(p);
        }
    }
}


void Packet::on_wire(SerializerStream &stream) {
    deque_on_wire(stream, &inputs);
    deque_on_wire(stream, &nyan_changes);
    deque_on_wire(stream, &static_states);
    deque_on_wire(stream, &dynamic_states);
}


void Packet::clear() {
    inputs.clear();
    nyan_changes.clear();
    static_states.clear();
    dynamic_states.clear();
}


size_t Packet::serialized_size() {
    //TODO there has to be a faster version than serializing and counting maybe Protobuf?
    SerializerStream ss;
    to_stream(ss);
    return ss.size();
}

}
} //openage::network
