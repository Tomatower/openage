// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#include "client.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory.h>

#include "packet.h"
#include "serializerstream.h"

namespace openage {

namespace network {

Client::Client(const std::string &remote, short port, const Host::object_provider_t & op) :
    Interface(remote, port, InterfaceType::CLIENT, op)
{
    setup_client(remote, port);
}

void Client::setup_client(const std::string &remote, short port) {
	this->remote = remote;
	this->addr = "";

	std::string portstring;
	{
		std::stringstream ss;
		ss << port;
		portstring = ss.str();
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_DGRAM;  // UDP stream sockets

	int status;
	struct addrinfo *servinfo;
	if ((status = getaddrinfo(remote.c_str(), portstring.c_str(), &hints, &servinfo)) != 0) {
		log(ERR, std::string("getaddrinfo()") + gai_strerror(status), true);
	}


	for (struct addrinfo *res = servinfo;
	     this->gameloop_socket < 0 && res != nullptr;
	     res = res->ai_next)
	{
		char addr_str[INET6_ADDRSTRLEN];
		inet_ntop(res->ai_family,res->ai_addr, addr_str, sizeof (addr_str));
		this->addr = addr_str;

		this->gameloop_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (this->gameloop_socket < 0) {
			this->log(INFO, "socket()", false, errno);
			continue;
		}

		if (connect(this->gameloop_socket, res->ai_addr, res->ai_addrlen) < 0) {
			this->log(INFO, "connect()", false, errno);
			continue;
		}
		this->log(INFO, "connected successfully", false);

		host = std::shared_ptr<Host>(new Host(this->logsink, object_provider));
		host->src_addr = *(sockaddr_storage*)res->ai_addr;
		host->src_addr_len = res->ai_addrlen;

		break;
	}
	freeaddrinfo(servinfo); // free the linked-list

	if (this->gameloop_socket < 0) {
		//Something failed
		this->log(ERR, "failed to connect: ", true, errno);
	}
}

void Client::begin_service() {
	//Receive the stuff, fill event buffer
	sockaddr_storage src_addr;

	bool pending = true;
	SerializerStream ss(logsink);
	ss.set_write_mode(false); // we dont write to the stream
	while (pending) {
		unsigned int addr_len = sizeof(src_addr);
		int retval = recvfrom(this->gameloop_socket,
					&this->network_buffer[0],
					this->network_buffer.size(),
					MSG_DONTWAIT,
					(sockaddr*)&src_addr, &addr_len
				);

		if (retval == -1) {
            switch(errno) {
			case EAGAIN:
				pending = false;
				break;
			default:
				this->log(ERR, "recvfrom", false, errno);
				break;
            }
            continue;
		}

		/*if (server_addr != src_addr) {
			this->log(INFO, "Received Message not from server. Skipping");
		}*/

		// Now we have a wonderfull package in network_buffer
		// Maybe distinguish the type of connection from the first byte?
		switch (this->network_buffer[0]) {
		case ProtocolIdentifier::HANDSHAKE_MESSAGE_V0:
			ss.clear();
			ss.set_data(&this->network_buffer[1], retval-1);
			host->handshake->from_wire(ss);
			break;
		case ProtocolIdentifier::GAME_MESSAGE_V0:
			ss.clear();
			ss.set_data(&this->network_buffer[1], retval-1);
			host->wire->from_wire(ss);
			break;
		default:
			this->logsink.log(WARN << "Unknown protocol Identifier " << this->network_buffer[0]);
			break;
		}
	}
}


void Client::game_loop() {
	this->begin_service();
	//TODO Do fancy stuff with the messages received

	for (size_t i = 0; i < this->get_event_count(); ++i) {
        const auto &e = this->get_event(i);

		e;
		///TODO Handle Events
	}

	this->handle(host);
	this->end_service();
}


void Client::end_service() {
	//Send replies in handshake mode
}


}} // openage::network
