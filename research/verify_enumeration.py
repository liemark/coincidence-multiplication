"""Check the published pairs and counts. Run from any directory with Python 3."""

import argparse
import csv
import re
import subprocess
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent


def read_data():
    with (ROOT / "solutions.csv").open(encoding="utf-8", newline="") as f:
        pairs = defaultdict(set)
        for row in csv.DictReader(f):
            m, a, b = (int(row[key]) for key in ("m", "A", "B"))
            digits_a, digits_b = str(a), str(b)
            assert len(digits_a) == m+1 and len(digits_b) == m
            fake = digits_a[0] + "".join(str(int(u)*int(v)) for u, v in zip(digits_a[1:], digits_b))
            assert a*b == int(fake) and (a, b) not in pairs[m]
            pairs[m].add((a, b))
    with (ROOT / "counts.csv").open(encoding="utf-8", newline="") as f:
        counts = {}
        for row in csv.DictReader(f):
            m, leaves, size = (int(row[key]) for key in ("m", "leaves", "solutions"))
            assert m not in counts and len(pairs[m]) == size
            counts[m] = (leaves, size)
    assert set(counts) == set(range(2, 13)) == set(pairs)
    return pairs, counts


def high_to_low(m):
    """Exhaust all completions of high digits, using only product intervals."""
    radix = [10**j for j in range(m+1)]
    # Each entry appends one digit to each factor and one two-digit block to C.
    extensions = [(u, v, u*v) for u in range(2, 10) for v in range(2, 10) if u*v >= 10]
    result = set()

    def visit(a, b, c, missing):
        t = radix[missing]
        # The possible ordinary products and concatenations must overlap.
        if a*b > c or ((a+1)*t-1)*((b+1)*t-1) < c*t*t:
            return
        if not missing:
            assert a*b == c and (a, b) not in result
            result.add((a, b))
            return
        for u, v, block in extensions:
            visit(10*a + u, 10*b + v, 100*c + block, missing-1)

    for x in range(1, 10):
        visit(x, 0, x, m)
    return result


def check_counter(executable, pairs, counts, *, has_leaves):
    output = subprocess.check_output([str(executable.resolve()), "--list"], text=True)
    measured, measured_pairs = {}, defaultdict(set)
    m = None
    for line in output.splitlines():
        header = re.fullmatch(r"m=(\d+)(?: leaves=(\d+))? solutions=(\d+)", line)
        if header:
            m, leaves, size = header.groups()
            m = int(m)
            assert m not in measured and (leaves is not None) == has_leaves
            measured[m] = (int(leaves), int(size)) if has_leaves else int(size)
        else:
            match = re.fullmatch(r"A=(\d+) B=(\d+)", line)
            assert match and m is not None, line
            pair = tuple(map(int, match.groups()))
            assert pair not in measured_pairs[m]
            measured_pairs[m].add(pair)
    assert measured == (counts if has_leaves else {m: size for m, (_, size) in counts.items()})
    assert all(measured_pairs[m] == pairs[m] for m in counts)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--counter", type=Path, help="also run the C++ counter through m=12")
    parser.add_argument("--interval-counter", type=Path, help="also run the C++ high-to-low search through m=12")
    parser.add_argument("--prefix-max", type=int, choices=range(2, 13), default=6,
                        help="last digit count for the independent high-to-low search (default 6)")
    args = parser.parse_args()
    pairs, counts = read_data()
    for m in range(2, args.prefix_max+1):
        assert high_to_low(m) == pairs[m], m
        print(f"Independent high-to-low enumeration: m={m}, {len(pairs[m])} pairs.", flush=True)
    if args.counter:
        check_counter(args.counter, pairs, counts, has_leaves=True)
        print("All published pairs and leaf counts match the C++ run through m=12.")
    if args.interval_counter:
        check_counter(args.interval_counter, pairs, counts, has_leaves=False)
        print("The independent interval search matches every pair through m=12.")
