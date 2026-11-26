"""Finite bounds used in the unpadded n = m + 1 search-tree proof (Python 3)."""

from collections import Counter
from fractions import Fraction
from math import gcd


DIGIT_PAIRS = [(a, b) for a in range(2, 10) for b in range(2, 10) if a * b >= 10]
TWO_DIGIT_PAIRS = [(a + 10 * c, b + 10 * d)
                   for a, b in DIGIT_PAIRS for c, d in DIGIT_PAIRS]


def largest_fibre(pairs, modulus, a, b):
    counts = Counter((a * u + b * v) % modulus for u, v in pairs)
    return max(counts.values())


def check_block_updates():
    # Locally feasible suffixes built by extending one legal digit pair at a time.
    # Membership below is rechecked by ordinary multiplication and literal concatenation.
    samples = [
        (1, 3232333223233223223222233233222, 5947878967554856557976845957955),
        (2, 3332232422222242243323442323342, 8997897855999576689869886899986),
        (4, 3224424342342224342432324233252, 7977798975887668776777757777756),
        (5, 4443353234555426462623225222225, 3345424643332463326264652556585),
        (16, 2223432224222223422223222244592, 9655449654566584386794587949648),
    ]

    def concatenate(a, b):
        left, right = str(a), str(b)
        assert len(left) == len(right)
        return int("".join(f"{int(u)*int(v):02d}" for u, v in zip(left, right)))

    place = 10**24
    for h, a, b in samples:
        y, z = a % place, b % place
        assert gcd(gcd(y, z), place) == h
        carry, remainder = divmod(y*z-concatenate(y, z), place)
        assert remainder == carry % h == 0
        for length in (2, 4, 5, 6, 7):
            modulus = 10**length
            if length == 2:
                blocks = TWO_DIGIT_PAIRS
            else:
                # Include a known feasible extension and every legal single-column edit.
                u0, v0 = (a//place) % modulus, (b//place) % modulus
                blocks = set()
                for j in range(length):
                    pos = 10**j
                    u1, v1 = (u0//pos) % 10, (v0//pos) % 10
                    blocks.update((u0+(u-u1)*pos, v0+(v-v1)*pos) for u, v in DIGIT_PAIRS)
            outcomes = set()
            for u, v in blocks:
                first, second = y+place*u, z+place*v
                cross = first*second-concatenate(first, second)
                numerator = carry+z*u+y*v+place*(u*v-concatenate(u, v))
                assert cross == place*numerator
                direct = cross % (place*modulus*h) == 0
                assert direct == ((carry//h + (z//h)*u + (y//h)*v) % modulus == 0)
                outcomes.add(direct)
            assert outcomes == {False, True}
    print("Block updates agree with direct big-integer products at widths 2, 4, 5, 6 and 7.")


def check_primitive_columns():
    # The coefficients have no common factor with 10. Their common scale has
    # already been divided out. Decimal multiples cannot occur individually.
    one_digit_max = max(
        largest_fibre(DIGIT_PAIRS, 10, a, b)
        for a in range(1, 10) for b in range(1, 10) if gcd(gcd(a, b), 10) == 1
    )
    slopes = {s: largest_fibre(TWO_DIGIT_PAIRS, 100, 1, s)
              for s in range(100) if s % 10}
    mixed_max = max(
        largest_fibre(TWO_DIGIT_PAIRS, 100, a, b)
        for a in range(2, 100, 2) if a % 5 for b in range(5, 100, 10)
    )
    assert one_digit_max == 8
    assert slopes[1] == 64
    assert max(size for s, size in slopes.items() if s != 1) == mixed_max == 56
    print("Maximum fibre: one digit 8, ordinary two digits 56, slope one 64.")


def check_equal_suffixes():
    candidates = {a for a, b in DIGIT_PAIRS if a == b}
    sizes = [len(candidates)]
    at_lengths = {}
    for k in range(2, 7):
        place = 10 ** (k - 1)
        extensions = {a * place + low for low in candidates for a in range(4, 10)}
        candidates = {
            y for y in extensions
            if (y * y - sum(int(d) ** 2 * 100 ** i for i, d in enumerate(str(y)[::-1])))
            % (10 * place) == 0
        }
        sizes.append(len(candidates))
        at_lengths[k] = sorted(candidates)
    assert sizes == [6, 11, 3, 2, 1, 0]
    assert at_lengths[3] == [754, 756, 758]
    assert at_lengths[4] == [8754, 8758]
    print("Viable equal suffixes at lengths 1 through 6:", sizes)


def check_delayed_digit():
    sums = Counter((a + b) % 10 for a, b in DIGIT_PAIRS)
    assert sums[1] == 8 and max(n for r, n in sums.items() if r != 1) == 7
    assert {a + b for a, b in DIGIT_PAIRS if (a + b) % 10 == 1} == {11}
    # When the first column has eight choices their carry is identical.
    # Check the second column after the delayed term e*b changes its target.
    for r in range(10):
        choices = [(a, b) for a, b in DIGIT_PAIRS if (a + b) % 10 == r]
        for e in range(1, 10):
            for target in range(10):
                if len(choices) == 8:
                    assert sum(sums[(target - e * b) % 10] for _, b in choices) <= 56
                else:
                    assert 8 * len(choices) <= 56
    print("Paired columns with a delayed nonzero coefficient: at most 56 choices.")


def check_even_gcd_equal_slopes():
    # If H is even and y/H = B/H mod 100, the two low digits agree.
    # The mod-100 cross-term equation is necessary at these two positions.
    equal_even = []
    for low, other in TWO_DIGIT_PAIRS:
        if low != other or low % 2:
            continue
        tens, units = divmod(low, 10)
        if (low**2 - (100*tens**2 + units**2)) % 100 == 0:
            equal_even.append(low)
    assert sorted(equal_even) == [54, 56, 58]
    # If 4 divides H and m >= 2 then 16 divides AB. Blocks above the
    # lowest two and the prefix contribution are multiples of 16.
    for low in equal_even:
        tens, units = divmod(low, 10)
        assert (units**2 + 100*tens**2) % 16 != 0
    print("Equal low two digits with even H force H = 2.")


def check_common_factor_25():
    states = [(a, b, 0) for a, b in DIGIT_PAIRS]
    for place in (10, 100):
        states = [
            (a + place*u, b + place*v, (carry + u*b + v*a) // 10)
            for a, b, carry in states for u, v in DIGIT_PAIRS
            if (carry + u*b + v*a) % 10 == 0
        ]
    assert all(a % 25 or b % 25 for a, b, _ in states)
    print("No feasible pair of three-digit suffixes is divisible by 25 on both sides.")


def check_common_power_of_two():
    states = [(0, 0, 0)]
    sizes = []
    for k in range(1, 18):
        place = 10 ** (k - 1)
        modulus = 2 ** k
        states = [
            (a + place*u, b + place*v, (carry + u*b + v*a) // 10)
            for a, b, carry in states for u, v in DIGIT_PAIRS
            if (a + place*u) % modulus == (b + place*v) % modulus == 0
            and (carry + u*b + v*a) % (10*modulus) == 0
        ]
        sizes.append(len(states))
    assert sizes == [13, 26, 12, 10, 16, 18, 20, 12, 8, 12, 12, 12, 10, 8, 4, 2, 0]
    print("Survivors with 2^k dividing both suffixes and the carry:", sizes)


def check_four_digit_agreement():
    # Agreement to three digits after normalization forces even common
    # factors (the possible common suffixes are 754, 756, 758), hence H=2.
    # For agreement to four digits, enumerate the fifth digits as well.
    for suffix in (8754, 8758):
        low_product = sum(int(v)**2 * 100**i for i, v in enumerate(str(suffix)[::-1]))
        for u, v in DIGIT_PAIRS:
            y, b = suffix + 10000*u, suffix + 10000*v
            if (y - b) % 20000 == 0:
                assert (y*b - low_product - u*v*100**4) % 200000 != 0
    print("Normalized slopes congruent to one modulo 10000 cannot extend to five digits.")


def check_slope_two():
    weights = Counter(a + 2*b for a, b in DIGIT_PAIRS)
    assert max(sum(weights[n] for n in range(r, 30, 10)) for r in range(10)) == 7
    # Start with target digit 2, then use only 4. Carries 1 and 2 each
    # have seven continuations within the same two-state set.
    for carry, digit in [(0, 2), (1, 4), (2, 4)]:
        out = [divmod(carry + a + 2*b, 10) for a, b in DIGIT_PAIRS]
        carries = [c for c, r in out if r == digit]
        assert len(carries) == 7 and set(carries) == {1, 2}
    print("For slope 2 the largest length-L fibre is exactly 7^L for every L.")
    states = [(0, 0, 0)]
    sizes = []
    for k in range(1, 5):
        place = 10 ** (k-1)
        states = [
            (a + place*u, b + place*v, (carry + u*b + v*a) // 10)
            for a, b, carry in states for u, v in DIGIT_PAIRS
            if (a + place*u - 2*(b + place*v)) % (10*place) == 0
            and (carry + u*b + v*a) % 10 == 0
        ]
        sizes.append(len(states))
    assert sizes == [6, 2, 2, 0]
    print("Viable suffixes with y = 2B modulo 10^k at k=1..4:", sizes)


def check_rounded_base():
    bound = Fraction(7824, 1000)
    assert Fraction(8 ** 20, 2) < bound ** 20
    assert 8 ** 40 * 4 * Fraction(7, 8) ** 19 < bound ** 40
    assert 8 ** 6 * Fraction(7, 8) < bound ** 6
    print("All three exponential bases are strictly below 7.824.")
    block_bound = Fraction(139, 20)
    # Maxima (and the cutoff at length 6) are certified by block_fibres.cpp.
    for length, count in [(4, 2332), (4, 2199), (5, 15402), (6, 108536),
                          (6, 112696), (7, 693568)]:
        assert count < block_bound**length
    print("All adaptive block bounds are below 6.95^L (exact rational checks).")


if __name__ == "__main__":
    check_block_updates()
    check_primitive_columns()
    check_equal_suffixes()
    check_even_gcd_equal_slopes()
    check_common_factor_25()
    check_common_power_of_two()
    check_four_digit_agreement()
    check_slope_two()
    check_delayed_digit()
    check_rounded_base()
