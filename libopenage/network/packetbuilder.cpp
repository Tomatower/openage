// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "packetbuilder.h"

#include "packet.h"

namespace openage {
namespace network {

PacketBuilder::PacketBuilder(static_state_provider s_provider, dynamic_state_provider d_provider) :
    static_provider (s_provider),
    dynamic_provider (d_provider),
    active_packet{new Packet()} {
}


void PacketBuilder::nyan_change(const std::string& patch) {
    Packet::nyanchange nc;
    nc.nyan_patch = patch;

    active_packet->nyan_changes.push_back(nc);

}


void PacketBuilder::input(const Packet::input& input) {
    active_packet->inputs.push_back(input);
}


std::unique_ptr<Packet> PacketBuilder::build(int mtu) {

    //While packet is smaller than mtu
    int last_pkg_size = 0;
    int last_size = active_packet->serialized_size();
    int cnt = 0;
    bool st_success = true;
    bool dy_success = true;

    //Alternate between adding dynamic and static packages.
    //If one of both is not available anymore, only add the other one.
    //If both are not available, break.
    //If the size of the packet exceedes the MTU, break.
    while ((st_success || dy_success) && active_packet->serialized_size() < (mtu - last_pkg_size)) {
        if (cnt % 2 == 0 && st_success) {
            Packet::static_state st;
            st_success = static_provider(&st);
        }

        if (cnt % 2 == 1 && dy_success) {
            Packet::dynamic_state dy;
            dy_success = dynamic_provider(&dy);
        }

        if (dy_success && st_success) {
            cnt++;
        } else if (dy_success) {
            cnt = 1;
        } else if (st_success) {
            cnt = 0;
        } else break;
    }

    auto new_pkt = std::unique_ptr<Packet>(new Packet());
    active_packet.swap(new_pkt);
    return new_pkt;
}


void PacketBuilder::clear() {
    active_packet->clear();
}


}
} // openage::network
