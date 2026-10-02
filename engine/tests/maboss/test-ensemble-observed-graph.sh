#!/bin/bash
#
# test-ensemble-observed-graph.sh
#
# The observed graph of an ensemble run must be built the same way as for a
# single model. Until 2026-10, EnsembleEngine handed every worker thread one
# shared graph, reported the (empty) per-thread graphs instead, and so always
# wrote an all-zero graph. Two self-consistency checks, no reference files:
#
#   1. An ensemble of a single model runs exactly the same trajectories as a
#      plain run of that model, so both observed graphs (counts and averaged
#      durations) must be identical.
#   2. Each trajectory's seed depends only on its index, so the transition
#      counts of a two-model ensemble must not depend on the thread count.

. `dirname $0`/share.sh

return_code=0

if [ ! -x $MABOSS ]
then
    echo $MABOSS not found
    exit 1
fi

check_file()
{
    if [ $? = 0 ]; then
	echo "File $1 OK"
    else
	echo "File $1 ** error: differences found **"
	return_code=1
    fi
}

GRAPH="AKT2.in_graph = TRUE; Apoptosis.in_graph = TRUE; CDH1.in_graph = TRUE;"
SAMPLES="sample_count = 2000;"

echo
echo "Ensemble observed graph: one-model ensemble against a plain run"
rm -rf tmp; mkdir -p tmp
$LAUNCHER $MABOSS -c ensemble/ensemble.cfg -e "$GRAPH" -e "$SAMPLES" -e "thread_count = 1;" \
    -o tmp/single ensemble/invasion/Invasion_0.bnet
if [ $? != 0 ]; then exit 1; fi
$LAUNCHER $MABOSS --ensemble -c ensemble/ensemble.cfg -e "$GRAPH" -e "$SAMPLES" -e "thread_count = 1;" \
    -o tmp/ensemble_one ensemble/invasion/Invasion_0.bnet
if [ $? != 0 ]; then exit 1; fi

# A graph with no recorded transition at all would trivially "match" another empty one.
transitions=`awk -F'\t' 'NR>1 { for (i = 2; i <= NF; i++) s += $i } END { print s + 0 }' tmp/single_observed_graph.csv`
[ "$transitions" -gt 0 ]
check_file "observed_graph (plain run records transitions: $transitions)"

cmp -s tmp/single_observed_graph.csv tmp/ensemble_one_observed_graph.csv
check_file "observed_graph (one-model ensemble = plain run)"
cmp -s tmp/single_observed_durations.csv tmp/ensemble_one_observed_durations.csv
check_file "observed_durations (one-model ensemble = plain run)"

echo
echo "Ensemble observed graph: independent of the thread count"
for threads in 1 6; do
    $LAUNCHER $MABOSS --ensemble -c ensemble/ensemble.cfg -e "$GRAPH" -e "$SAMPLES" -e "thread_count = $threads;" \
        -o tmp/ensemble_$threads ensemble/invasion/Invasion_0.bnet ensemble/invasion/Invasion_200.bnet
    if [ $? != 0 ]; then exit 1; fi
done
cmp -s tmp/ensemble_1_observed_graph.csv tmp/ensemble_6_observed_graph.csv
check_file "observed_graph (1 thread = 6 threads)"

exit $return_code
