"""Branch counters of the sandbox ports (which code paths an input exercised); the verification scripts sum
them over the cases to report what was and was not covered."""
import collections

COV = collections.Counter()


def hit(name, n=1):
    COV[name] += n


def reset():
    COV.clear()


def snapshot():
    return dict(COV)
