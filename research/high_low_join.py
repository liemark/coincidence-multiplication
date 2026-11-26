"""Exact small-instance oracle for the high/low point-plane reduction.

Uses a direct incidence scan to check the algebra and prefix generator.
The theoretical time bound uses a 3D range-searching data structure instead.
"""

import re
import subprocess
import sys
from itertools import product
from pathlib import Path


DIGITS = range(2, 10)
PAIRS = [(a, b) for a in DIGITS for b in DIGITS if a * b >= 10]


def high_planes(x, a_digits):
    """Safe candidates for a fixed high prefix of A, at unit scale."""
    width = len(a_digits)
    radix = 10**width
    a = int("".join(map(str, a_digits)))
    u = x * radix + a
    states = [(b, 100*x + a_digits[0]*b) for b in (8, 9)]
    j = 1
    while j < width:
        length = min(2*j, width)
        cell = 10**(width - length)
        remaining = 100**(width - j)
        extensions = []
        for b, value in states:
            # A/t lies in [u,u+1), and A*B/t^2 in [value*remaining,(value+1)*remaining).
            first = max(b*10**(length-j), value*remaining // ((u+1)*cell))
            last = min((b+1)*10**(length-j)-1, (value+1)*remaining // (u*cell))
            for prefix in range(first, last+1):
                blocks = [d*int(e) for d, e in zip(a_digits[j:length], str(prefix)[j:])]
                if min(blocks) < 10:
                    continue
                extended_value = value
                for block in blocks:
                    extended_value = 100*extended_value + block
                extensions.append((prefix, extended_value))
        states = extensions
        j = length
    for b, value in states:
        yield u, b, value - u*b


def low_points(k):
    states = [(0, 0, 0)]
    for j in range(k):
        place = 10**j
        states = [
            (a + u*place, b + v*place, (carry + u*b + v*a)//10)
            for a, b, carry in states for u, v in PAIRS
            if (carry + u*b + v*a) % 10 == 0
        ]
    return states


def join(m, k):
    t = 10**k
    points = low_points(k)
    answers = set()
    for x in range(1, 10):
        for a_digits in product(DIGITS, repeat=m-k):
            for u, v, delta in high_planes(x, a_digits):
                target = t*delta
                for a, b, carry in points:
                    if v*a + u*b + carry == target:
                        factors = (u*t+a, v*t+b)
                        assert factors not in answers
                        answers.add(factors)
    return answers


def check_mod_11():
    states = {(0, 0, 0)}
    sizes = []
    for _ in range(3):
        states = {((u-a) % 11, (v-b) % 11, (p+u*v) % 11)
                  for a, b, p in states for u, v in PAIRS}
        sizes.append(len(states))
    assert sizes == [58, 1150, 1331]
    print("Attainable triples modulo 11 with up to three free pairs:", sizes)


if __name__ == "__main__":
    from verify_enumeration import read_data

    solver = Path(sys.argv[1] if len(sys.argv) > 1 else "solver.exe").resolve()
    published, _ = read_data()
    check_mod_11()
    for m, cuts in [(2, (1,)), (3, (1, 2)), (4, (1, 2, 3)), (5, (3,)), (6, (4,))]:
        out = subprocess.check_output([str(solver)], input=f"{m+1}\n{m}\n4\n\n", text=True)
        unpadded = out.split("Solutions without zero padding:\n")[1]
        expected = {tuple(map(int, p)) for p in re.findall(r"^A = (\d+), B = (\d+)$", unpadded, re.M)}
        assert expected == published[m], m
        for k in cuts:
            assert join(m, k) == expected, (m, k)
        print(f"m = {m}, checked cuts {cuts}, {len(expected)} solutions.")
