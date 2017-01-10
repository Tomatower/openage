// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#pragma once

#include <functional>
#include <map>
#include <mutex>

#include "packet.h"
#include "../log/named_logsource.h"

namespace openage {

class Player;

namespace network {


/** @brief The WireManager class assists in creating packets to transmit over the network
 *
 * The Packet Builder helps with the seperation into reliable/unreliable and urgent/continuous information
 * Nyan changes and Input are written directly into the packet. Upon a call to build it starts pulling dynamic and static
 * object updates into the packet until it is full.
 */
class WireManager {
public:
	typedef std::function<bool(Packet::object_state*)> object_provider_t;

	WireManager(openage::log::NamedLogSource &, object_provider_t, int initial_mtu = 1500);

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

	/** @brief handle the next packet
	 *
	 * create a packet, fill it from inputs, nyan changes and fill it up
	 * until MTU is reached from object_provider
	 *
	 * @param strean stream to serialize to
	 */
	void to_wire(SerializerStream &stream);

	/** @brief handle the next packet
	 *
	 * Read Data from the packet and store it in the jitter buffer.
	 * the Packet may now be read.
     *
	 * @param strean stream to serialize to
	 */

	void from_wire(SerializerStream &stream);


	uint16_t next_frame();

	std::shared_ptr<Packet> get_frame_packet(uint16_t frame);

	void release_packet(std::shared_ptr<Packet> packet);

private:
	void confirm_remote(uint16_t remote_frame);
	std::shared_ptr<Packet> get_packet(uint16_t frame_id);


	openage::log::NamedLogSource &logsink;
	object_provider_t object_provider;

	int mtu;
	uint16_t last_confirmed_frame = 0;
	uint16_t local_frame = 0;

	//TODO make unordered?
	std::map<int, std::shared_ptr<Packet> > jitter_buffer;

	std::mutex pool_mutex;
	std::deque<std::shared_ptr<Packet> > pool;

	template<typename _type>
	struct reliable {
		uint16_t remote_frame;
		std::deque<_type> data;
	};

	std::deque<reliable<Packet::input> > pending_inputs;
	std::deque<reliable<Packet::nyanchange> > pending_nyan;

	template <typename _type>
	void confirm_reliables(std::deque<reliable<_type> > &d,
							uint16_t last_received_frame) {
		uint16_t max_frame = last_received_frame + 1;
		while (last_received_frame != max_frame
				&& d.front().remote_frame == last_received_frame) {
			d.pop_front();
			last_received_frame --;
		}
	}
};


}
} //openage::network
