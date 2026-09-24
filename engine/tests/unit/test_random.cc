/*
  Pseudo-random generators.

  Every golden-output regression test in engine/tests/maboss compares CSV files
  byte-for-byte against references produced years ago, which means the exact
  random streams are part of the contract: a refactor that perturbs a single
  draw invalidates every reference file in the repository.

  The known-answer vectors below were captured from the engine and cross-check
  against the reference implementations they imitate — the glibc vector is the
  documented output of glibc's rand() for seed 1 and 42, and the mt19937 vector
  is standard MT19937 output for those seeds. Do not "fix" them: if a change
  makes them fail, the change altered the simulation output.
*/

#include <doctest/doctest.h>

#include <vector>

#include "RandomGenerator.h"

namespace {

// First eight draws of generateUInt32() for a freshly seeded generator.
const std::vector<unsigned int> rand48_seed1 = {
  89400484U, 976015093U, 1792756325U, 721524505U,
  1214379247U, 3794415U, 402845420U, 2126940991U
};
const std::vector<unsigned int> rand48_seed42 = {
  1598855263U, 735945821U, 238553827U, 906966006U,
  174184913U, 1839192415U, 1071163602U, 1028245859U
};
const std::vector<unsigned int> glibc_seed1 = {
  1804289383U, 846930886U, 1681692777U, 1714636915U,
  1957747793U, 424238335U, 719885386U, 1649760492U
};
const std::vector<unsigned int> glibc_seed42 = {
  71876166U, 708592740U, 1483128881U, 907283241U,
  442951012U, 537146758U, 1366999021U, 1854614940U
};
const std::vector<unsigned int> mt19937_seed1 = {
  1791095845U, 4282876139U, 3093770124U, 4005303368U,
  491263U, 550290313U, 1298508491U, 4290846341U
};
const std::vector<unsigned int> mt19937_seed42 = {
  1608637542U, 3421126067U, 4083286876U, 787846414U,
  3143890026U, 3348747335U, 2571218620U, 2563451924U
};

std::vector<unsigned int> take(RandomGenerator& gen, size_t count) {
  std::vector<unsigned int> drawn;
  drawn.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    drawn.push_back(gen.generateUInt32());
  }
  return drawn;
}

} // namespace

TEST_SUITE("random") {

TEST_CASE("rand48 produces its documented stream") {
  Rand48RandomGenerator seeded_one(1);
  CHECK(take(seeded_one, rand48_seed1.size()) == rand48_seed1);

  Rand48RandomGenerator seeded_fortytwo(42);
  CHECK(take(seeded_fortytwo, rand48_seed42.size()) == rand48_seed42);
}

TEST_CASE("glibc generator matches glibc rand()") {
  GLibCRandomGenerator seeded_one(1);
  CHECK(take(seeded_one, glibc_seed1.size()) == glibc_seed1);

  GLibCRandomGenerator seeded_fortytwo(42);
  CHECK(take(seeded_fortytwo, glibc_seed42.size()) == glibc_seed42);
}

TEST_CASE("mt19937 matches the reference Mersenne Twister") {
  MT19937RandomGenerator seeded_one(1);
  CHECK(take(seeded_one, mt19937_seed1.size()) == mt19937_seed1);

  MT19937RandomGenerator seeded_fortytwo(42);
  CHECK(take(seeded_fortytwo, mt19937_seed42.size()) == mt19937_seed42);
}

TEST_CASE("the same seed replays the same stream") {
  // This is what lets a multi-threaded run be reproducible: each thread seeds
  // its own generator deterministically from the config seed.
  Rand48RandomGenerator a(7);
  Rand48RandomGenerator b(7);
  CHECK(take(a, 256) == take(b, 256));

  GLibCRandomGenerator c(7);
  GLibCRandomGenerator d(7);
  CHECK(take(c, 256) == take(d, 256));

  MT19937RandomGenerator e(7);
  MT19937RandomGenerator f(7);
  CHECK(take(e, 256) == take(f, 256));
}

TEST_CASE("different seeds give different streams") {
  Rand48RandomGenerator a(1);
  Rand48RandomGenerator b(2);
  CHECK(take(a, 64) != take(b, 64));
}

TEST_CASE("setSeed rewinds the stream") {
  Rand48RandomGenerator gen(11);
  const std::vector<unsigned int> first = take(gen, 32);

  gen.setSeed(11);
  CHECK(take(gen, 32) == first);
}

TEST_CASE("generate() stays within the unit interval") {
  Rand48RandomGenerator rand48(3);
  GLibCRandomGenerator glibc(3);
  MT19937RandomGenerator mt(3);

  RandomGenerator* generators[] = {&rand48, &glibc, &mt};
  for (RandomGenerator* gen : generators) {
    CAPTURE(gen->getName());
    for (int i = 0; i < 4096; ++i) {
      const double value = gen->generate();
      REQUIRE(value >= 0.);
      REQUIRE(value < 1.);
    }
  }
}

TEST_CASE("generators report themselves correctly") {
  Rand48RandomGenerator rand48(1);
  CHECK(rand48.getName() == "rand48");
  CHECK(rand48.isPseudoRandom());

  GLibCRandomGenerator glibc(1);
  CHECK(glibc.getName() == "glibc");
  CHECK(glibc.isPseudoRandom());

  MT19937RandomGenerator mt(1);
  CHECK(mt.getName() == "mt19937");
  CHECK(mt.isPseudoRandom());
}

TEST_CASE("the factory hands out the generator it advertises") {
  RandomGeneratorFactory factory(RandomGeneratorFactory::DEFAULT);
  CHECK(factory.getName() == "rand48");

  RandomGenerator* gen = factory.generateRandomGenerator(42);
  REQUIRE(gen != nullptr);
  CHECK(gen->getName() == "rand48");
  CHECK(take(*gen, rand48_seed42.size()) == rand48_seed42);
  delete gen;
}

} // TEST_SUITE
