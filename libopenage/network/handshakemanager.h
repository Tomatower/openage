// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#pragma once


namespace openage {
namespace network {
class SerializerStream;

class HandshakeManager {
public:

	void from_wire (SerializerStream &ss);
	void to_wire (SerializerStream &ss);

	bool handle (SerializerStream &ss);
};

}} //openage::network
