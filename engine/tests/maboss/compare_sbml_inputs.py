#!/usr/bin/env python3
#
# compare_sbml_inputs.py REFERENCE.sbml EXPORTED.sbml
#
# Checks that every qual:transition of an SBML file exported by MaBoSS lists
# the same regulators (qual:input) as a reference SBML-qual file of the same
# model. MaBoSS also lists each node as an input of its own transition, since
# the rate-to-logic rewrite always mentions it; that self-input is ignored.

import sys
import xml.etree.ElementTree as ET

QUAL = "{http://www.sbml.org/sbml/level3/version1/qual/version1}"


def inputs_by_output(path):
    result = {}
    for transition in ET.parse(path).iter(QUAL + "transition"):
        outputs = [o.get(QUAL + "qualitativeSpecies") for o in transition.iter(QUAL + "output")]
        inputs = {i.get(QUAL + "qualitativeSpecies") for i in transition.iter(QUAL + "input")}
        for output in outputs:
            result[output] = inputs - {output}
    return result


reference = inputs_by_output(sys.argv[1])
exported = inputs_by_output(sys.argv[2])

errors = 0
for species in sorted(set(reference) | set(exported)):
    expected = reference.get(species, set())
    found = exported.get(species, set())
    if expected != found:
        errors += 1
        print("%s: missing %s, unexpected %s" % (species, sorted(expected - found), sorted(found - expected)))

if not reference:
    print("no transitions found in %s" % sys.argv[1])
    errors += 1

sys.exit(1 if errors else 0)
