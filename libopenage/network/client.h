// Copyright 2017-2017 the openage authors. See copying.md for legal info.

#pragma once

#include "network.h"

namespace openage {

namespace network {

class Client : public Interface {
public:
    Client(const std::string &remote, short port, const Host::object_provider_t &);

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
	void setup_client(const std::string &remote, short port);


private:
	std::shared_ptr<Host> host;

};

}} // openage::network
