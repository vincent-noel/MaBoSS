/*
  How the simulation engines consume random draws.

  The engines are driven here by a scripted generator, so that a test can hand
  them exactly the edge-case draws that a real generator produces only once in
  billions of calls.
*/

#include <doctest/doctest.h>

#include <vector>

#include "RunConfig.h"
#include "engines/StochasticSimulationEngine.h"
#include "test_helpers.h"

using namespace MaBoSS;

using maboss_test::parse_network;

namespace {

// Returns the scripted values from generate(), then 0.5 once they run out.
class ScriptedRandomGenerator final : public RandomGenerator {
  std::vector<double> values;
  size_t next = 0;

 public:
  explicit ScriptedRandomGenerator(std::vector<double> _values) : values(std::move(_values)) { }

  std::string getName() const override { return "scripted"; }
  bool isPseudoRandom() const override { return true; }
  unsigned int generateUInt32() override { return 0; }
  double generate() override { return next < values.size() ? values[next++] : 0.5; }
  void setSeed(int) override { }
};

} // namespace

TEST_SUITE("simulation") {

// A draw at the top of the range used to run the selection loop off the end
// and pick the last node, even one whose rate is 0: B flipped although
// nothing in the model lets it. Rounding can do the same with a draw just
// below 1.
TEST_CASE("the transition draw never picks a node that cannot flip") {
  std::unique_ptr<Network> network = parse_network(
    "node A { rate_up = 1.0; rate_down = 0.0; }\n"
    "node B { rate_up = 0.0; rate_down = 0.0; }\n"
  );
  const Node* a = network->getNode("A");
  const Node* b = network->getNode("B");

  RunConfig config;
  StochasticSimulationEngine engine(network.get(), &config, 1);
  delete engine.random_generator;
  // The waiting time (-log(0.5) < 1), then the transition draw at the top.
  engine.random_generator = new ScriptedRandomGenerator({0.5, 1.0});
  engine.setMaxTime(1.0);
  engine.setDiscreteTime(false);

  NetworkState initial_state;
  initial_state.reset();
  NetworkState final_state = engine.run(initial_state);

  CHECK(final_state.getNodeState(a) == true);
  CHECK(final_state.getNodeState(b) == false);
}

} // TEST_SUITE
