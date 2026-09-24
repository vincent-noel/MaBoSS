/*
  Entry point for the MaBoSS unit tests.

  Two pieces of global engine state have to be set up before any test runs,
  exactly as the engines do it at startup:

  - builtin_functions_init(), reached through MetaEngine::init(), fills the
    global function registry that expression parsing looks names up in.

  - For dynamic-bitset builds, MBDynBitset::init_pthread() installs this
    thread's bitset allocator. It is per-thread and must be called exactly once
    per thread (each call consumes an allocator slot, and end_pthread() frees
    the thread's slot), so it belongs here rather than in individual cases.
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "NetworkState_Impl.h"
#include "engines/MetaEngine.h"

namespace {

struct GlobalEngineState {
  GlobalEngineState() {
    MetaEngine::init();
#ifdef USE_DYNAMIC_BITSET
    MBDynBitset::init_pthread();
#endif
  }
  ~GlobalEngineState() {
#ifdef USE_DYNAMIC_BITSET
    MBDynBitset::end_pthread();
#endif
  }
};

const GlobalEngineState engine_state_is_initialised;

} // namespace
