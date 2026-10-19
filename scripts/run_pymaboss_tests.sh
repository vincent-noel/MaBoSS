#!/bin/bash
#
# Runs pyMaBoSS's cMaBoSS tests against the cMaBoSS of this checkout.
#
# usage: scripts/run_pymaboss_tests.sh [CMABOSS]
#   CMABOSS  what to install as cMaBoSS: a wheel, or a source directory
#            (default: engine/python, built by pip)
#
# environment:
#   PYTHON         interpreter to install into and test with (default: python3)
#   PIP_FLAGS      extra flags for pip install (e.g. --break-system-packages)
#   PYMABOSS_REPO  pyMaBoSS git repository (default: vincent-noel/pyMaBoSS)
#   PYMABOSS_REF   branch or tag to test (default: develop)
#
# This cMaBoSS is force-reinstalled first: pip otherwise keeps an installed
# cMaBoSS of the same version (from PyPI, say). It is then named again next to
# pyMaBoSS, so pip has to use it for pyMaBoSS's "cmaboss" requirement: if
# pyMaBoSS asks for a newer version, the install fails instead of quietly
# pulling one from PyPI.

PYTHON=${PYTHON:-python3}
PYMABOSS_REPO=${PYMABOSS_REPO:-https://github.com/vincent-noel/pyMaBoSS.git}
PYMABOSS_REF=${PYMABOSS_REF:-develop}
CMABOSS=$(realpath "${1:-engine/python}")

return_code=0

check()
{
    if [ $? = 0 ]; then
        echo "$1 OK"
    else
        echo "$1 ERR"
        return_code=1
    fi
}

WORK_DIR=$(mktemp -d)
trap 'rm -rf "$WORK_DIR"' EXIT
PYMABOSS_DIR=$WORK_DIR/pyMaBoSS

# Build a source directory once: pip would rebuild it for each install below.
if [ -d "$CMABOSS" ]; then
    $PYTHON -m pip wheel --no-deps -w "$WORK_DIR/wheel" "$CMABOSS" || exit 1
    CMABOSS=$(ls "$WORK_DIR"/wheel/cmaboss-*.whl)
fi

git clone --quiet --depth 1 --branch "$PYMABOSS_REF" "$PYMABOSS_REPO" "$PYMABOSS_DIR" || exit 1
echo "pyMaBoSS $PYMABOSS_REF at $(git -C "$PYMABOSS_DIR" rev-parse --short HEAD)"

$PYTHON -m pip install $PIP_FLAGS --force-reinstall --no-deps "$CMABOSS" || exit 1
$PYTHON -m pip install $PIP_FLAGS "$CMABOSS" "$PYMABOSS_DIR" || exit 1
$PYTHON -m maboss_setup_experimental || exit 1

cd "$PYMABOSS_DIR"

$PYTHON -m unittest test.test_probtrajs_cmaboss
check "probtrajs_cmaboss"
$PYTHON -m unittest test.test_observed_graph_cmaboss
check "observed_graph_cmaboss"
$PYTHON -m unittest test.test_uppmaboss_cmaboss
check "uppmaboss_cmaboss"
# The other test_loadsbml cases run the MaBoSS executables, not cMaBoSS.
$PYTHON -m unittest test.test_loadsbml.TestLoadSBML.test_load_sbml_cmaboss
check "load_sbml_cmaboss"

exit $return_code
