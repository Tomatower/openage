// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "wiremanager.h"
#include "../log/log.h"
#include "serializerstream.h"


namespace openage {
namespace network {

WireManager::WireManager(openage::log::NamedLogSource &log, object_provider_t provider, int initial_mtu):
	logsink{log},
	object_provider(provider),
	mtu(initial_mtu)
{

}


void WireManager::nyan_change(const std::string& patch)
{
	if (pending_nyan.empty()
		|| pending_nyan.back().remote_frame != local_frame) {
		reliable<Packet::nyanchange> r;
		pending_nyan.push_back(r);
	}

	Packet::nyanchange nc;
	nc.nyan_patch = patch;
	pending_nyan.back().data.push_back(nc);
}


void WireManager::input(const Packet::input& input) {
	if (pending_inputs.empty()
		|| pending_inputs.back().remote_frame != local_frame) {
		reliable<Packet::input> r;
		pending_inputs.push_back(r);
	}

	pending_inputs.back().data.push_back(input);
}


// TODO Observe packet size and optimize around the MTU
void WireManager::to_wire(SerializerStream &s) {
	uint16_t frame = next_frame();
	s.on_wire(&frame);
	s.on_wire(&last_confirmed_frame);
	s.deque_on_wire(&pending_inputs);
	s.deque_on_wire(&pending_nyan);

	int32_t data = 0;
	int32_t *datastream_counter = (int32_t *)s.write_observable((uint8_t *)&data, sizeof(data));
	bool obj_generator_state = true;
	while (obj_generator_state && (int32_t)s.size() < mtu) {
		Packet::object_state state;
		obj_generator_state = object_provider(&state);
		if (obj_generator_state) {
			s.on_wire(&state);
			(*datastream_counter)++;
		}
	}
}


void WireManager::from_wire(SerializerStream &s) {
	uint16_t frame;
	s.on_wire(&frame);
	std::shared_ptr<Packet> packet = get_packet(frame);

	s.on_wire(&packet->remote_frame);
	confirm_remote(packet->remote_frame);

	s.deque_on_wire(&pending_inputs);
	s.deque_on_wire(&pending_nyan);
	s.deque_on_wire(&packet->object_states);

	for (reliable<Packet::input> rp : pending_inputs) {
		std::shared_ptr<Packet> jitter_packet = get_packet(rp.remote_frame);
		if (jitter_packet->inputs.empty()) {
			jitter_packet->inputs = rp.data;
		} //TODO: make it possible to merge different data sets
	}

	for (reliable<Packet::nyanchange> &rp : pending_nyan) {
		std::shared_ptr<Packet> jitter_packet = get_packet(rp.remote_frame);
		if (jitter_packet->nyan_changes.empty()) {
			jitter_packet->nyan_changes = rp.data;
		} //TODO: make it possible to merge different data sets
	}

}


void WireManager::confirm_remote(uint16_t remote_frame) {
	last_confirmed_frame = remote_frame;

	confirm_reliables<Packet::input>(pending_inputs, remote_frame);
	confirm_reliables<Packet::nyanchange>(pending_nyan, remote_frame);
}

std::shared_ptr<Packet> WireManager::get_packet(uint16_t frame_id) {
	auto it = jitter_buffer.find(frame_id);
	if (it != jitter_buffer.end()) {
		return it->second;
	} else {
		if (pool.empty()) {
			auto packet = std::shared_ptr<Packet>(new Packet());
			jitter_buffer.insert(std::make_pair(frame_id, packet));
			return packet;
		} else {
			std::lock_guard<std::mutex> lock(pool_mutex);
			std::shared_ptr<Packet> packet = pool.front();
			pool.pop_front();
			jitter_buffer.insert(std::make_pair(frame_id, packet));
			return packet;
		}
	}
}


uint16_t WireManager::next_frame() {
	return ++local_frame;
}


std::shared_ptr<Packet> WireManager::get_frame_packet(int frame) {
	auto it = jitter_buffer.find(frame);
	if (it != jitter_buffer.end()) {
		return it->second;
	} else {
		std::stringstream ss;
		ss << "Jitter-buffer underflow. Did not receive Frame for ID " << frame
		   << " " << "We only have : { ";
		for (auto it : jitter_buffer) {
			ss << it.first << ", ";
		}
		ss << "}";

		logsink.log(MSG(warn) << ss.str());
		return std::shared_ptr<Packet>(nullptr);
	}
}


}} //openage::network
