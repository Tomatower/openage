// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <deque>
#include <unordered_map>
#include <istream>
#include <ostream>

#include "serializerstream.h"

namespace openage {
namespace network {

class Interface;
class Host;

/**
 *
 * An information on the network can be in any of these fields:
 *
 * <pre>
 *            | reliable | unreliable |
 * urgent     |    1     |     2      |
 * -----------------------------------|
 * continuous |    3     |     4      |
 * </pre>
 *
 * Urgent information is transmitted immediately, continuous information is from larger dataset, and is used to
 * fill up the space within a packet, that is not used when all the other information is done.
 *
 * reliable information is to be retransmitted until the other end confirms the retrieval of the information.
 * unreliable information is transmitted, but if it will not reach the destination, it is not the end of all live.
 *
 * Examples:
 * <ol>
 * <li>Reliable/Urgent: User input</li>
 * <li> --</li>
 * <li>nyan</li>
 * <li>World Status updates</li>
 * </ol>
 *
 * The Packetclass is filtered over jitter buffer and partial state updates
 * Partial state updates provide a small subset of the world state of the remote
 *
 * the client does not send any position updates
 */
class Packet {
    friend class Interface;
public:

    /** @brief C'tor for packet
     *
     * Create a packet from receiving
     *
     * @param host Host*
     * @param stream SerializerStream&
     *
     */
    Packet(Host *host, SerializerStream &stream);

    /** @brief C'tor for Packet
     *
     * Create an empty packet for sending
     */
    Packet();

    /** @brief serialize this packet to the stream
     *
     * @param stream SerializerStream&
     * @return void
     *
     */
    void to_stream(SerializerStream &stream);

    /** @brief clear the packet and create it from the stream
     *
     * @param stream SerializerStream&
     * @return void
     *
     */
    void from_stream(SerializerStream &stream);

    /** Host from where the message was received */
    Host *source;

    /**
     * Input actions that happened within this packet's timeframe.
     * It identifies a source player, a target id (a unit), and the action that happened
     * The kv_info key value mapping allows to store more information regarding the specified action
     *
     * An input event includes: movement of units, creation of units & buildings, calling special abilities,
     * death of units and buildings, ...
     */
    struct input {
        int player;
        int target_id;
        int action_id;
        std::unordered_map<int, int> kv_info;
    };

    /**
     * Transfer changes in the nyan backend for the user. This information is server->client only.
     */
    struct nyanchange {
        std::string nyan_patch; //TODO Actual changes in Nyan
    };

    /**
     * State update
     * - used for non-dynamic objects (buildings, trees, ...)
     */
    struct static_state {
        int id;
        int x,y;
        std::unordered_map<int, int> kv_state;
    };

    /**
     * one step in the trajectory of the unit.
     * This can be used to extrapolate what a unit will do.
     */
    struct trajectory_element {
        int x, y;
        int action;
    };

    /**
     * State update
     * - used for dynamic objects (units, ...)
     * The same as the static update but includes trajectories
     */
    struct dynamic_state {
        int id;
        int x,y;
        std::unordered_map<int, int> kv_state;
        std::deque<trajectory_element> trajectory;
    };

    std::deque<input> inputs;
    std::deque<nyanchange> nyan_changes;
    std::deque<static_state> static_states;
    std::deque<dynamic_state> dynamic_states;

    /** @brief Clear all containing data and reset the Packet to factory state
     *
     * @return void
     *
     */
    void clear();

    size_t serialized_size();

private:
    int _server_frame;

    void on_wire(SerializerStream &stream);

    void on_wire(SerializerStream &stream, int *data);
    void on_wire(SerializerStream &stream, std::string *data);

    void on_wire(SerializerStream &stream, input *data);
    void on_wire(SerializerStream &stream, nyanchange *data);
    void on_wire(SerializerStream &stream, static_state *data);
    void on_wire(SerializerStream &stream, trajectory_element *data);
    void on_wire(SerializerStream &stream, dynamic_state *data);

    void map_on_wire(SerializerStream &stream, std::unordered_map<int, int> *data);

    template <typename _Base>
    void deque_on_wire(SerializerStream &stream, std::deque<_Base> *data) {
        int cnt = data->size();
        on_wire(stream, &cnt);

        if (stream.is_write()) {
            for (auto it = data->begin(); it != data->end(); ++it) {
                on_wire(stream, &*it);
            }
        } else {
            for (int i = 0; i < cnt; ++i) {
                _Base p;
                on_wire(stream, &p);
                data->push_back(p);
            }
        }
    }

};

}
} //openage::network
