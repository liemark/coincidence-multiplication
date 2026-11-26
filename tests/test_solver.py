import re
import subprocess
import sys
from math import gcd
from pathlib import Path


def suffix_products(y, b, digits):
    blocks = [int(a_digit) * int(b_digit) for a_digit, b_digit in zip(str(y).zfill(digits), str(b))]
    fixed = int("".join(f"{block:02d}" for block in blocks))
    unpadded = "".join(map(str, blocks))
    return fixed, unpadded


def brute_force(n, m):
    fixed, unpadded = set(), set()
    q = 10**m
    prefixes = range(10 ** (n - m - 1), 10 ** (n - m))
    for b in range(q // 10, q):
        for y in range(q):
            fixed_suffix, unpadded_text = suffix_products(y, b, m)
            unpadded_suffix = int(unpadded_text)
            scale = 10 ** len(unpadded_text)
            for x in prefixes:
                a = x * q + y
                product = a * b
                if product == x * q * q + fixed_suffix:
                    fixed.add((a, b))
                if product == x * scale + unpadded_suffix:
                    unpadded.add((a, b))
    return fixed, unpadded


def equal_prefix_and_suffix_lengths(m):
    # The cross terms are less than q*q/5 and x >= q/10, so B = q - 1.
    q = 10**m
    b = q - 1
    candidates = list(range(10))
    for place in (10**k for k in range(1, m)):
        extended = (y + digit * place for y in candidates for digit in range(10))
        candidates = [
            y for y in extended if (y * b - suffix_products(y, b, m)[0]) % (10 * place) == 0
        ]

    fixed, unpadded = set(), set()
    for y in candidates:
        padded_suffix, unpadded_text = suffix_products(y, b, m)
        x = (y * b - padded_suffix) // q
        if q // 10 <= x < q:
            a = x * q + y
            fixed.add((a, b))
            if a * b == int(str(x) + unpadded_text):
                unpadded.add((a, b))
    return fixed, unpadded


def near_boundary(n, m):
    """Independent reference for n = 2m-r with small r, using precision doubling."""
    q = 10**m
    first_prefix = 10**(n-m-1)
    fixed, unpadded = set(), set()
    # The cross-term bound is strictly below q^2/5, hence gap < q/(5*first_prefix).
    for gap in range(1, q//(5*first_prefix)):
        b = q-gap
        width = 1
        suffixes = list(range(10))
        while suffixes and width < m:
            new_width = min(2*width, m)
            modulus = 10**new_width
            divisor = gcd(b, modulus)
            period = modulus//divisor
            inverse = pow(b//divisor, -1, period)
            extended = []
            for y in suffixes:
                # Blocks above the known suffix are multiples of 10^(2*width).
                p, _ = suffix_products(y, str(b)[-width:], width)
                if p % divisor:
                    continue
                first = (p//divisor * inverse) % period
                extended.extend(z for z in range(first, modulus, period) if z % 10**width == y)
            width, suffixes = new_width, extended
        for y in suffixes:
            p, unpadded_text = suffix_products(y, b, m)
            x, remainder = divmod(y*b-p, q*gap)
            if remainder or not first_prefix <= x < 10*first_prefix:
                continue
            a = x*q+y
            assert a*b == x*q*q+p
            fixed.add((a, b))
            if a*b == int(str(x)+unpadded_text):
                unpadded.add((a, b))
    return fixed, unpadded


def run_solver(solver, n, m, threads, all_pairs=True):
    command = [solver, "--all"] if all_pairs else [solver]
    return subprocess.check_output(
        command, input=f"{n}\n{m}\n{threads}\n\n", text=True, timeout=30
    )


def check(solver, n, m, threads, expected):
    output = run_solver(solver, n, m, threads)
    fixed, unpadded = output.split("Solutions without zero padding:\n")
    for section, label, expected_pairs in zip(
        (fixed, unpadded), ("Fixed-width", "No-padding"), expected
    ):
        pairs = [(int(a), int(b)) for a, b in re.findall(r"^A = (\d+), B = (\d+)$", section, re.M)]
        count = int(re.search(rf"{label} total: (\d+)", section)[1])
        assert len(pairs) == len(expected_pairs) == count, (n, m, threads, label, count)
        assert set(pairs) == expected_pairs, (n, m, threads, label)


def main(solver):
    for n, m in [(2, 1), (3, 2), (4, 2), (5, 2), (4, 3)]:
        expected = brute_force(n, m)
        for threads in (1, 8):
            check(solver, n, m, threads, expected)

    for m in (18, 19, 41):
        expected = equal_prefix_and_suffix_lengths(m)
        check(solver, 2 * m, m, 4, expected)

    for n, m in ((5, 3), (37, 19), (12, 7)):
        expected = near_boundary(n, m)
        check(solver, n, m, 4, expected)

    output = run_solver(solver, 40, 1, 1, all_pairs=False)
    assert f"A in [{10**39}, {10**40 - 1}], B = 1" in output
    assert f"No-padding total: {9 * 10**39}" in output
    print("All solver checks passed.")


if __name__ == "__main__":
    main(Path(sys.argv[1]).resolve())
