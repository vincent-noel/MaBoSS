/*
  The network state representation.

  NetworkState wraps one of three interchangeable implementations chosen at
  compile time by MAXNODES/DYNBITSET (see NetworkState_Impl.h). These tests are
  written against the public NetworkState interface so that they run unchanged
  for every variant, which is exactly what is needed to keep the three honest
  as the representation is reworked.
*/

#include <doctest/doctest.h>

#include "NetworkState.h"
#include "test_helpers.h"

using maboss_test::parse_abc_network;

TEST_SUITE("bitset") {

TEST_CASE("a fresh state has every node down") {
  std::unique_ptr<Network> network = parse_abc_network();

  NetworkState state;
  state.reset();

  for (const Node* node : network->getNodes()) {
    CAPTURE(node->getLabel());
    CHECK(state.getNodeState(node) == false);
  }
}

TEST_CASE("set() raises every node and reset() lowers them again") {
  std::unique_ptr<Network> network = parse_abc_network();

  NetworkState state;
  state.set();
  for (const Node* node : network->getNodes()) {
    CAPTURE(node->getLabel());
    CHECK(state.getNodeState(node) == true);
  }

  state.reset();
  for (const Node* node : network->getNodes()) {
    CAPTURE(node->getLabel());
    CHECK(state.getNodeState(node) == false);
  }
}

TEST_CASE("nodes are set independently") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");
  const Node* b = network->getNode("B");
  const Node* c = network->getNode("C");

  NetworkState state;
  state.reset();
  state.setNodeState(b, true);

  CHECK(state.getNodeState(a) == false);
  CHECK(state.getNodeState(b) == true);
  CHECK(state.getNodeState(c) == false);

  state.setNodeState(b, false);
  CHECK(state.getNodeState(b) == false);
}

TEST_CASE("flipState toggles exactly one node") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");
  const Node* b = network->getNode("B");

  NetworkState state;
  state.reset();

  state.flipState(a);
  CHECK(state.getNodeState(a) == true);
  CHECK(state.getNodeState(b) == false);

  state.flipState(a);
  CHECK(state.getNodeState(a) == false);

  // Flipping twice is the identity, for every node, from a full state too.
  state.set();
  for (const Node* node : network->getNodes()) {
    state.flipState(node);
    state.flipState(node);
  }
  for (const Node* node : network->getNodes()) {
    CAPTURE(node->getLabel());
    CHECK(state.getNodeState(node) == true);
  }
}

TEST_CASE("operator& masks nodes out") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");
  const Node* b = network->getNode("B");
  const Node* c = network->getNode("C");

  NetworkState state;
  state.set();

  NetworkState mask;
  mask.reset();
  mask.setNodeState(a, true);
  mask.setNodeState(c, true);

  const NetworkState masked = state & mask;
  CHECK(masked.getNodeState(a) == true);
  CHECK(masked.getNodeState(b) == false);
  CHECK(masked.getNodeState(c) == true);

  // The operands are untouched.
  CHECK(state.getNodeState(b) == true);
  CHECK(mask.getNodeState(b) == false);
}

TEST_CASE("copying a state copies its values") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");
  const Node* b = network->getNode("B");

  NetworkState original;
  original.reset();
  original.setNodeState(a, true);

  NetworkState copy(original);
  CHECK(copy.getNodeState(a) == true);
  CHECK(copy.getNodeState(b) == false);

  NetworkState assigned;
  assigned.reset();
  assigned = original;
  CHECK(assigned.getNodeState(a) == true);
  CHECK(assigned.getNodeState(b) == false);
}

TEST_CASE("getNetworkStates yields the one underlying state") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");

  NetworkState state;
  state.reset();
  state.setNodeState(a, true);

  std::set<NetworkState_Impl>* states = state.getNetworkStates();
  REQUIRE(states != nullptr);
  CHECK(states->size() == 1);
  delete states;

  CHECK(NetworkState::isPopState() == false);
}

#ifdef USE_DYNAMIC_BITSET
// MBDynBitset is only compiled into the library for dynamic-bitset builds, so
// these cases exist only there. Its per-thread allocator is set up once in
// main.cc.

TEST_CASE("MBDynBitset stores and reads back bits") {
  MBDynBitset bits(128);
  bits.reset();
  CHECK(bits.none());

  bits.set(0, true);
  bits.set(64, true);
  bits.set(127, true);

  CHECK(bits.test(0));
  CHECK(bits.test(64));
  CHECK(bits.test(127));
  CHECK_FALSE(bits.test(1));
  CHECK_FALSE(bits.none());

  bits.flip(0);
  CHECK_FALSE(bits.test(0));

  bits.set(0, false);
  bits.set(64, false);
  bits.set(127, false);
  CHECK(bits.none());
}

TEST_CASE("MBDynBitset set() raises every bit it holds") {
  MBDynBitset bits(8);
  bits.set();
  CHECK(bits.toString() == "11111111");

  bits.reset();
  CHECK(bits.toString() == "00000000");

  // toString() is written most-significant bit first.
  bits.set(0, true);
  CHECK(bits.toString() == "00000001");
}

TEST_CASE("MBDynBitset assignment deep-copies") {
  MBDynBitset source(64);
  source.reset();
  source.set(3, true);

  MBDynBitset assigned(64);
  assigned.reset();
  assigned = source;

  CHECK(assigned.test(3));

  // Writing through one must not be visible through the other. The copy
  // constructor deliberately shares the buffer instead (see the refcounting in
  // MBDynBitset.h); only assignment is a deep copy today. That asymmetry is
  // pinned here so a future rework of the type has to face it explicitly.
  source.set(4, true);
  CHECK(source.test(4));
  CHECK_FALSE(assigned.test(4));
}

TEST_CASE("MBDynBitset compares by value") {
  MBDynBitset a(64);
  a.reset();
  a.set(9, true);

  MBDynBitset b(64);
  b.reset();
  b.set(9, true);

  CHECK(a == b);

  b.set(10, true);
  CHECK_FALSE(a == b);
}

#endif // USE_DYNAMIC_BITSET

} // TEST_SUITE
