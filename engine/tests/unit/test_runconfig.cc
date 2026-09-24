/*
  Run configuration (.cfg) parsing.

  RunConfig is the other half of a MaBoSS model: it carries the simulation
  parameters, the symbol values a .bnd file refers to, and the per-node
  attributes (istate, is_internal). It is parsed by a second bison grammar
  sharing the same global-state style as the network one.
*/

#include <doctest/doctest.h>

#include "BNException.h"
#include "Network.h"
#include "RunConfig.h"
#include "test_helpers.h"

using maboss_test::parse_abc_network;

TEST_SUITE("runconfig") {

TEST_CASE("simulation parameters are read back") {
  std::unique_ptr<Network> network = parse_abc_network();
  RunConfig config;

  REQUIRE(config.parseExpression(network.get(),
    "time_tick = 0.25;"
    "max_time = 10;"
    "sample_count = 500;"
    "thread_count = 3;"
    "discrete_time = 0;"
    "statdist_traj_count = 17;"
  ) == 0);

  CHECK(config.getTimeTick() == doctest::Approx(0.25));
  CHECK(config.getMaxTime() == doctest::Approx(10.));
  CHECK(config.getSampleCount() == 500);
  CHECK(config.getThreadCount() == 3);
  CHECK(config.isDiscreteTime() == false);
  CHECK(config.getStatDistTrajCount() == 17);
}

TEST_CASE("is_internal hides a node from the output") {
  std::unique_ptr<Network> network = parse_abc_network();
  RunConfig config;

  REQUIRE(config.parseExpression(network.get(),
    "A.is_internal = TRUE;"
    "B.is_internal = FALSE;"
  ) == 0);

  CHECK(network->getNode("A")->isInternal());
  CHECK_FALSE(network->getNode("B")->isInternal());
  CHECK_FALSE(network->getNode("C")->isInternal());
}

TEST_CASE("symbol values declared in the config reach the network") {
  std::unique_ptr<Network> network = maboss_test::parse_network(
    "node A { rate_up = $up; rate_down = $down; }\n"
  );
  RunConfig config;

  REQUIRE(config.parseExpression(network.get(), "$up = 2.5; $down = 0.5;") == 0);

  SymbolTable* symbols = network->getSymbolTable();
  CHECK(symbols->getSymbolValue(symbols->getSymbol("$up")) == doctest::Approx(2.5));
  CHECK(symbols->getSymbolValue(symbols->getSymbol("$down")) == doctest::Approx(0.5));
}

TEST_CASE("the random generator is selected by the config") {
  std::unique_ptr<Network> network = parse_abc_network();

  SUBCASE("default is rand48") {
    RunConfig config;
    REQUIRE(config.parseExpression(network.get(), "seed_pseudorandom = 100;") == 0);
    CHECK(config.getRandomGeneratorFactory()->getName() == "rand48");
    CHECK(config.getSeedPseudoRandom() == 100);
  }

  SUBCASE("glibc can be requested") {
    RunConfig config;
    REQUIRE(config.parseExpression(network.get(), "use_glibcrandgen = TRUE;") == 0);
    CHECK(config.getRandomGeneratorFactory()->getName() == "glibc");
  }

  SUBCASE("mersenne twister can be requested") {
    RunConfig config;
    REQUIRE(config.parseExpression(network.get(), "use_mtrandgen = TRUE;") == 0);
    CHECK(config.getRandomGeneratorFactory()->getName() == "mt19937");
  }
}

TEST_CASE("setParameter overrides a parsed value") {
  std::unique_ptr<Network> network = parse_abc_network();
  RunConfig config;

  REQUIRE(config.parseExpression(network.get(), "sample_count = 10;") == 0);
  CHECK(config.getSampleCount() == 10);

  config.setParameter("sample_count", 99);
  CHECK(config.getSampleCount() == 99);
}

TEST_CASE("an unknown node attribute is rejected") {
  std::unique_ptr<Network> network = parse_abc_network();
  RunConfig config;

  CHECK_THROWS_AS(config.parseExpression(network.get(), "A.no_such_attribute = 1;"),
                  BNException);
}

TEST_CASE("an attribute on an undeclared node is rejected") {
  std::unique_ptr<Network> network = parse_abc_network();
  RunConfig config;

  CHECK_THROWS_AS(config.parseExpression(network.get(), "NoSuchNode.is_internal = TRUE;"),
                  BNException);
}

TEST_CASE("malformed config text is rejected") {
  std::unique_ptr<Network> network = parse_abc_network();
  RunConfig config;

  CHECK_THROWS(config.parseExpression(network.get(), "time_tick = ;"));
}

} // TEST_SUITE
