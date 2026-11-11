#!/bin/sh
#
# test-schedule.sh
# (very) basic test of MaBoSS node scheduling

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
echo "Testing default cell fate model"
rm -rf tmp; mkdir -p tmp
$LAUNCHER $MABOSS cellfate/cellfate.bnd -c cellfate/cellfate.cfg -o tmp/res
if [ $? != 0 ]; then exit 1; fi
python compare_probtrajs.py cellfate/refer/res_probtraj.csv tmp/res_probtraj.csv --exact
check_file "projtraj"

echo
echo "Testing cell fate model with scheduling"
$LAUNCHER $MABOSS cellfate/cellfate.bnd -c cellfate/cellfate_schedule.cfg -o tmp/res_schedule
if [ $? != 0 ]; then exit 1; fi
python compare_probtrajs.py cellfate/refer/res_schedule_probtraj.csv tmp/res_schedule_probtraj.csv --exact
check_file "projtraj"

echo
echo "Testing a schedule on a fixed point: no other node may flip"
$LAUNCHER $MABOSS schedule/fixpoint.bnd -c schedule/fixpoint.cfg -o tmp/res_schedule_fixpoint
if [ $? != 0 ]; then exit 1; fi
python check_schedule.py fixpoint tmp/res_schedule_fixpoint_probtraj.csv
check_file "projtraj"

echo
echo "Testing a schedule that changes nothing: the dynamics must not change"
$LAUNCHER $MABOSS schedule/noop.bnd -c schedule/noop.cfg -o tmp/res_schedule_noop
if [ $? != 0 ]; then exit 1; fi
python check_schedule.py noop tmp/res_schedule_noop_probtraj.csv
check_file "projtraj"

exit $return_code