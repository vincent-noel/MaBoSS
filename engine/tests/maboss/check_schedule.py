import math, sys

# Checks the node probabilities of the schedule/ models against their exact
# values: a schedule time must not add a transition of its own (issue #15).

def node_probas(file):
    with open(file, 'r') as f:
        header = f.readline().strip('\n').split('\t')
        first_index = header.index("State")
        rows = []
        for line in f:
            data = line.strip('\n').split('\t')
            probas = {}
            for state, proba in zip(data[first_index::3], data[first_index+1::3]):
                for node in state.split(" -- "):
                    probas[node] = probas.get(node, 0.) + float(proba)
            rows.append((float(data[0]), probas))
    return rows


def check(rows, expected, atol):
    errors = 0
    for time, probas in rows:
        for node, value in expected(time).items():
            proba = probas.get(node, 0.)
            if abs(proba - value) > atol:
                print("t=%g: P(%s) = %g, expected %g" % (time, node, proba, value))
                errors += 1
    return errors


def main(args):
    model, probtraj = args[1], args[2]
    rows = node_probas(probtraj)

    if model == "fixpoint":
        # The initial state is a fixed point and only C is scheduled (to 0 at
        # t=10): A and B must stay ON, whatever the node declaration order.
        errors = check(rows, lambda t: {"A": 1., "B": 1., "C": 1. if t < 10 else 0.}, 1e-9)

    elif model == "noop":
        # Z flips at rate 0.01 both ways, and the schedule never changes S:
        # P(Z) = 0.5*(1 - exp(-0.02*t)), averaged over each time tick.
        tick = rows[1][0] - rows[0][0]
        def expected(t):
            mean = 0.5 - 0.5 * (math.exp(-0.02 * t) - math.exp(-0.02 * (t + tick))) / (0.02 * tick)
            return {"Z": mean, "S": 1.}
        errors = check(rows, expected, 0.015)

    else:
        print("Unknown model %s" % model)
        return 1

    return 0 if errors == 0 else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
