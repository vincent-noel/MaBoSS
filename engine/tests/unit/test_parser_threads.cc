/*
  Concurrent parsing.

  The .bnd and .cfg parsers are reentrant: each parse has its own scanner and
  its own context (network, configuration, line number), so different threads
  can parse different models at the same time. These cases run several parses
  at once and check each thread got its own result back. Run them under
  ThreadSanitizer (-DMABOSS_SANITIZE=thread) to check for races as well.
*/

#include <doctest/doctest.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "BNException.h"
#include "Network.h"
#include "PopNetwork.h"
#include "RunConfig.h"
#include "NetworkState_Impl.h"

using namespace MaBoSS;

namespace {

const int THREAD_COUNT = 8;
const int PARSES_PER_THREAD = 50;

// Runs body(index) on THREAD_COUNT threads at once and returns how many of
// them failed. Failures are counted rather than CHECKed: doctest assertions
// are not thread-safe.
template <typename Body>
int run_threads(Body body) {
  std::atomic<int> failures(0);
  std::atomic<bool> go(false);
  std::vector<std::thread> threads;
  for (int t = 0; t < THREAD_COUNT; ++t) {
    threads.emplace_back([&, t]() {
#ifdef USE_DYNAMIC_BITSET
      MBDynBitset::init_pthread();
#endif
      while (!go) {
        std::this_thread::yield();
      }
      try {
        if (!body(t)) {
          failures++;
        }
      } catch (...) {
        failures++;
      }
#ifdef USE_DYNAMIC_BITSET
      MBDynBitset::end_pthread();
#endif
    });
  }
  go = true;
  for (auto& thread : threads) {
    thread.join();
  }
  return failures;
}

// A chain of node_count nodes whose names are specific to thread t.
std::string chain_network(int t, int node_count) {
  std::string bnd;
  for (int n = 0; n < node_count; ++n) {
    std::string name = "T" + std::to_string(t) + "_N" + std::to_string(n);
    std::string next = "T" + std::to_string(t) + "_N" + std::to_string((n + 1) % node_count);
    bnd += "node " + name + " {\n"
           "  logic = " + next + ";\n"
           "  rate_up = @logic ? $up : 0;\n"
           "  rate_down = @logic ? 0 : 1;\n"
           "}\n";
  }
  return bnd;
}

} // namespace

TEST_SUITE("parser") {

TEST_CASE("threads parsing different networks and configurations get their own results") {
  int failures = run_threads([](int t) {
    for (int i = 0; i < PARSES_PER_THREAD; ++i) {
      int node_count = 2 + t;
      Network network;
      if (network.parseExpression(chain_network(t, node_count).c_str()) != 0) {
        return false;
      }
      if ((int)network.getNodes().size() != node_count) {
        return false;
      }
      for (const Node* node : network.getNodes()) {
        if (node->getLabel().rfind("T" + std::to_string(t) + "_", 0) != 0) {
          return false;
        }
      }

      RunConfig config;
      std::string cfg = "$up = " + std::to_string(t) + ";\n"
                        "max_time = " + std::to_string(10 + t) + ";\n"
                        "T" + std::to_string(t) + "_N0.istate = TRUE;\n";
      if (config.parseExpression(&network, cfg.c_str()) != 0) {
        return false;
      }
      SymbolTable* symbols = network.getSymbolTable();
      if (symbols->getSymbolValue(symbols->getSymbol("$up")) != t) {
        return false;
      }
      if (config.getMaxTime() != 10 + t) {
        return false;
      }
    }
    return true;
  });
  CHECK(failures == 0);
}

TEST_CASE("PopMaBoSS keywords are only keywords for the thread parsing a PopNetwork") {
  // Even threads parse a PopNetwork with a death rule; odd threads parse a
  // plain Network with a node called "death", which only parses if DEATH is
  // not a keyword there.
  int failures = run_threads([](int t) {
    for (int i = 0; i < PARSES_PER_THREAD; ++i) {
      if (t % 2 == 0) {
        PopNetwork network;
        if (network.parseExpression(
              "node A { rate_up = 1; rate_down = 1; }\n"
              "death { rate = 0.5; }\n") != 0) {
          return false;
        }
        if (network.getDeathRate() == nullptr) {
          return false;
        }
      } else {
        Network network;
        if (network.parseExpression(
              "node death { rate_up = 1; rate_down = 1; }\n") != 0) {
          return false;
        }
        if (network.getNode("death") == nullptr) {
          return false;
        }
      }
    }
    return true;
  });
  CHECK(failures == 0);
}

TEST_CASE("each thread's syntax error reports its own line number") {
  int failures = run_threads([](int t) {
    for (int i = 0; i < PARSES_PER_THREAD; ++i) {
      // The error is on line t + 2.
      std::string bnd = "node A { rate_up = 1; rate_down = 1; }\n";
      for (int line = 0; line < t; ++line) {
        bnd += "\n";
      }
      bnd += "node B { rate_up = 1 +; }\n";
      Network network;
      try {
        network.parseExpression(bnd.c_str());
        return false;
      } catch (const BNException& e) {
        if (e.getMessage() != "BND syntax error at line #" + std::to_string(t + 2)) {
          return false;
        }
      }
    }
    return true;
  });
  CHECK(failures == 0);
}

}
