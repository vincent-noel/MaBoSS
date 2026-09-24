# MaBoSS test suite

Two families of tests, both driven by `ctest`.

| | what it is | where |
|---|---|---|
| `maboss.regression.*` | Golden-output tests: run an engine binary over a model and compare the CSV output against stored references | [`maboss/`](maboss/) |
| `maboss.unit.*` | C++ unit tests linked against the MaBoSS library | [`unit/`](unit/) |

## Running

Tests are opt-in, so that packaging builds pull in neither a test framework nor
a Python dependency:

```sh
cmake -B build -S . -DMABOSS_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Useful variations:

```sh
ctest --test-dir build -N                     # list what this build registers
ctest --test-dir build -L unit                # unit tests only (fast)
ctest --test-dir build -L regression          # golden-output tests only
ctest --test-dir build -R cellcycle           # one test
ctest --test-dir build --parallel 4           # regression tests still serialize
```

## What gets registered

A single CMake build tree produces exactly one `MAXNODES` variant, so only the
tests that variant has reference outputs for are registered. `ctest -N` is
always the authoritative answer; the rule is:

| build | regression tests registered |
|---|---|
| default (`MAXNODES=64`) | cellcycle, bnet, ensemble, prngs, schedule, rngs, popmaboss |
| `-DMAXNODES=128` | ewing, observed_graph |
| `-DDYNBITSET=1` | all of the above (the dynamic-bitset binary handles any node count) |
| `+ -DSBML=1` | adds sbml |
| `+ -DSEDML=1` | adds sedml |
| `+ -DBUILD_SERVER=1 -DBUILD_CLIENT=1` | adds server |
| `+ -DMPI=1` | adds the mpi.* variants, if `mpirun` is found |

Two scripts under `maboss/` are deliberately not registered, because they need
their own CMake configuration rather than a binary from the current build tree:

- `test-user_func.sh` needs a `-DUSERFUNC=...` build of the plugin — see
  [`scripts/run_userfunc_tests.sh`](../../scripts/run_userfunc_tests.sh).
- `test-container.sh` needs a running MaBoSS server container.

## Requirements

- **bash**, not just `/bin/sh`: several scripts use `[[ ]]`.
- **Python with numpy**, for the `compare_*.py` comparison scripts. CMake picks
  the first interpreter that can import numpy; if a pyenv or conda Python
  earlier in `PATH` shadows the one that has it, point CMake at the right one
  with `-DMABOSS_TEST_PYTHON=/usr/bin/python3`. When no suitable interpreter is
  found the regression tests are skipped, with a message saying so at configure
  time.

The scripts honour `$PYTHON` for the interpreter and `$LAUNCHER` for a command
prefix (`ctest` sets `LAUNCHER=` to suppress the default `/usr/bin/time -p`).

## Sanitizers

```sh
cmake -B build-asan -S . -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DMABOSS_TESTS=ON -DMABOSS_SANITIZE=address,undefined
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

`MABOSS_SANITIZE` takes anything `-fsanitize=` accepts (`address,undefined`,
`thread`, ...). For a sanitizer build, ctest automatically applies the
suppression files in [`sanitizers/`](sanitizers/).

Current state:

- **Unit tests are clean** under `address,undefined`, so `ctest -L unit` is what
  CI gates on. The suppression files cover exactly what that needs: the known
  flex/bison per-parse leaks, and the signed-overflow the glibc RNG relies on.
- **Regression tests still report a leak backlog** (a `Network`/`RunConfig` not
  freed on one exit path in `MaBoSS.cc`, and the per-node schedule expressions
  in `ProbTrajEngine::buildSchedule`). Both are listed in
  [`sanitizers/lsan.supp`](sanitizers/lsan.supp) as deliberately *not*
  suppressed, because any pattern broad enough to catch them would hide real
  leaks too. CI runs that pass reporting-only until they are fixed.

Entries in the suppression files should be deleted as the underlying issues
are fixed.

## Adding tests

A regression test is one line in [`CMakeLists.txt`](CMakeLists.txt) next to the
others, naming the script and the binaries it needs:

```cmake
maboss_add_regression_test(NAME mytest SCRIPT test-mytest.sh
                           ENV MABOSS=${MABOSS_BIN})
```

Scripts must keep working when `MABOSS`, `POPMABOSS`, ... are already set in
the environment (`maboss/share.sh` handles that).

> **Careful:** every script does `rm -rf tmp; mkdir -p tmp` inside
> `engine/tests/maboss/`, in the *source* tree. Within one ctest invocation a
> `RESOURCE_LOCK` keeps them from overlapping, but that lock does not extend
> across processes: running two `ctest` invocations against the same checkout
> at the same time — a Release build and a sanitizer build, say — makes them
> clobber each other's output and fail in confusing ways. Run them one at a
> time, or from separate checkouts.

Unit tests are doctest `TEST_CASE`s inside a `TEST_SUITE`. Add the file to
`MABOSS_UNIT_SOURCES` and, for a new suite, its name to `MABOSS_UNIT_SUITES` in
[`unit/CMakeLists.txt`](unit/CMakeLists.txt); ctest registers one entry per
suite.

Two cases are deliberately not green-by-default, each documenting a real bug
with a comment explaining it:

- `getNodes sees through a negation` is marked `doctest::should_fail()`.
- `copying a network yields an independent object graph` is marked
  `doctest::skip()`, because running it segfaults.

Remove the decorator when the corresponding bug is fixed.
