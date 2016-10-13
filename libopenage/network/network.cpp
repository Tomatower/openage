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

#include "../error/error.h"
#include "../log/log.h"
#include "../log/message.h"


namespace openage {
namespace network {


size_t Interface::SockaddrHasher::operator() (const sockaddr_storage &socka) const {
	if (socka.ss_family == AF_INET) {
		return (size_t)(((sockaddr_in*)&socka)->sin_addr.s_addr);
	} else if (socka.ss_family == AF_INET6) {
		return openage::util::hash_combine(((size_t*)((sockaddr_in6*)&socka)->sin6_addr.s6_addr)[0],
										   ((size_t*)((sockaddr_in6*)&socka)->sin6_addr.s6_addr)[1]);
	}
	return 0; //TODO Fail epically
}


bool Interface::SockaddrHasher::operator () (const sockaddr_storage &a, const sockaddr_storage &b) const {
	return memcmp(&a, &b, a.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) == 0;
}


Interface::Interface(const std::string& remote, short port, InterfaceType type) :
	remote(remote),
	port(port),
	logsink( type == InterfaceType::SERVER ? "SRV" : "NET"),
	gameloop_socket{-1},
	serializer{logsink},
	network_buffer(100000, 0),
	is_server{ type == InterfaceType::SERVER }
{
	if (this->is_server) {
		this->setup_server(port);
	} else {
		this->setup_client(this->remote, this->port);
	}

}


Interface::~Interface() {
	close(gameloop_socket);
}


std::string Interface::get_state() const {
	return this->state.text;
}


const Interface::host_pool &Interface::connected_players() const {
	return this->connected;
}


void Interface::begin_service() {
	//Receive the stuff, fill event buffer
	sockaddr_storage src_addr;

	bool pending = true;
	SerializerStream ss(logsink);
	ss.set_write_mode(false); // we dont write to the stream
	while (pending) {
		unsigned int addr_len = sizeof(src_addr);
		int retval = recvfrom(this->gameloop_socket, &this->network_buffer[0], this->network_buffer.size(),
				MSG_DONTWAIT,
				(sockaddr*)&src_addr, &addr_len);

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

		std::shared_ptr<Host> host = get_host(src_addr);
		//Now we have a wonderfull package in network_buffer
		//Maybe distinguish the type of connection from the first byte?

		if (host) {
			switch (this->network_buffer[0]) {
			case ProtocolIdentifier::HANDSHAKE_MESSAGE_V0:
				ss.clear();
				ss.set_data(&this->network_buffer[1], retval-1);
				host->handshake->on_wire(ss);
				break;
			case ProtocolIdentifier::GAME_MESSAGE_V0:
				ss.clear();
				ss.set_data(&this->network_buffer[1], retval-1);
				host->wire->on_wire(ss);
				break;
			}
		} else {
			//Check if state is in Accept Connection Mode

			host = std::shared_ptr<Host>(new Host());
			host->src_addr = src_addr;
			host->src_addr_len = addr_len;

			this->handshake_pending.insert(std::make_pair(src_addr, host));

			this->log(INFO, "Recieved message from a not connected host");  //TODO handshake
		}
	}
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


void Interface::handle(std::shared_ptr<Host> host) {
	this->serializer.clear();
	this->serializer.set_write_mode(true);
	host->wire->on_wire(serializer);

    //TODO Check size
    size_t size = this->serializer.get_data(this->network_buffer);
	this->send_buffer(host, this->network_buffer, size);
}


void Interface::end_service() {
	//Send replies in handshake mode
}


void Interface::game_loop() {
	this->begin_service();
	//TODO Do fancy stuff with the messages received

	for (size_t i = 0; i < this->get_event_count(); ++i) {
        const auto &e = this->get_event(i);
	}

	for (auto p : this->connected_players()) {
		//TODO Insert Input & Nyan

		this->handle(p.second);
	}

	this->end_service();
}


void Interface::lobby_loop() {

}


void Interface::setup_client(const std::string &remote, short port) {
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
		break;
	}
	freeaddrinfo(servinfo); // free the linked-list

	if (this->gameloop_socket < 0) {
		//Something failed
		this->log(ERR, "failed to connect: ", true, errno);
	}
}


void Interface::setup_server(short port) {
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
	if ((status = getaddrinfo(NULL, portstring.c_str(), &hints, &servinfo)) != 0) {
		this->log(ERR, std::string("getaddrinfo()") + gai_strerror(status), true);
	}


	for (struct addrinfo *res = servinfo;
	     this->gameloop_socket < 0 && res != nullptr;
	     res = res->ai_next)
	{
		this->gameloop_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (this->gameloop_socket < 0) {
			log(INFO, "socket()", false, errno);
			continue;
		}

		if (bind(this->gameloop_socket, res->ai_addr, res->ai_addrlen) < 0) {
			log(INFO, "bind()", false, errno);
			continue;
		}

		this->log(INFO, "Started Successfully", false);
		break;
	}

	if (this->gameloop_socket < 0) {
		//Something failed
		this->log(ERR, "failed to start", true, errno);
	}
	freeaddrinfo(servinfo); // free the linked-list

	int yes = 1;
	// lose the pesky "Address already in use" error message
	if (setsockopt(this->gameloop_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
		//Something failed
		this->log(ERR, "setsockopt(SO_REUSEADDR)", true, errno);
	}

	//Now we have a working socket in gameloop_socket
}


std::shared_ptr<Host> Interface::get_host(const sockaddr_storage &addr) {
	host_pool::iterator it = this->connected.find(addr);
	if (it == this->connected.end()) {
		it = this->handshake_pending.find(addr);
		if (it == this->handshake_pending.end()) {
			return std::shared_ptr<Host>(nullptr);
		}
	}
	return it->second;
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


}} //openage::network
