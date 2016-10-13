// Copyright 2016-2016 the openage authors. See copying.md for legal info.

#include "wiremanager.h"

namespace openage {
namespace network {

WireManager::WireManager(openage::log::NamedLogSource &log, object_provider provider)
{

}


void WireManager::nyan_change(const std::string& patch)
{

}


void WireManager::input(const Packet::input& input)
{

}


void WireManager::to_wire(std::vector<int8_t>& buffer, int* size)
{

}


void WireManager::from_wire(std::vector<int8_t>& buffer, int size)
{

}


void WireManager::confirm(int16_t remote_frame)
{

}


int16_t WireManager::next_frame()
{

}


std::shared_ptr<Packet> WireManager::get_frame_packet(int frame)
{

}


}} //openage::network
