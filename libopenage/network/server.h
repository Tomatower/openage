// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#pragma once

#include "network.h"

namespace openage {

namespace network {

class Server : public Interface {
public:
    Server(short port, const Host::object_provider_t &);

    class SockaddrHasher {
	public:
		size_t operator() (const sockaddr_storage &socka) const;
		bool operator() (const sockaddr_storage &a, const sockaddr_storage &b) const;
	};

	typedef std::unordered_map<sockaddr_storage, std::shared_ptr<Host>, SockaddrHasher, SockaddrHasher> host_pool;


    /** @brief server: get connected players
	 *
	 * Gets a list of alshitl connected players, only consists of the server on
	 * a client instance.
	 *
	 * @return const std::deque<Host *>& list of connected players
	 *
	 */
	const host_pool &connected_players() const;

	/** @brief start the service/event handling
	 *
	 * Start the event handling service and take a snapshot from any network
	 * activity happening.
	 *
	 */
	virtual void begin_service() override;

	/** \brief Run the Game Loop
     *
     * \return void
     *
     */
	virtual void game_loop() override;

	virtual void end_service() override;


private:
    void setup_server(short port);
	std::shared_ptr<Host> get_host(const sockaddr_storage &addr);
	std::shared_ptr<Host> get_handshakepending(const sockaddr_storage &addr);


	host_pool connected;
	host_pool handshake_pending;


};

}} // openage::network
