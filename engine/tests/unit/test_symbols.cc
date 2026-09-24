/*
  SymbolTable: the .cfg variable table. Symbols are interned by name and
  addressed by a dense index, and every Expression holding a symbol keeps the
  Symbol* it was given at parse time — so index stability across insertions is
  a load-bearing property, not an implementation detail.
*/

#include <doctest/doctest.h>

#include <algorithm>
#include <string>

#include "Symbols.h"
#include "BNException.h"

TEST_SUITE("symbols") {

TEST_CASE("symbols are interned by name") {
  SymbolTable table;

  const Symbol* a = table.getOrMakeSymbol("$a");
  const Symbol* b = table.getOrMakeSymbol("$b");

  CHECK(table.getSymbolCount() == 2);
  CHECK(a->getName() == "$a");
  CHECK(a->getIndex() == 0);
  CHECK(b->getIndex() == 1);

  // Asking again yields the very same symbol, not a copy.
  CHECK(table.getOrMakeSymbol("$a") == a);
  CHECK(table.getSymbolCount() == 2);
}

TEST_CASE("getSymbol does not create") {
  SymbolTable table;

  CHECK(table.getSymbol("$missing") == nullptr);
  CHECK(table.getSymbolCount() == 0);

  table.getOrMakeSymbol("$present");
  CHECK(table.getSymbol("$present") != nullptr);
  CHECK(table.getSymbolCount() == 1);
}

TEST_CASE("indices stay stable as symbols are added") {
  SymbolTable table;

  const Symbol* first = table.getOrMakeSymbol("$first");
  const SymbolIndex first_idx = first->getIndex();

  for (int i = 0; i < 64; ++i) {
    table.getOrMakeSymbol("$filler" + std::to_string(i));
  }

  CHECK(first->getIndex() == first_idx);
  CHECK(table.getOrMakeSymbol("$first") == first);
}

TEST_CASE("reading an undefined symbol throws unless unchecked") {
  SymbolTable table;
  const Symbol* s = table.getOrMakeSymbol("$undefined");

  CHECK_THROWS_AS(table.getSymbolValue(s), BNException);
  CHECK(table.getSymbolValue(s, false) == 0.);

  table.setSymbolValue(s, 3.5);
  CHECK(table.getSymbolValue(s) == doctest::Approx(3.5));
}

TEST_CASE("overrideSymbolValue pins a value against later assignment") {
  SymbolTable table;
  const Symbol* s = table.getOrMakeSymbol("$pinned");

  table.overrideSymbolValue(s, 1.);
  table.setSymbolValue(s, 2.);

  // This is how --config-vars beats a value set in the .cfg file.
  CHECK(table.getSymbolValue(s) == doctest::Approx(1.));
}

TEST_CASE("defineUndefinedSymbols makes every symbol readable") {
  SymbolTable table;
  const Symbol* s = table.getOrMakeSymbol("$never_set");

  table.defineUndefinedSymbols();

  CHECK_NOTHROW(table.getSymbolValue(s));
  CHECK(table.getSymbolValue(s) == doctest::Approx(0.));
}

TEST_CASE("getSymbolsNames reports every interned symbol") {
  SymbolTable table;
  table.getOrMakeSymbol("$x");
  table.getOrMakeSymbol("$y");

  std::vector<std::string> names = table.getSymbolsNames();
  std::sort(names.begin(), names.end());

  CHECK(names == std::vector<std::string>{"$x", "$y"});
}

} // TEST_SUITE
