// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "network.h"
#include "packet.h"
#include "packetbuilder.h"
#include "serializerstream.h"
#include "wiremanager.h"

#include "../testing/testing.h"

namespace openage {
namespace network {
namespace tests {

static bool operator == (Packet &a, Packet &b) {
	if (a.inputs.size() != b.inputs.size()) {
		//TESTFAILMSG("inputs size");
		return false;
	}

	for (auto ia = a.inputs.begin(), ib = b.inputs.begin();
	     ia != a.inputs.end() && ib != b.inputs.end();
	     ++ia, ++ib) {
		if (ia->player != ib->player
		    || ia->target_id != ib->target_id
		    || ia->action_id != ib->action_id) {
			//TESTFAILMSG("inputs");
			return false;
		}

		if (ia->kv_info != ib->kv_info) {
			//TESTFAILMSG("inputs kv");
			return false;
		}
	}

	if (a.nyan_changes.size() != b.nyan_changes.size()) {
		//TESTFAILMSG("nyan size");
		return false;
	}

	for (auto ia = a.nyan_changes.begin(), ib = b.nyan_changes.begin();
	     ia != a.nyan_changes.end() && ib != b.nyan_changes.end();
	     ++ia, ++ib) {
		if (ia->nyan_patch != ib->nyan_patch) {
			//TESTFAILMSG("nyan");
			return false;
		}
	}

	if (a.object_states.size() != b.object_states.size()) {
		//TESTFAILMSG("objects size");
		return false;
	}

	for (auto ia = a.object_states.begin(), ib = b.object_states.begin();
	     ia != a.object_states.end() && ib != b.object_states.end();
	     ++ia, ++ib) {
		if (ia->id != ib->id
		    || ia->x != ib->x
		    || ia->y != ib->y
		    || ia->kv_state != ib->kv_state) {
			TESTFAILMSG("objects");
			return false;
		}

		if (ia->trajectory.size() != ib->trajectory.size()) {
			TESTFAILMSG("trajectory size");
			return false;
		}
		for (auto iia = ia->trajectory.begin(), iib = ib->trajectory.begin();
		     iia != ia->trajectory.end() && iib != ib->trajectory.end();
		     ++iia, ++iib) {
			if (iia->x != iib->x
			    || iia->y != iib->y
			    || iia->action != iib->action) {
				TESTFAILMSG("trajectory");
				return false;
			}
		}
	}

	return true;
}


/** Generate a dummy packet with some random data stored inside (but always the same packet!) */
Packet generate_dummy_packet_long() {
	Packet p;
	for (int i = 0; i < 10; ++i) {
		Packet::input inp;
		inp.player = 1 * i;
		inp.target_id = 2 * i;
		inp.action_id = 3 * i + 3;
		inp.kv_info[123] = 234 * i;
		inp.kv_info[0x42] = 0x24 * i / 2;
		p.inputs.push_back(inp);
	}

	{
		Packet::nyanchange nc;
		nc.nyan_patch = "IAmATomatoe";
		p.nyan_changes.push_back(nc);
	}

	for (int i = 0; i < 10; ++i) {
		Packet::object_state ds;
		ds.x = 1;
		ds.y = i;
		ds.id = i + 5;

		for (int o = i; o < 15; ++o) {
			Packet::trajectory_element te;
			te.x = i;
			te.y = o;
			te.action = i*o;

			ds.trajectory.push_back(te);
		}
		p.object_states.push_back(ds);
		//EMPTY KV MAP
	}

	return p;
}

Packet generate_dummy_packet() {
	Packet p;
	for (int i = 0; i < 1; ++i) {
		Packet::input inp;
		inp.player = 1 * i;
		inp.target_id = 2 * i;
		inp.action_id = 3 * i + 3;
		inp.kv_info[123] = 234 * i;
		inp.kv_info[0x42] = 0x24 * i / 2;
		p.inputs.push_back(inp);
	}

	{
		Packet::nyanchange nc;
		nc.nyan_patch = ".";
		p.nyan_changes.push_back(nc);
	}

	for (int i = 0; i < 1; ++i) {
		Packet::object_state ds;
		ds.x = 1;
		ds.y = i;
		ds.id = i + 5;

		for (int o = i; o < 0; ++o) {
			Packet::trajectory_element te;
			te.x = i;
			te.y = o;
			te.action = i*o;

			ds.trajectory.push_back(te);
		}
		p.object_states.push_back(ds);
		//EMPTY KV MAP
	}

	return p;
}


void test_test() {
	Packet p1 = generate_dummy_packet();
	Packet p2 = generate_dummy_packet();

	if (!(p1 == p2)) {
		TESTFAILMSG("p1 == p2");
	}

	log::message m = ERR << "Message";

	p1.inputs.front().player = 2;
	if ((p1 == p2)) {
		TESTFAILMSG("p1 != p2");
	}
}


void serializer_test() {
	// Test serialize / deserialize in the same stream
	log::NamedLogSource log("SerializerTest");
	{
		Packet pin = generate_dummy_packet();
		SerializerStream ss(log);

		Packet pout;
		pin.to_stream(ss);
		pout.from_stream(ss);

		if (!(pin == pout)) {
			TESTFAILMSG("pin == pout");
		}
	}
	// Test serialize / deserialize with copying the data buffer (i.e. simulate a network)
	{
		std::vector<int8_t> buffer;
		Packet pin = generate_dummy_packet();
		Packet pout;
		{
			SerializerStream ss(log);
			pin.to_stream(ss);

			ss.get_data(buffer);
		}

		{
			SerializerStream ss(log);
			ss.set_data(buffer);
			pout.from_stream(ss);
		}

		if (!(pin == pout)) {
			TESTFAILMSG("pin == pout with data copy");
		}
	}
}

class dummy_object_state_provider {
	int cnt, start, max;
public:
	dummy_object_state_provider(int start, int max) : cnt {start}, start{start}, max{max}
	{};

	void reset() {
		cnt = start;
	}

	bool operator() (Packet::object_state* s) {
		return false;
//		s->id = cnt;
//		cnt ++;
//		return cnt < max;
	}
};


void wiremanager_singleframe_test() {
	dummy_object_state_provider provider(0, 5);
	log::NamedLogSource log("SingleFrameTest");

	log.log(INFO << "SingleFrameTest");

	WireManager wm1(log, provider);
	{
		Packet::input i;
		i.player = 5;
		wm1.input(i);
	}

	//The Serializer stream poses as "network"
	SerializerStream ss(log);
	ss.set_write_mode(true);
	wm1.to_wire(ss);

	dummy_object_state_provider provider2(200, 5);
	WireManager wm2(log, provider2);
    ss.set_write_mode(false);
    wm2.from_wire(ss);

	auto p1 = *wm1.get_frame_packet(0);
	auto p2 = *wm2.get_frame_packet(0);

	if (!(p1 == p2)) {
		TESTFAILMSG("pin == pout with wire manager");
	}
}

void wiremanager_multiframe_test() {

}

void handshake_test() {
	//TESTFAILMSG("NOT IMPLEMENTED YET");
	Interface server("localhost", 9001, InterfaceType::SERVER);

	Interface client("localhost", 9001, InterfaceType::CLIENT);

	//TODO create a timeout (e.g. 1 second?)
	while (server.connected_players().empty()) {
		server.game_loop();
		client.game_loop();
	}
}

void game_mode_test() {

}

void test() {
	test_test();
	serializer_test();
	wiremanager_singleframe_test();
	wiremanager_multiframe_test();
	handshake_test();
	game_mode_test();
}

}
}
} // openage::network::tests
