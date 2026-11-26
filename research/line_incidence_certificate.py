"""Small exhaustive checks of the one-step line lift and exact exponential bases."""

from collections import defaultdict
from fractions import Fraction
from math import gcd

from high_low_join import low_points


def valuation(a, p, cap):
    exponent = 0
    while exponent < cap and a % p == 0:
        a //= p
        exponent += 1
    return exponent


def check_free_digit():
    for u in range(10):
        for v in range(10):
            if gcd(gcd(u, v), 10) != 1:
                continue
            for a in range(10):
                for b in range(10):
                    count = sum((a+u*z) % 10 >= 2 and (b+v*z) % 10 >= 2 for z in range(10))
                    assert count <= 8
    assert 1 <= 8 * Fraction(5, 8) * Fraction(2, 8)


def check_low_lines():
    for k in (2, 4):
        n = k//2
        q = 10**n
        for u, v in [(1, 1), (1, -7), (2, 5), (-4, 5), (5, 8), (1, 25), (64, 1)]:
            groups = defaultdict(list)
            for a, b, _ in low_points(k):
                groups[v*a-u*b].append((a, b))
            for group in groups.values():
                origin_a, origin_b = group[0]
                linear = u*origin_b + v*origin_a
                low_classes = {}
                high_classes = {}
                for a, b in group:
                    dz, remainder = divmod(a-origin_a, u)
                    assert remainder == 0 and origin_b+v*dz == b
                    w, z = divmod(dz, q)
                    first, second = origin_a+u*z, origin_b+v*z
                    low_product = sum(int(x)*int(y)*100**i for i, (x, y) in enumerate(
                        zip(str(a)[::-1][:n], str(b)[::-1][:n])))
                    target, remainder = divmod(low_product-first*second, q)
                    derivative = linear + 2*u*v*z
                    assert remainder == 0 and (derivative*w-target) % q == 0
                    for p in (2, 5):
                        e = min(valuation(u*v, p, k), valuation(linear, p, k))
                        d = valuation(derivative, p, n)
                        assert a % p**e == 0 or b % p**e == 0
                        low = z % p**max(0, d-e-1)
                        high = w % p**(n-d)
                        assert low_classes.setdefault((p, d), low) == low
                        assert high_classes.setdefault((p, z), high) == high


def check_bases():
    # k/m = 7629/10000 for point construction and geometric queries.
    count_base, suffix = Fraction(4389, 1000), Fraction(139, 20)
    assert suffix**7629 < count_base**10000
    assert 8**2371 * suffix**5086 < count_base**10000
    # k/m = 1191/1250. Raise the second bound to 5000 to clear sqrt(32).
    base = Fraction(6343, 1000)
    assert suffix**1191 < base**1250
    assert 8**236 * suffix**2382 * 32**1191 < base**5000


if __name__ == "__main__":
    check_free_digit()
    check_low_lines()
    check_bases()
    print("One-step line lifts, digit counts and the 4.389 / 6.343 bounds passed.")
