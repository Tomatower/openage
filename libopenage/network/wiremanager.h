// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <functional>

#include "packet.h"
#include "../log/named_logsource.h"

namespace openage {
namespace network {


/** @brief The WireManager class assists in creating packets to transmit over the network
 *
 * The Packet Builder helps with the seperation into reliable/unreliable and urgent/continuous information
 * Nyan changes and Input are written directly into the packet. Upon a call to build it starts pulling dynamic and static
 * object updates into the packet until it is full.
 */
class WireManager {
public:
	typedef std::function<bool(Packet::object_state*)> object_provider;

	WireManager(openage::log::NamedLogSource &, object_provider);

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


	void to_wire(std::vector<int8_t> &buffer, int *size);
	void from_wire(std::vector<int8_t> &buffer, int size);

	void confirm(int16_t remote_frame);

	int16_t next_frame();

	std::shared_ptr<Packet> get_frame_packet(int frame);

private:
	object_provider object_provider;
	openage::log::NamedLogSource &logsink;


	std::deque<std::shared_ptr<Packet>> jitter_buffer;
	std::deque<std::shared_ptr<Packet>> pool;

	template<typename _Packet>
	struct reliable {
		int16_t server_frame;
		std::deque<_Packet> inputs;
	};

	std::vector<uint8_t> working_buffer;

	std::deque<reliable<Packet::input>> pending_inputs;
	std::deque<reliable<Packet::nyanchange>> pending_nyan;
};


}
} //openage::network
