# ---------------------------------------------------------------------------
# MaBoSS test suite
#
# Two families of tests are registered here:
#
#   maboss.regression.*   Golden-output tests. They run an engine binary over a
#                         model shipped under maboss/ and compare the resulting
#                         CSV against the reference files in maboss/*/refer/.
#                         These wrap the engine/tests/maboss/test-*.sh scripts
#                         that CI has always driven by hand through
#                         scripts/run_tests.sh.
#
#   maboss.unit.*         C++ unit tests linked against the MaBoSS library.
#                         See unit/CMakeLists.txt.
#
# Usage:
#     cmake -B build -S . -DMABOSS_TESTS=ON
#     cmake --build build --parallel
#     ctest --test-dir build --output-on-failure
#
# Only the tests the current configuration can actually satisfy are registered:
# a single CMake build tree produces exactly one MAXNODES variant, so e.g. the
# 128-node regression tests only appear for a -DMAXNODES=128 or -DDYNBITSET=1
# build. `ctest -N` lists what this build tree will run.
# ---------------------------------------------------------------------------

set(MABOSS_SHELL_TEST_DIR ${CMAKE_CURRENT_LIST_DIR}/maboss)

# ---------------------------------------------------------------------------
# Sanitizer runtime options
# ---------------------------------------------------------------------------
#
# When the tree is built with -DMABOSS_SANITIZE=..., every test gets the
# suppression files for the known pre-existing findings, so that `ctest` fails
# on newly introduced leaks and undefined behaviour rather than on the backlog.
# Both files document what they suppress and are meant to shrink to nothing.

set(MABOSS_SANITIZER_ENV "")
if(MABOSS_SANITIZE)
  set(MABOSS_SANITIZER_ENV
    LSAN_OPTIONS=suppressions=${CMAKE_CURRENT_LIST_DIR}/sanitizers/lsan.supp
    UBSAN_OPTIONS=suppressions=${CMAKE_CURRENT_LIST_DIR}/sanitizers/ubsan.supp:halt_on_error=1:print_stacktrace=1
  )
endif()

# ---------------------------------------------------------------------------
# Prerequisites for the shell-driven regression tests
# ---------------------------------------------------------------------------
#
# They all need bash: several scripts use [[ ]], so /bin/sh will not do.
#
# Most of them also need a Python interpreter with numpy, for the compare_*.py
# helpers that do the tolerant CSV comparison. The few that compare with plain
# diff are marked NO_PYTHON below and stay available without it.

find_program(MABOSS_TEST_BASH NAMES bash DOC "bash used to run the regression test scripts")

if(NOT MABOSS_TEST_PYTHON)
  # The first interpreter that can import numpy wins. Override with
  # -DMABOSS_TEST_PYTHON=/path/to/python if the autodetected one is wrong.
  find_package(Python3 COMPONENTS Interpreter QUIET)
  # Note the last two entries: a pyenv/conda python earlier in PATH often
  # shadows the system interpreter that actually has numpy installed.
  foreach(candidate ${Python3_EXECUTABLE} python3 python
                    /usr/bin/python3 /usr/local/bin/python3)
    if(NOT candidate)
      continue()
    endif()
    execute_process(
      COMMAND ${candidate} -c "import numpy"
      RESULT_VARIABLE numpy_missing
      OUTPUT_QUIET ERROR_QUIET
    )
    if(numpy_missing EQUAL 0)
      # Resolve to an absolute path so the value does not depend on the PATH
      # ctest happens to be invoked with.
      execute_process(
        COMMAND ${candidate} -c "import sys; sys.stdout.write(sys.executable)"
        OUTPUT_VARIABLE resolved
        ERROR_QUIET
      )
      set(MABOSS_TEST_PYTHON "${resolved}" CACHE FILEPATH "Python (with numpy) used by the regression test scripts")
      break()
    endif()
  endforeach()
endif()

if(NOT MABOSS_TEST_BASH)
  message(STATUS "MaBoSS tests: bash not found, skipping the regression tests")
elseif(NOT MABOSS_TEST_PYTHON)
  message(STATUS "MaBoSS tests: no Python interpreter with numpy found, skipping the "
                 "regression tests that need it (install numpy, or pass "
                 "-DMABOSS_TEST_PYTHON=...)")
else()
  message(STATUS "MaBoSS tests: regression tests will use ${MABOSS_TEST_PYTHON}")
endif()

# ---------------------------------------------------------------------------
# maboss_add_regression_test(NAME <name> SCRIPT <test-foo.sh>
#                            [ARGS <args...>] [ENV <VAR=value...>]
#                            [TIMEOUT <seconds>] [NO_PYTHON])
#
# Wraps one engine/tests/maboss/test-*.sh script as a CTest test.
#
# The scripts locate the binaries under test through the environment variables
# that maboss/share.sh honours (MABOSS, MABOSS_128n, POPMABOSS, ...), so each
# test is handed the exact path of the binary this build produced rather than
# relying on share.sh's ../../pub defaults.
#
# LAUNCHER is deliberately set to the empty string: share.sh otherwise defaults
# it to "/usr/bin/time -p", which does not exist everywhere and only adds noise
# to the test log.
# ---------------------------------------------------------------------------
function(maboss_add_regression_test)
  cmake_parse_arguments(RT "NO_PYTHON" "NAME;SCRIPT;TIMEOUT" "ARGS;ENV" ${ARGN})

  if(NOT MABOSS_TEST_BASH)
    return()
  endif()
  # NO_PYTHON marks the scripts that compare with plain diff instead of the
  # numpy-based compare_*.py helpers, so they stay available without numpy.
  if(NOT RT_NO_PYTHON AND NOT MABOSS_TEST_PYTHON)
    return()
  endif()
  if(NOT EXISTS ${MABOSS_SHELL_TEST_DIR}/${RT_SCRIPT})
    message(WARNING "MaBoSS tests: ${RT_SCRIPT} does not exist, not registering ${RT_NAME}")
    return()
  endif()

  set(test_name maboss.regression.${RT_NAME})

  add_test(
    NAME ${test_name}
    COMMAND ${CMAKE_COMMAND} -E env
            LAUNCHER=
            PYTHON=${MABOSS_TEST_PYTHON}
            ${MABOSS_SANITIZER_ENV}
            ${RT_ENV}
            ${MABOSS_TEST_BASH} ./${RT_SCRIPT} ${RT_ARGS}
    WORKING_DIRECTORY ${MABOSS_SHELL_TEST_DIR}
  )

  if(NOT RT_TIMEOUT)
    set(RT_TIMEOUT 600)
  endif()

  set_tests_properties(${test_name} PROPERTIES
    TIMEOUT ${RT_TIMEOUT}
    LABELS regression
    # Every script does "rm -rf tmp; mkdir -p tmp" inside
    # engine/tests/maboss, so two of them must never overlap.
    RESOURCE_LOCK maboss_shell_test_tmp
  )
endfunction()

# ---------------------------------------------------------------------------
# Which binary plays which role in this build tree
# ---------------------------------------------------------------------------
#
# CI validates two ways of satisfying the 128-node tests: a real -DMAXNODES=128
# build, and a dynamic-bitset build (the DynBitSet job points both MABOSS and
# MABOSS_128n at MaBoSS_dn). We register the same combinations, and nothing else
# — a -DMAXNODES=256 build, say, has no reference outputs of its own.

set(MABOSS_BIN $<TARGET_FILE:${MABOSS_NAME}>)
set(POPMABOSS_BIN $<TARGET_FILE:${POPMABOSS_NAME}>)

if(DYNBITSET)
  set(MABOSS_RUNS_SMALL TRUE)   # handles any node count
  set(MABOSS_RUNS_128N TRUE)
elseif(MAXNODES EQUAL 64)
  set(MABOSS_RUNS_SMALL TRUE)
  set(MABOSS_RUNS_128N FALSE)
elseif(MAXNODES EQUAL 128)
  set(MABOSS_RUNS_SMALL FALSE)  # reference outputs belong to the 64n binary
  set(MABOSS_RUNS_128N TRUE)
else()
  set(MABOSS_RUNS_SMALL FALSE)
  set(MABOSS_RUNS_128N FALSE)
  message(STATUS "MaBoSS tests: MAXNODES=${MAXNODES} has no reference outputs, "
                 "only unit tests will be registered")
endif()

# --- Tests driven by the standard (<= 64 node) engine -----------------------
if(MABOSS_RUNS_SMALL)
  maboss_add_regression_test(NAME cellcycle SCRIPT test-cellcycle.sh
                             ENV MABOSS=${MABOSS_BIN})
  maboss_add_regression_test(NAME bnet      SCRIPT test-bnet.sh
                             ENV MABOSS=${MABOSS_BIN})
  maboss_add_regression_test(NAME ensemble  SCRIPT test-ensemble.sh
                             ENV MABOSS=${MABOSS_BIN})
  maboss_add_regression_test(NAME prngs     SCRIPT test-prngs.sh
                             ENV MABOSS=${MABOSS_BIN})
  maboss_add_regression_test(NAME schedule  SCRIPT test-schedule.sh
                             ENV MABOSS=${MABOSS_BIN})

  # test-rngs.sh exercises the physical (/dev/urandom) generator, which is
  # POSIX-only; run_tests.sh likewise skips it on Windows.
  if(NOT WIN32)
    maboss_add_regression_test(NAME rngs SCRIPT test-rngs.sh
                               ENV MABOSS=${MABOSS_BIN})
  endif()

  maboss_add_regression_test(NAME popmaboss SCRIPT test-popmaboss.sh
                             ENV POPMABOSS=${POPMABOSS_BIN})

  if(SBML)
    maboss_add_regression_test(NAME sbml SCRIPT test-sbml.sh
                               ENV MABOSS=${MABOSS_BIN})
  endif()
  if(SEDML)
    maboss_add_regression_test(NAME sedml SCRIPT test-sedml.sh
                               NO_PYTHON
                               ENV MABOSS=${MABOSS_BIN})
  endif()
endif()

# --- Tests driven by the 128-node engine ------------------------------------
if(MABOSS_RUNS_128N)
  maboss_add_regression_test(NAME ewing SCRIPT test-ewing.sh
                             TIMEOUT 1200
                             ENV MABOSS=${MABOSS_BIN} MABOSS_128n=${MABOSS_BIN})
  maboss_add_regression_test(NAME observed_graph SCRIPT test-observed_graph.sh
                             NO_PYTHON
                             ENV MABOSS_128n=${MABOSS_BIN})
endif()

# --- Client/server, only when those executables were built ------------------
if(BUILD_SERVER AND BUILD_CLIENT AND NOT WIN32 AND MABOSS_RUNS_SMALL)
  # test-server.sh runs in two phases: cellcycle against $MABOSS_SERVER, then
  # the 128-node ewing model against $MABOSS_SERVER_128n. It skips the second
  # phase when that binary is not executable, so hand it an empty value unless
  # this build actually produced a server that can handle more than 64 nodes --
  # pointing it at the 64-node server instead makes the client fail with
  # "maximum node count exceeded".
  if(MABOSS_RUNS_128N)
    set(maboss_server_128n $<TARGET_FILE:${MABOSS_SERVER_NAME}>)
  else()
    set(maboss_server_128n "")
  endif()

  maboss_add_regression_test(NAME server SCRIPT test-server.sh
                             ENV MABOSS=${MABOSS_BIN}
                                 MABOSS_CLIENT=$<TARGET_FILE:${MABOSS_CLIENT_NAME}>
                                 MABOSS_SERVER=$<TARGET_FILE:${MABOSS_SERVER_NAME}>
                                 MABOSS_SERVER_128n=${maboss_server_128n})
endif()

# --- MPI ---------------------------------------------------------------------
# The MPI scripts take the process count as their first argument and invoke
# mpirun themselves. Note they also hardcode "/usr/bin/time -p" rather than
# going through $LAUNCHER, so they need that binary present.
if(MPI AND MPI_CXX_FOUND)
  find_program(MABOSS_TEST_MPIRUN NAMES mpirun mpiexec)
  if(MABOSS_TEST_MPIRUN AND MABOSS_RUNS_SMALL)
    maboss_add_regression_test(NAME mpi.cellcycle SCRIPT test-mpi-cellcycle.sh
                               ARGS 4 ENV MABOSS_MPI=${MABOSS_BIN})
    maboss_add_regression_test(NAME mpi.ensemble SCRIPT test-mpi-ensemble.sh
                               ARGS 4 ENV MABOSS_MPI=${MABOSS_BIN})
    maboss_add_regression_test(NAME mpi.popmaboss SCRIPT test-mpi-popmaboss.sh
                               ARGS 4 ENV POPMABOSS_MPI=${POPMABOSS_BIN})
  endif()
  if(MABOSS_TEST_MPIRUN AND MABOSS_RUNS_128N)
    maboss_add_regression_test(NAME mpi.ewing SCRIPT test-mpi-ewing.sh
                               ARGS 4 TIMEOUT 1200
                               ENV MABOSS_128n_MPI=${MABOSS_BIN})
    maboss_add_regression_test(NAME mpi.observed_graph SCRIPT test-mpi-observed_graph.sh
                               ARGS 4 ENV MABOSS_128n_MPI=${MABOSS_BIN})
  endif()
endif()

# Not registered here, because they need their own CMake configuration rather
# than just a binary from this build tree:
#   test-user_func.sh   needs a -DUSERFUNC=... build of the plugin
#                       (see scripts/run_userfunc_tests.sh)
#   test-container.sh   needs a running MaBoSS server container

add_subdirectory(unit)
