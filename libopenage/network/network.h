// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <deque>
#include <unordered_map>
#include <memory>

namespace openage {

class Player;

/**
 * Network related tools
 */
namespace network {

class Packet;


/**
 * Implements the remote side of the socket.
 * Here any information related to the remote side is stored.
 */
class Host {
public:
    /**
     * The associated player
     *
     * This value has to be set upon receiving Event::Type::ESTABLISHED
     */
    Player *connected_player;
};

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
 * The primary interfacing class with network things.
 * Here connections, packets and events are managed.
 */
class Interface {
public:
    /** @brief C'tor, get connection information
     *
     * @param remote uri of the remote server.
     * @param port the port to connect to
     * @param server is this a server-network type?
     *
     */
    Interface (const std::string &remote,
             short port,
             bool server);


    /** @brief top of network packet stack
     *
     * The network packets are managed in form of a packet stack.
     * this method gives the top of the stack, maybe filtered over multiple packages in the jitter buffer.
     *
     * whenever this method returns nullptr multiple times in a row, it is to be expected that the network
     * has problems.
     *
     * @return top of network stack or nullptr, if no packet for the next frame was received.
     *
     */
    Packet *current_packet() const;

    /** @brief pop the top of network packet stack
     *
     * To move on to the next packet in the stack, you have to pop the next packet.
     * even if the last call to @see current_packet has returned nullptr, this may activate more packages in the stack
     *
     * @return void
     *
     */
    void pop_packet();

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
    const std::deque<Host *> &connected_players();

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
    int get_event_count();
    Event *get_event(int id) const;


    /** @brief end the service/event handling
     *
     * End the event handling service after a call to @see begin_service.
     *
     */
    void end_service();

    /** @brief Send a message to the remote
     *
     * Send a message to the remote. If remote is nullptr, the first host in the connected_players list is used
     *
     * @param pkt std::unique_ptr<Packet> Packet to be transmitted
     * @param remote Host * destination to send it to
     * @return void
     *
     */
    void send(std::unique_ptr<Packet> pkt, Host *remote = nullptr);

    ///@todo create a lobby-loop too
    void main_loop();

private:
    std::deque<std::unique_ptr<Event>> events;

    std::deque<Packet *> jitter_buffer;
    std::deque<Packet *> pool;

    std::deque<std::unique_ptr<Host>> connected;
    std::deque<std::unique_ptr<Host>> handshake_pending;

    int current_packet_frame;

    /**
     * get packet for frame, used for handling the input event stuff
     */
    Packet *getFramePacket(int frame);
};


}
} // openage::network
