# ---------------------------------------------------------------------------
# C++ unit tests for the MaBoSS library
#
# These pin the behaviour of the pieces that a modernization pass is most
# likely to disturb silently: the state/bitset representation, the symbol
# table, expression parsing and evaluation, network compilation, and -- most
# importantly -- the exact pseudo-random streams, since every golden-output
# regression test depends on those being byte-for-byte unchanged.
#
# doctest is used as the framework: a system installation is preferred, and
# otherwise it is fetched at configure time. Keep tests OFF-by-default builds
# (the MABOSS_TESTS option) free of any network access.
# ---------------------------------------------------------------------------

set(MABOSS_UNIT_SOURCES
  main.cc
  test_bitset.cc
  test_symbols.cc
  test_expressions.cc
  test_network.cc
  test_runconfig.cc
  test_random.cc
)

find_package(doctest QUIET)

if(NOT doctest_FOUND)
  if(CMAKE_VERSION VERSION_LESS 3.11)
    message(STATUS "MaBoSS tests: CMake >= 3.11 or an installed doctest is required "
                   "for the unit tests, skipping them")
    return()
  endif()

  include(FetchContent)
  FetchContent_Declare(doctest
    URL      https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
    URL_HASH SHA256=632ed2c05a7f53fa961381497bf8069093f0d6628c5f26286161fbd32a560186
  )

  # Do not let doctest add its own tests/examples to our ctest listing, build
  # a main() we do not use, or get installed alongside MaBoSS.
  set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
  set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
  set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

  FetchContent_MakeAvailable(doctest)

  if(NOT TARGET doctest::doctest)
    message(STATUS "MaBoSS tests: doctest unavailable (no network?), skipping the unit tests")
    return()
  endif()
endif()

add_executable(maboss_unit_tests ${MABOSS_UNIT_SOURCES})

# The static library carries MAXNODES, the bitset flags and the include
# directories as PUBLIC usage requirements, so the tests see exactly the same
# configuration as the engine binaries.
target_link_libraries(maboss_unit_tests PRIVATE ${MABOSS_LIBNAME}-static doctest::doctest)

if(MSVC)
  target_compile_options(maboss_unit_tests PRIVATE /bigobj)
endif()

# ---------------------------------------------------------------------------
# Test registration
# ---------------------------------------------------------------------------
#
# One ctest entry per doctest TEST_SUITE, launched through `cmake -E env` so
# that the sanitizer options reach the process the same way they do for the
# regression tests.
#
# doctest ships a doctest_discover_tests() helper that would give one entry per
# TEST_CASE, but it splats its PROPERTIES argument into set_tests_properties(),
# which mispairs any multi-value property such as ENVIRONMENT, and it mangles
# test names containing brackets. Per-suite entries are coarser but robust, and
# the whole binary runs in well under a second anyway.
#
# Adding a suite: add its name here and to the TEST_SUITE in the source file.
set(MABOSS_UNIT_SUITES
  bitset
  symbols
  expressions
  network
  runconfig
  random
)

foreach(suite ${MABOSS_UNIT_SUITES})
  add_test(
    NAME maboss.unit.${suite}
    COMMAND ${CMAKE_COMMAND} -E env ${MABOSS_SANITIZER_ENV}
            $<TARGET_FILE:maboss_unit_tests> --test-suite=${suite} --no-skipped-summary
  )
  set_tests_properties(maboss.unit.${suite} PROPERTIES LABELS unit TIMEOUT 300)
endforeach()
