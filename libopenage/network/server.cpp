// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#include "server.h"

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/socket.h>
#include <linux/sctp.h>

#include "packet.h"
#include "serializerstream.h"

namespace openage {

namespace network {


size_t Server::SockaddrHasher::operator() (const sockaddr_storage &socka) const {
	if (socka.ss_family == AF_INET) {
		return (size_t)(((sockaddr_in*)&socka)->sin_addr.s_addr);
	} else if (socka.ss_family == AF_INET6) {
		return openage::util::hash_combine(((size_t*)((sockaddr_in6*)&socka)->sin6_addr.s6_addr)[0],
										   ((size_t*)((sockaddr_in6*)&socka)->sin6_addr.s6_addr)[1]);
	}
	return 0; //TODO Fail epically
}


bool Server::SockaddrHasher::operator () (const sockaddr_storage &a, const sockaddr_storage &b) const {
	return memcmp(&a, &b, a.ss_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6)) == 0;
}


Server::Server(short port, const Host::object_provider_t &op) : Interface("", port, InterfaceType::SERVER, op) {
  setup_server(port);
}


const Server::host_pool &Server::connected_players() const {
	return this->connected;
}


void Server::setup_server(short port) {
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
	
	struct sockaddr_storage res_addr;
	int res_addrlen = 0;
	for (struct addrinfo *res = servinfo;
	     this->udp_socket < 0 && res != nullptr;
	     res = res->ai_next)
	{
		this->udp_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (this->udp_socket < 0) {
			log(ERR, "socket()", false, errno);
			continue;
		}

		if (bind(this->udp_socket, res->ai_addr, res->ai_addrlen) < 0) {
			log(ERR, "bind()", false, errno);
			continue;
		}

		memcpy(&res_addr, res->ai_addr, res->ai_addrlen);
		res_addrlen = res->ai_addrlen;
		this->log(INFO, "started Successfully", false);
		break;
	}

	if (this->udp_socket < 0) {
		//Something failed
		this->log(ERR, "failed to start", true, errno);
		freeaddrinfo(servinfo); // free the linked-list
		return;
	}
	freeaddrinfo(servinfo); // free the linked-list
	
	int yes = 1;
	// lose the pesky "Address already in use" error message
	if (setsockopt(this->udp_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
		//Something failed
		this->log(ERR, "setsockopt(SO_REUSEADDR)", true, errno);
		return;
	}

	//Now we have a working udp socket in gameloop_socket
	
	// TODO check if we have to create a new UDP socket for every connection, or if just accept()ing is enough
	this->gameloop_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
	
	setsockopt(this->gameloop_socket, 
				IPPROTO_SCTP, 
				SCTP_SET_PEER_PRIMARY_ADDR,
				&res_addr,
				res_addrlen);
		
}


std::shared_ptr<Host> Server::get_host(const sockaddr_storage &addr) {
	host_pool::iterator it = this->connected.find(addr);
	if (it == this->connected.end()) {
		return std::shared_ptr<Host>(nullptr);
	}
	return it->second;
}

std::shared_ptr<Host> Server::get_handshakepending(const sockaddr_storage &addr) {
	host_pool::iterator it = this->handshake_pending.find(addr);
	if (it == this->handshake_pending.end()) {
		return std::shared_ptr<Host>(nullptr);
	}
	return it->second;
}

//TODO use libav for event reception?
void Server::begin_service() {
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

		std::shared_ptr<Host> host = get_host(src_addr);

		// Now we have a wonderfull package in network_buffer
		// Maybe distinguish the type of connection from the first byte?
		if (host) {
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
				{
					char test[] = "Can I haz plz"; 
					for (int o = 0; o < 100; ++o) {
						sendto(this->udp_socket, test, sizeof(test), 0, (struct sockaddr *)&src_addr, addr_len);
					}
				}
				this->log(WARN,"Received unknown message identifier ");
				break;
			}
		} else {
			host = get_handshakepending(src_addr);
			if (not host) {

				//Check if state is in Accept Connection Mode

				host = std::shared_ptr<Host>(new Host(this->logsink, object_provider));
				host->src_addr = src_addr;
				host->src_addr_len = addr_len;

				this->handshake_pending.insert(std::make_pair(src_addr, host));

				this->log(INFO, "Recieved message from a not connected host");
			}
			switch (this->network_buffer[0]) {
			case ProtocolIdentifier::HANDSHAKE_MESSAGE_V0:
				ss.clear();
				ss.set_data(&this->network_buffer[1], retval-1);
				if (host->handshake->handle(ss)) {
					send_buffer(host, ss);
				}
				break;
			default:
				this->log(WARN,"Received unknown message identifier ");
				break;
			}
		}
	}
}


void Server::end_service() {

}


void Server::game_loop() {
	this->begin_service();
	//TODO Do fancy stuff with the messages received

	for (size_t i = 0; i < this->get_event_count(); ++i) {
        const auto &e = this->get_event(i);

		;	
		///TODO Handle Events

	}


	this->end_service();
}




}} //openage::network
