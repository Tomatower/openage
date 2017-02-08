// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "network.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory.h>

#include "packet.h"
#include "serializerstream.h"
#include "handshakemanager.h"

#include "../error/error.h"
#include "../log/log.h"
#include "../log/message.h"

using namespace std::placeholders;


namespace openage {
namespace network {

Host::Host(openage::log::NamedLogSource &log, const Host::object_provider_t &objectprovider) :
	state(State::DISCONNECT),
	handshake(new HandshakeManager())
{
	wire = std::unique_ptr<WireManager>(new WireManager(log, std::bind(objectprovider, connected_player, _1)));
}


Interface::Interface(const std::string& remote, short port, InterfaceType type, const Host::object_provider_t &op) :
	remote(remote),
	port(port),
	logsink( type == InterfaceType::SERVER ? "SRV" : "NET"),
	gameloop_socket{-1},
	serializer{new SerializerStream(logsink)},
	network_buffer(100000, 0),
	is_server{ type == InterfaceType::SERVER },
	object_provider{op}
{
}


Interface::~Interface() {
	close(gameloop_socket);
}


std::string Interface::get_state() const {
	return this->state.text;
}


size_t Interface::get_event_count() {
	return this->events.size();
}


const std::unique_ptr<Event> &Interface::get_event(int id) const {
	if ((size_t)id < this->events.size()) {
		return *(this->events.begin() + id);
	} else {
		static auto null_ref = std::unique_ptr<Event>(nullptr);
		return null_ref;
	}
}


void Interface::log(log::MessageBuilder msg, std::string text, bool fatal, int err) {
	if (this->is_server) {
		msg << "";
	} else {
		msg << remote << "(" << addr << "): ";
	}
	msg << text;

	if (err) {
		msg << "[" << strerror(errno) << "]";
	}

	this->logsink.log(msg);
	this->state = msg;

	if (fatal) {
		throw new openage::error::Error(msg);
	}
}


void Interface::send_buffer(const std::shared_ptr<Host> &to, const std::vector<int8_t> &buffer, int size) {
	//TODO Lock
	sendto(this->gameloop_socket, &buffer[0], size, MSG_NOSIGNAL, (sockaddr*)&to->src_addr, to->src_addr_len);
}


void Interface::send_buffer(const std::shared_ptr<Host> &to, const SerializerStream &stream) {
	//TODO Lock
	int size = stream.get_data(network_buffer);
	send_buffer(to, network_buffer, size);
}


void Interface::handle(std::shared_ptr<Host> host) {
	this->serializer->clear();
	this->serializer->set_write_mode(true);
	// Move data from the wiremanager into the serializer
	host->wire->to_wire(*serializer);

    // TODO Check size; TODO make it in place
	// Move Serializer Data into the network buffer
    size_t size = this->serializer->get_data(this->network_buffer);
	// Send the Data to the network
	this->send_buffer(host, this->network_buffer, size);
}


}} //openage::network
