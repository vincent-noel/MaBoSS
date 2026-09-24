/*
  Expression parsing and evaluation.

  The expression tree is the part of the engine most exposed to a smart-pointer
  conversion: every node is heap-allocated, cloned through a raw clone(), and
  constant-folded through cloneAndShrink(). These tests cover the arithmetic,
  the operator precedence and the folding, so the tree can be rebuilt without
  changing what a .bnd file means.
*/

#include <doctest/doctest.h>

#include <cmath>

#include "BNException.h"
#include "Expressions.h"
#include "Network.h"
#include "NetworkState.h"
#include "test_helpers.h"

using maboss_test::parse_abc_network;
using maboss_test::parse_network;

namespace {

// Parses a standalone expression against `network` and evaluates it in `state`.
double eval_in(Network& network, const std::string& text, const NetworkState& state) {
  Expression* expr = network.parseSingleExpression(text.c_str());
  if (expr == nullptr) {
    throw BNException("could not parse expression: " + text);
  }
  const double value = expr->eval(nullptr, state);
  delete expr;
  return value;
}

// Evaluates an expression that does not read any node.
double eval_const(Network& network, const std::string& text) {
  NetworkState state;
  state.reset();
  return eval_in(network, text, state);
}

} // namespace

TEST_SUITE("expressions") {

TEST_CASE("arithmetic follows the usual precedence") {
  std::unique_ptr<Network> network = parse_abc_network();

  CHECK(eval_const(*network, "1 + 2") == doctest::Approx(3.));
  CHECK(eval_const(*network, "2 * 3 + 1") == doctest::Approx(7.));
  CHECK(eval_const(*network, "1 + 2 * 3") == doctest::Approx(7.));
  CHECK(eval_const(*network, "(1 + 2) * 3") == doctest::Approx(9.));
  CHECK(eval_const(*network, "10 - 4 - 3") == doctest::Approx(3.));
  CHECK(eval_const(*network, "8 / 4 / 2") == doctest::Approx(1.));
  CHECK(eval_const(*network, "-3 + 1") == doctest::Approx(-2.));
}

TEST_CASE("comparisons and equality yield 0 or 1") {
  std::unique_ptr<Network> network = parse_abc_network();

  CHECK(eval_const(*network, "1 < 2") == doctest::Approx(1.));
  CHECK(eval_const(*network, "2 < 1") == doctest::Approx(0.));
  CHECK(eval_const(*network, "2 <= 2") == doctest::Approx(1.));
  CHECK(eval_const(*network, "3 > 2") == doctest::Approx(1.));
  CHECK(eval_const(*network, "3 >= 4") == doctest::Approx(0.));
  CHECK(eval_const(*network, "2 == 2") == doctest::Approx(1.));
  CHECK(eval_const(*network, "2 != 2") == doctest::Approx(0.));
}

TEST_CASE("logical operators work on truthiness") {
  std::unique_ptr<Network> network = parse_abc_network();

  CHECK(eval_const(*network, "1 & 1") == doctest::Approx(1.));
  CHECK(eval_const(*network, "1 & 0") == doctest::Approx(0.));
  CHECK(eval_const(*network, "1 | 0") == doctest::Approx(1.));
  CHECK(eval_const(*network, "0 | 0") == doctest::Approx(0.));
  CHECK(eval_const(*network, "1 ^ 1") == doctest::Approx(0.));
  CHECK(eval_const(*network, "1 ^ 0") == doctest::Approx(1.));
  CHECK(eval_const(*network, "!0") == doctest::Approx(1.));
  CHECK(eval_const(*network, "!1") == doctest::Approx(0.));
}

TEST_CASE("the conditional operator picks a branch") {
  std::unique_ptr<Network> network = parse_abc_network();

  CHECK(eval_const(*network, "1 ? 10 : 20") == doctest::Approx(10.));
  CHECK(eval_const(*network, "0 ? 10 : 20") == doctest::Approx(20.));
  CHECK(eval_const(*network, "(2 > 1) ? 1.5 : 2.5") == doctest::Approx(1.5));
}

TEST_CASE("node references read the network state") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");
  const Node* b = network->getNode("B");

  NetworkState state;
  state.reset();
  state.setNodeState(a, true);
  state.setNodeState(b, false);

  CHECK(eval_in(*network, "A", state) == doctest::Approx(1.));
  CHECK(eval_in(*network, "B", state) == doctest::Approx(0.));
  CHECK(eval_in(*network, "A & !B", state) == doctest::Approx(1.));
  CHECK(eval_in(*network, "A & B", state) == doctest::Approx(0.));
  CHECK(eval_in(*network, "A | B", state) == doctest::Approx(1.));
  CHECK(eval_in(*network, "A ? 5 : 7", state) == doctest::Approx(5.));
}

TEST_CASE("symbols evaluate to their table value") {
  std::unique_ptr<Network> network = parse_network(
    "node A { rate_up = $rate; rate_down = 1.0; }\n"
  );

  SymbolTable* symbols = network->getSymbolTable();
  const Symbol* rate = symbols->getSymbol("$rate");
  REQUIRE(rate != nullptr);

  symbols->setSymbolValue(rate, 0.25);
  CHECK(eval_const(*network, "$rate") == doctest::Approx(0.25));

  symbols->setSymbolValue(rate, 4.);
  CHECK(eval_const(*network, "$rate * 2") == doctest::Approx(8.));
}

TEST_CASE("reading an undefined symbol throws") {
  std::unique_ptr<Network> network = parse_abc_network();

  // $never_defined is interned by the parse but never given a value.
  CHECK_THROWS_AS(eval_const(*network, "$never_defined"), BNException);
}

TEST_CASE("constant expressions are recognised and folded") {
  std::unique_ptr<Network> network = parse_abc_network();

  Expression* constant = network->parseSingleExpression("2 * 3 + 1");
  REQUIRE(constant != nullptr);
  CHECK(constant->isConstantExpression());

  double folded = 0.;
  CHECK(constant->evalIfConstant(folded));
  CHECK(folded == doctest::Approx(7.));
  delete constant;

  Expression* variable = network->parseSingleExpression("A + 1");
  REQUIRE(variable != nullptr);
  CHECK_FALSE(variable->isConstantExpression());

  double unused = 0.;
  CHECK_FALSE(variable->evalIfConstant(unused));
  delete variable;
}

TEST_CASE("clone produces an independent, equivalent tree") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");

  NetworkState state;
  state.reset();
  state.setNodeState(a, true);

  Expression* original = network->parseSingleExpression("A ? 3 : 4");
  REQUIRE(original != nullptr);

  Expression* clone = original->clone();
  REQUIRE(clone != nullptr);
  CHECK(clone != original);
  CHECK(clone->toString() == original->toString());
  CHECK(clone->eval(nullptr, state) == doctest::Approx(original->eval(nullptr, state)));

  // Destroying the clone must leave the original intact.
  delete clone;
  CHECK(original->eval(nullptr, state) == doctest::Approx(3.));
  delete original;
}

TEST_CASE("toString reparses to the same value") {
  std::unique_ptr<Network> network = parse_abc_network();
  const Node* a = network->getNode("A");

  NetworkState state;
  state.reset();
  state.setNodeState(a, true);

  const char* sources[] = {
    "A & !A", "A | 0", "1 + 2 * 3", "A ? 1.5 : 2.5", "(A | 0) & 1", "!(A & 1)"
  };

  for (const char* source : sources) {
    CAPTURE(source);

    Expression* expr = network->parseSingleExpression(source);
    REQUIRE(expr != nullptr);
    const double value = expr->eval(nullptr, state);
    const std::string printed = expr->toString();
    delete expr;

    Expression* reparsed = network->parseSingleExpression(printed.c_str());
    REQUIRE(reparsed != nullptr);
    CHECK(reparsed->eval(nullptr, state) == doctest::Approx(value));
    delete reparsed;
  }
}

TEST_CASE("printing is value-preserving but not textually idempotent") {
  std::unique_ptr<Network> network = parse_abc_network();

  Expression* expr = network->parseSingleExpression("!(A & 1)");
  REQUIRE(expr != nullptr);
  const std::string once = expr->toString();
  delete expr;

  Expression* reparsed = network->parseSingleExpression(once.c_str());
  REQUIRE(reparsed != nullptr);
  const std::string twice = reparsed->toString();
  delete reparsed;

  // Each round trip wraps the sub-expression in another layer of parentheses
  // ("NOT (A AND 1)" becomes "NOT ((A AND 1))"), so --dump-bnd output is not a
  // fixed point. Harmless today, but worth knowing before anyone builds a
  // format-and-diff workflow on top of it.
  CHECK(once != twice);
  CHECK(twice.size() > once.size());
}

TEST_CASE("getNodes reports the nodes an expression depends on") {
  std::unique_ptr<Network> network = parse_abc_network();

  Expression* expr = network->parseSingleExpression("A & B");
  REQUIRE(expr != nullptr);
  CHECK(expr->getNodes().size() == 2);
  delete expr;

  Expression* constant = network->parseSingleExpression("1 + 1");
  REQUIRE(constant != nullptr);
  CHECK(constant->getNodes().empty());
  delete constant;
}

TEST_CASE("getNodes sees through a negation" * doctest::should_fail()) {
  // KNOWN BUG. NotLogicalExpression does not override Expression::getNodes(),
  // so it inherits the base implementation, which returns an empty vector:
  // any node appearing only under a "!" is invisible to getNodes().
  // AliasExpression (@logic) and FuncCallExpression have the same gap.
  //
  // This is not cosmetic: SBMLExporter builds each qual Transition's
  // listOfInputs from expr->getNodes() (see sbml/SBMLExporter.h), so exported
  // SBML omits those regulators while the <math> still references them.
  //
  // Marked should_fail so the suite stays green while the bug stands. Once
  // getNodes() is overridden in those three classes, doctest will report this
  // as an unexpected pass -- remove the decorator then.
  std::unique_ptr<Network> network = parse_abc_network();

  Expression* negated = network->parseSingleExpression("!A");
  REQUIRE(negated != nullptr);
  CHECK(negated->getNodes().size() == 1);
  delete negated;

  Expression* mixed = network->parseSingleExpression("A & !B");
  REQUIRE(mixed != nullptr);
  CHECK(mixed->getNodes().size() == 2);
  delete mixed;
}

TEST_CASE("malformed input is rejected") {
  std::unique_ptr<Network> network = parse_abc_network();

  CHECK_THROWS(network->parseSingleExpression("1 +"));
  CHECK_THROWS(network->parseSingleExpression("(1"));
}

TEST_CASE("division by zero yields a non-finite value rather than throwing") {
  std::unique_ptr<Network> network = parse_abc_network();

  // Documented rather than endorsed: a zero rate denominator in a .bnd file
  // propagates inf/nan into the transition rates instead of being diagnosed.
  CHECK(std::isinf(eval_const(*network, "1 / 0")));
  CHECK(eval_const(*network, "1 / 0") > 0.);
  CHECK(std::isinf(eval_const(*network, "(0 - 1) / 0")));
  CHECK(eval_const(*network, "(0 - 1) / 0") < 0.);
  CHECK(std::isnan(eval_const(*network, "0 / 0")));
}

} // TEST_SUITE
