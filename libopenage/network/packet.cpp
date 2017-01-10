// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "packet.h"

#include <cassert>

#include "serializerstream.h"

namespace openage {
namespace network {

Packet::Packet () :
	source{nullptr} {

}


void Packet::clear() {
	this->inputs.clear();
	this->nyan_changes.clear();
	this->object_states.clear();
}


size_t Packet::serialized_size() {
	//TODO there has to be a faster version than serializing and counting maybe Protobuf?
//	SerializerStream ss(logsink);
//	to_stream(ss);
//	return ss.size();
	return 0;
}

}
} //openage::network
