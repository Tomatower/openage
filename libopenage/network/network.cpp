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
}


bool Interface::SockaddrHasher::operator () (const sockaddr_storage &a, const sockaddr_storage &b) const {
	return memcmp(&a, &b, a.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) == 0;
}


Interface::Interface(const std::string& remote, short port, InterfaceType type) :
	remote(remote),
	logsink( type == InterfaceType::SERVER ? "SRV" : "NET"),
	gameloop_socket{-1},
	network_buffer(100000, 0),
	is_server{ type == InterfaceType::SERVER }
{
	if (is_server) {
		setup_server(port);
	} else {
		setup_client(remote, port);
	}

}


Interface::~Interface() {
	close(gameloop_socket);
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
	     gameloop_socket < 0 && res != nullptr;
	     res = res->ai_next)
	{
		char addr_str[INET6_ADDRSTRLEN];
		inet_ntop(res->ai_family,res->ai_addr, addr_str, sizeof (addr_str));
		addr = addr_str;

		gameloop_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (gameloop_socket < 0) {
			log(INFO, "socket()", false, errno);
			continue;
		}

		if (connect(gameloop_socket, res->ai_addr, res->ai_addrlen) < 0) {
			log(INFO, "connect()", false, errno);
			continue;
		}

		log(INFO, "connected successfully", false);
		break;
	}
	freeaddrinfo(servinfo); // free the linked-list

	if (gameloop_socket < 0) {
		//Something failed
		log(ERR, "failed to connect: ", true, errno);
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
		log(ERR, std::string("getaddrinfo()") + gai_strerror(status), true);
	}


	for (struct addrinfo *res = servinfo;
	     gameloop_socket < 0 && res != nullptr;
	     res = res->ai_next)
	{
		gameloop_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (gameloop_socket < 0) {
			log(INFO, "socket()", false, errno);
			continue;
		}

		if (bind(gameloop_socket, res->ai_addr, res->ai_addrlen) < 0) {
			log(INFO, "bind()", false, errno);
			continue;
		}

		log(INFO, "Started Successfully", false);
		break;
	}

	if (gameloop_socket < 0) {
		//Something failed
		log(ERR, "failed to start", true, errno);
	}
	freeaddrinfo(servinfo); // free the linked-list

	int yes = 1;
	// lose the pesky "Address already in use" error message
	if (setsockopt(gameloop_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
		//Something failed
		log(ERR, "setsockopt(SO_REUSEADDR)", true, errno);
	}

	//Now we have a working socket in gameloop_socket
}


void Interface::pop_packet() {
	for (auto it = jitter_buffer.begin(); it != jitter_buffer.end(); ++it) {
		if ((*it)->_server_frame == current_packet_frame) {
			pool.push_back(*it);
			jitter_buffer.erase(it);
			break;
		}
	}
	current_packet_frame ++;
}


std::string Interface::get_state() const {
	return state.text;
}


const std::deque<std::unique_ptr<Host>>& Interface::connected_players() const {
	return connected;
}


void Interface::begin_service() {
	//Receive the stuff, fill event buffer
	sockaddr src_addr;

	bool pending = true;
	while (pending) {
		unsigned int addr_len = sizeof(src_addr);
		int retval = recvfrom(gameloop_socket, &network_buffer[0], network_buffer.size(),
				MSG_DONTWAIT,
				&src_addr, &addr_len);

		if (retval == -1) {
            switch(errno) {
			case EAGAIN:
				pending = false;
				break;
			default:
				log(ERR, "RECV", false, errno);
				break;
            }
            continue;
		}

		std::shared_ptr<Host> host = get_host(src_addr);
		//Now we have a wonderfull package in network_buffer
		if (host) {
			host->wire->from_wire(network_buffer, retval);
		} else {
			log(INFO, "Recieved message from a not connected host");  //TODO handshake
		}
	}


}


size_t Interface::get_event_count() {
	return events.size();
}


Event* Interface::get_event(int id) const {
	if (id < events.size()) {
		return *(events.begin() + id);
	} else {
		return nullptr;
	}
}


void Interface::end_service() {
	//Send replies in handshake mode
}


void Interface::game_loop() {
	begin_service();
	//TODO Do fancy stuff with the messages received

	for (int i = 0; i < get_event_count(); ++i) {
        Event *e = get_event(i);

	}

	end_service();
}


void Interface::lobby_loop() {

}


void Interface::log(log::message msg, std::string text, bool fatal, int err) {
	if (is_server) {
		msg.text = "";
	} else {
		msg.text = remote + "(" + addr + "): ";
	}
	msg.text += text;

	if (err) {
		msg.text += "[";
		msg.text += strerror(errno);
		msg.text += "]";
	}

	logsink.log(msg);
	state = msg;

	if (fatal) {
		throw new openage::error::Error(msg);
	}
}


std::shared_ptr<Host> Interface::get_host(const sockaddr_storage &addr, bool pending_allowed) {
	auto it = connected.find(addr);
	if (it == connected.end()) {
		if  (pending_allowed) {
			it = handshake_pending.find(addr);
			if (it == handshake_pending.end()) {
				return std::shared_ptr<Host>(nullptr);
			}
		} else {
			return std::shared_ptr<Host>(nullptr);
		}
	}

	//it is now valid

	return it->second;
}


}} //openage::network
