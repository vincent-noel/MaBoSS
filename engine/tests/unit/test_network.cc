/*
  Network parsing and compilation.

  Network doubles as the node factory and owns the whole object graph (nodes,
  expressions, initial-state groups, symbol table) through raw pointers, with a
  hand-written copy constructor. These tests fix the observable behaviour of
  that graph so it can be moved onto smart pointers without silent drift.
*/

#include <doctest/doctest.h>

#include <algorithm>

#include "BNException.h"
#include "Network.h"
#include "test_helpers.h"

using maboss_test::parse_abc_network;
using maboss_test::parse_network;

namespace {

std::vector<std::string> labels_of(const std::vector<Node*>& nodes) {
  std::vector<std::string> labels;
  for (const Node* node : nodes) {
    labels.push_back(node->getLabel());
  }
  std::sort(labels.begin(), labels.end());
  return labels;
}

} // namespace

TEST_SUITE("network") {

TEST_CASE("parsing declares every node once, in order") {
  std::unique_ptr<Network> network = parse_abc_network();

  REQUIRE(network->getNodeCount() == 3);

  const std::vector<Node*>& nodes = network->getNodes();
  REQUIRE(nodes.size() == 3);
  CHECK(nodes[0]->getLabel() == "A");
  CHECK(nodes[1]->getLabel() == "B");
  CHECK(nodes[2]->getLabel() == "C");

  // Indices agree with the position in getNodes(), which is what every
  // NetworkState bit position relies on.
  for (NodeIndex i = 0; i < nodes.size(); ++i) {
    CHECK(nodes[i]->getIndex() == i);
    CHECK(network->getNode(i) == nodes[i]);
  }
}

TEST_CASE("nodes are reachable by label") {
  std::unique_ptr<Network> network = parse_abc_network();

  CHECK(network->getNode("A")->getLabel() == "A");
  CHECK(network->isNodeDefined("A"));
  CHECK_THROWS_AS(network->getNode("Nonexistent"), BNException);
}

TEST_CASE("input nodes are the ones with no rates of their own") {
  // Despite the comment on Node::isInputNode(), "input node" does not mean
  // "does not depend on other nodes": it means the node declares neither
  // logic, rate_up nor rate_down, so the engine never transitions it and its
  // value stays at its initial state.
  std::unique_ptr<Network> network = parse_network(
    "node Fixed  { }\n"
    "node Signal { rate_up = 0.1; rate_down = 0.2; }\n"
    "node Target { rate_up = Signal ? 1.0 : 0.0; rate_down = Signal ? 0.0 : 1.0; }\n"
  );

  CHECK(labels_of(network->getInputNodes()) == std::vector<std::string>{"Fixed"});
  CHECK(labels_of(network->getNonInputNodes()) == std::vector<std::string>{"Signal", "Target"});
  CHECK(labels_of(network->getNodes()) == std::vector<std::string>{"Fixed", "Signal", "Target"});

  CHECK(network->getNode("Fixed")->isInputNode());
  CHECK_FALSE(network->getNode("Signal")->isInputNode());
  CHECK_FALSE(network->getNode("Target")->isInputNode());
}

TEST_CASE("logic, rate_up and rate_down are attached to the node") {
  std::unique_ptr<Network> network = parse_network(
    "node A { logic = !A; rate_up = @logic ? 1.0 : 0.0; rate_down = @logic ? 0.0 : 1.0; }\n"
  );

  const Node* a = network->getNode("A");
  CHECK(a->getLogicalInputExpression() != nullptr);
  CHECK(a->getRateUpExpression() != nullptr);
  CHECK(a->getRateDownExpression() != nullptr);
}

TEST_CASE("a node description survives parsing") {
  std::unique_ptr<Network> network = parse_network(
    "node A { description = \"the first node\"; rate_up = 1.0; rate_down = 1.0; }\n"
  );

  CHECK(network->getNode("A")->getDescription() == "the first node");
}

TEST_CASE("nodes are reported by default") {
  // is_internal is a runconfig (.cfg) attribute, not a .bnd one -- see
  // test_runconfig.cc for the parsing side.
  std::unique_ptr<Network> network = parse_abc_network();

  for (const Node* node : network->getNodes()) {
    CAPTURE(node->getLabel());
    CHECK_FALSE(node->isInternal());
  }
}

TEST_CASE("symbols used in rates land in the symbol table") {
  std::unique_ptr<Network> network = parse_network(
    "node A { rate_up = $up; rate_down = $down; }\n"
  );

  SymbolTable* symbols = network->getSymbolTable();
  REQUIRE(symbols != nullptr);
  CHECK(symbols->getSymbol("$up") != nullptr);
  CHECK(symbols->getSymbol("$down") != nullptr);
  CHECK(symbols->getSymbol("$absent") == nullptr);
}

TEST_CASE("toString round-trips through the parser") {
  std::unique_ptr<Network> original = parse_network(
    "node A { logic = !B; rate_up = @logic ? 1.5 : 0.0; rate_down = @logic ? 0.0 : 2.5; }\n"
    "node B { rate_up = 0.25; rate_down = 0.75; }\n"
  );

  const std::string printed = original->toString();

  std::unique_ptr<Network> reparsed = parse_network(printed);
  CHECK(reparsed->getNodeCount() == original->getNodeCount());
  CHECK(labels_of(reparsed->getNodes()) == labels_of(original->getNodes()));

  // Note that reparsed->toString() is *not* equal to `printed`: each round trip
  // adds a layer of parentheses around the rate expressions. See the matching
  // case in test_expressions.cc.
  for (const Node* node : reparsed->getNodes()) {
    CAPTURE(node->getLabel());
    CHECK(node->getRateUpExpression() != nullptr);
    CHECK(node->getRateDownExpression() != nullptr);
  }
}

TEST_CASE("a syntactically invalid network is rejected") {
  CHECK_THROWS(parse_network("node A { rate_up = ; }\n"));
}

TEST_CASE("referring to an undeclared node is an error") {
  // Undeclared identifiers in a logic expression must not silently become a
  // new node with default rates.
  CHECK_THROWS_AS(
    parse_network("node A { logic = Undeclared; rate_up = @logic ? 1 : 0; rate_down = 1; }\n"),
    BNException
  );
}

TEST_CASE("copying a network yields an independent object graph" * doctest::skip()) {
  // KNOWN BUG, and the reason this case is skipped rather than merely failing:
  // running it segfaults and takes the whole test binary with it.
  //
  // Network::operator=() shallow-copies node_map, nodes, input_nodes,
  // non_input_nodes and symbol_table -- all raw pointers -- while
  // ~Network() deletes every Node in node_map and the SymbolTable. Copying a
  // Network and letting both copies die therefore double-frees each node.
  // Network::Network(const Network&) delegates to operator=() without running
  // the default constructor body, so the copy's istate_group_list is never
  // initialised either, and the destructor dereferences that indeterminate
  // pointer.
  //
  // Nothing in the engine copies a Network today, but the copy constructor is
  // public and PopNetwork's copy constructor forwards to it. Un-skip this case
  // once ownership moves to smart pointers.
  std::unique_ptr<Network> original = parse_abc_network();

  Network copy(*original);
  CHECK(copy.getNodeCount() == original->getNodeCount());
  CHECK(labels_of(copy.getNodes()) == labels_of(original->getNodes()));

  for (size_t i = 0; i < copy.getNodes().size(); ++i) {
    CHECK(copy.getNodes()[i] != original->getNodes()[i]);
  }
}

} // TEST_SUITE
