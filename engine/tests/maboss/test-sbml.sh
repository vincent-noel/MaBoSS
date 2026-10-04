#!/bin/sh
#
# test-sbml.sh
# (very) basic test of SBML qual compatibility

. `dirname $0`/share.sh

return_code=0

if [ ! -x $MABOSS ]
then
    echo $MABOSS not found
    exit 1
fi

check_file()
{
    # echo
    if [ $? = 0 ]; then
	echo "File $1 OK"
    else
	echo "File $1 ** error: differences found **"
    return_code=1
    fi
}

echo
echo "Testing SBML compatibility"
rm -rf tmp; mkdir -p tmp
$LAUNCHER $MABOSS sbml/cell_fate.sbml -c sbml/cell_fate.cfg -o tmp/sbml_cell_fate
if [ $? != 0 ]; then exit 1; fi
${PYTHON:-python} compare_probtrajs.py sbml/refer/cell_fate_probtraj.csv tmp/sbml_cell_fate_probtraj.csv --exact
check_file "projtraj"

$LAUNCHER $MABOSS sbml/cell_fate.bnd -c sbml/cell_fate.bnd.cfg -o tmp/cell_fate
if [ $? != 0 ]; then exit 1; fi
${PYTHON:-python} compare_probtrajs.py tmp/sbml_cell_fate_probtraj.csv tmp/cell_fate_probtraj.csv --exact
check_file "projtraj"

# Export the .bnd model and check that each exported transition lists the
# regulators of the reference SBML. Most of them sit behind "@logic" or "!",
# which the exporter used to drop.
$LAUNCHER $MABOSS -c sbml/cell_fate.bnd.cfg -x tmp/cell_fate_export.sbml sbml/cell_fate.bnd
if [ $? != 0 ]; then exit 1; fi
${PYTHON:-python} compare_sbml_inputs.py sbml/cell_fate.sbml tmp/cell_fate_export.sbml
check_file "exported transition inputs"

exit $return_code