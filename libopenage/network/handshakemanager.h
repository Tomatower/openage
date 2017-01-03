// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once


namespace openage {
namespace network {
class SerializerStream;

class HandshakeManager {
public:

	void from_wire (SerializerStream &ss);
	void to_wire (SerializerStream &ss);

};

}} //openage::network
