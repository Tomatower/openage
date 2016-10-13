// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <deque>
#include <unordered_map>
#include <memory>

#include <sys/socket.h>

#include "wiremanager.h"

#include "../log/named_logsource.h"
#include "../util/hash.h"

namespace openage {

class Player;

/**
 * Network related tools
 */
namespace network {

class Packet;
class Host;

/**
 * Represents state changes in the network stack
 */
class Event {
public:
	enum class Type {
		/**
		 * The connection was openend.
		 */
		CONNECT,

		/**
		 * Handshake requested, additional information to be used for authentication should reside in the kv_store
		 * the handshake is started with a call to @see end_service.
		 */
		HANDSHAKE_REQUEST,

		/**
		 * The remote end has sent a @see HANDSHAKE_REQUEST, the @see kv_store contains all information given by
		 * the other side. Fill the kv_store with data to be transmitted in a @see HANDSHAKE_CONFIRMED or perform
		 * a disconnect.
		 */
		HANDSHAKE_RESPONSE,

		/**
		 * The other end has confirmed your handshake and sent additional information, available in @see kv_store.
		 * no additional actions are required at this point.
		 */
		HANDSHAKE_CONFIRMED,

		/**
		 * Both ends switch to normal transmission mode.
		 */
		ESTABLISHED,

		/**
		 * The Connection was dropped
		 */
		DISCONNECT,

		/**
		 * There was some kind of error, see @see kv_store for additional details
		 */
		ERROR
	};

	/**
	 * The type of the event
	 */
	Type type;

	/**
	 * The remote from where the data was received
	 */
	Host *remote;

	/**
	 * store for additional information for the events.
	 * This is also used to store information to be transmitted in reply to certain events
	 *
	 * In these cases, this store has to be rewritten in the event handler.
	 *
	 * @todo define constexprs as keys for the store
	 */
	std::unordered_map<int, int> kv_store;
};


/**
 * Implements the remote side of the socket.
 * Here any information related to the remote side is stored.
 */
class Host {
public:
	enum class State {
		/**
		 * The connection was openend.
		 */
		CONNECT = Event::Type::CONNECT,

		/**
		 * Handshake requested, additional information to be used for authentication should reside in the kv_store
		 * the handshake is started with a call to @see end_service.
		 */
		HANDSHAKE_REQUEST = Event::Type::HANDSHAKE_REQUEST,

		/**
		 * The remote end has sent a @see HANDSHAKE_REQUEST, the @see kv_store contains all information given by
		 * the other side. Fill the kv_store with data to be transmitted in a @see HANDSHAKE_CONFIRMED or perform
		 * a disconnect.
		 */
		HANDSHAKE_RESPONSE = Event::Type::HANDSHAKE_RESPONSE,

		/**
		 * The other end has confirmed your handshake and sent additional information, available in @see kv_store.
		 * no additional actions are required at this point.
		 */
		HANDSHAKE_CONFIRMED = Event::Type::HANDSHAKE_CONFIRMED,

		/**
		 * Both ends switch to normal transmission mode.
		 */
		ESTABLISHED = Event::Type::ESTABLISHED,

		/**
		 * The Connection was dropped
		 */
		DISCONNECT = Event::Type::DISCONNECT,

		/**
		 * There was some kind of error, see @see kv_store for additional details
		 */
		ERROR = Event::Type::ERROR
	};

	State state;

	sockaddr_storage src_addr;
	socklen_t src_addr_len;


	std::unique_ptr<WireManager> wire;

	std::unique_ptr<HandshakeManager> handshake;

	/**
	 * The associated player
	 *
	 * This value has to be set upon receiving Event::Type::ESTABLISHED
	 */
	std::shared_ptr<Player> connected_player;
};


enum class InterfaceType {
	SERVER,
	CLIENT,
};


/**
 * The primary interfacing class with network things.
 * Here connections, packets and events are managed.
 */
class Interface {
public:

	enum class ProtocolIdentifier {
		HANDSHAKE_MESSAGE_V0 = 0,
		GAME_MESSAGE_V0 = 1,
	};

	class SockaddrHasher {
	public:
		size_t operator() (const sockaddr_storage &socka) const;
		bool operator() (const sockaddr_storage &a, const sockaddr_storage &b) const;
	};

	typedef std::unordered_map<sockaddr_storage, std::shared_ptr<Host>, SockaddrHasher, SockaddrHasher> host_pool;

	/** @brief C'tor, get connection information
	 *
	 * @param remote uri of the remote server.
	 * @param port the port to connect to
	 * @param server is this a server-network type?
	 *
	 */
	Interface (const std::string &remote,
	           short port,
	           InterfaceType type);

	~Interface();

	/** @brief get a state string
	 *
	 * Create a network state string to be displayed as debug information in the ui
	 *
	 * @return std::string Message from the network to the world
	 *
	 */
	std::string get_state() const;

	/** @brief server: get connected players
	 *
	 * Gets a list of alshitl connected players, only consists of the server on a client instance.
	 *
	 * @return const std::deque<Host *>& list of connected players
	 *
	 */
	const host_pool &connected_players() const;

	/** @brief start the service/event handling
	 *
	 * Start the event handling service and take a snapshot from any network activity happening.
	 *
	 */
	void begin_service();

	/** @brief get number of events
	 *
	 * Get the number of events that occurred between the last call to @see end_service and @see begin_service
	 * These can be read with @see get_event.
	 *
	 * @warning this is only available between calls to @see begin_service and @see end_service
	 * @return int number of events
	 *
	 */
	size_t get_event_count();

    /** \brief Accessor to a event in the queue
     *
     * \param id int index of the event in the range of [0 .. @see get_event_count()]
     * \return Event*
     *
     */
	const std::unique_ptr<Event> &get_event(int id) const;

	/** @brief Send the prepared message to the remote
	 *
	 * The Message will be created in this step via callbacks
	 *
	 * @param remote Host * destination to send it to. The Wiremanager of this user will be used
	 * @return void
	 *
	 */
	void handle(std::shared_ptr<Host> host);

	/** @brief end the service/event handling
	 *
	 * End the event handling service after a call to @see begin_service.
	 *
	 */
	void end_service();

    /** \brief Run the Game Loop
     *
     * \return void
     *
     */
	void game_loop();

    /** \brief Run the Lobby-network
     *
     * \return void
     *
     */
	void lobby_loop();

private:
	void setup_client(const std::string &remote, short port);
	void setup_server(short port);

	std::shared_ptr<Host> get_host(const sockaddr_storage &addr);

	void log(log::MessageBuilder, std::string text, bool fatal = false, int err = 0);

	void send_buffer(const std::shared_ptr<Host> &to, const std::vector<int8_t> &buffer, int size);



private:
	int16_t port;
	std::string remote;
	std::string addr;

	openage::log::NamedLogSource logsink;

	std::deque<std::unique_ptr<Event>> events;

	host_pool connected;
	host_pool handshake_pending;


	int gameloop_socket; //My UDP socket

	SerializerStream serializer;
	std::vector<int8_t> network_buffer;

	bool is_server;

	log::message state;
};


}
} // openage::network
