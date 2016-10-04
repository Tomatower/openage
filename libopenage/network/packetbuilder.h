// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <functional>

#include "network.h"
#include "packet.h"

namespace openage {
namespace network {

class Packet;

/** @brief The PacketBuilder class assists in creating packets to transmit over the network
 *
 * The Packet Builder helps with the seperation into reliable/unreliable and urgent/continuous information
 * Nyan changes and Input are written directly into the packet. Upon a call to build it starts pulling dynamic and static
 * object updates into the packet until it is full.
 */
class PacketBuilder {
public:
    typedef std::function<bool(Packet::static_state*)> static_state_provider;
    typedef std::function<bool(Packet::dynamic_state*)> dynamic_state_provider;

    /** @brief C'tor, assigns the polling functions
     *
     * @param  static_state_provider called, whenever a static object should be appended to the message
     * @param dynamic_state_provider called, whenever a dynamic object should be appended to the message
     *
     */
    PacketBuilder(static_state_provider , dynamic_state_provider );

    /** @brief append a nyan change
     *
     * Add the reliable information nyan_change to the packet
     *
     * @param patch const std::string&
     * @return void
     */
    void nyan_change(const std::string &patch);

    /** @brief append an input
     *
     * Add a input to the message
     *
     * @param input const Packet::input&
     * @return void
     *
     */
    void input(const Packet::input &input);

    /** @brief Create a packet from the reliable information and fill up with state data
     *
     * @param
     * @return std::unique_ptr<Packet>
     *
     */
    std::unique_ptr<Packet> build(int mtu);

    /** @brief
     *
     * @return void
     *
     */
    void clear();
private:
    static_state_provider static_provider;
    dynamic_state_provider dynamic_provider;

    std::unique_ptr<Packet> active_packet;
};

}
} //namespace openage::network
