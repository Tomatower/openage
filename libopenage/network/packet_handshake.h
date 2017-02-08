// Copyright 2016-2017 the openage authors. See copying.md for legal info.

#pragma once

namespace openage {
namespace network {

// 
// 1. C->S Packet: All info to server
// 2. S->C Packet: random number token, hmecinfo)
//
//
// Build the complete library on top of enet: 
//
// Channel 1: Static Game Changes (like Handshaking, Nyan)
// Channel 2: UI Connections (like Chat) 
// Unreliable: Game update channel
//


