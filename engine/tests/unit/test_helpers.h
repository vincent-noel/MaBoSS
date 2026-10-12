/*
  Small helpers shared by the MaBoSS unit tests.
*/

#pragma once

#include <memory>
#include <string>

#include "BNException.h"
#include "Network.h"

namespace maboss_test {

// Parses a .bnd body given inline. Network::parseExpression drives the same
// bison parser as Network::parse does, without needing a temporary file.
inline std::unique_ptr<Network> parse_network(const std::string& bnd) {
  std::unique_ptr<Network> network(new Network());
  if (network->parseExpression(bnd.c_str()) != 0) {
    throw BNException("test network failed to parse: " + bnd);
  }
  return network;
}

// A three-node network with no interesting dynamics, used wherever a test just
// needs some Node objects and a sized NetworkState.
inline std::unique_ptr<Network> parse_abc_network() {
  return parse_network(
    "node A { rate_up = 1.0; rate_down = 0.0; }\n"
    "node B { rate_up = 0.0; rate_down = 1.0; }\n"
    "node C { rate_up = 1.0; rate_down = 1.0; }\n"
  );
}

} // namespace maboss_test

