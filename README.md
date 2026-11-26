# coincidence-multiplication

Given an `n`-digit factor `A` and an `m`-digit factor `B` with `n > m`, align the factors on the right. Keep the first `n - m` digits of `A` and append the products of the aligned digits from left to right. A coincidence occurs when this concatenation equals the ordinary product `A × B`.

**研究分析（中文）** · [问题定义、位数分界、算法与实验](research/README.md) · [复现实验](research/reproduce.md)

For the unpadded rule with `n = m + 1`, two independent exhaustive searches agree on all 40 pairs for `2 <= m <= 12`. The counts for `m = 9, 10, 11, 12` are all zero. [Counts by digit length](research/counts.csv) · [All 40 pairs](research/solutions.csv)

The solver reports two rules. **Without zero padding**, each digit product uses its usual decimal representation. With **fixed-width** blocks, each digit product occupies two places, so `3 × 2` contributes `06`. The pair `164 × 94` satisfies both rules because the concatenation `1 | 54 | 16` equals `15416`.

## Build and run

The source requires a C++17 compiler and the Boost headers. Boost.Multiprecision is header-only.

Use these commands on Linux with Boost available on the compiler's include path.

```sh
g++ -std=c++17 -O3 -pthread main.cpp -o solver
./solver
```

On Windows, run the included x64 `solver.exe`, or build it from an x64 Native Tools Command Prompt. Set the include path to the directory containing the `boost` folder.

```bat
cl /nologo /std:c++17 /O2 /EHsc /MT /I"C:\path\to\boost-headers" main.cpp /Fe:solver.exe
solver.exe
```

Enter the two digit counts and the number of threads. For `n = 4` and `m = 3`, the solver reports 54 fixed-width and 8 unpadded solutions. By default it displays up to 1,000 fixed-width pairs and counts them all. It also displays all unpadded solutions, writing the family `B = 1` as the exact range of eligible values of `A`. Pass `--all` to list every pair individually.

## Search method

Let `q = 10^m` and write `A = xq + y`, with `y` the last `m` digits of `A`. If `P` is the fixed-width concatenation of the aligned digit products alone, a solution satisfies

```text
yB - P = xq(q - B)
```

In the ordinary expansion of `yB`, the terms with matching digit positions sum to `P`. The remaining cross terms are nonnegative, so their sum reaches its maximum when both sets of digits are all nines. This maximum is `2(q - 1)(q - 10)/11`. For `m >= 2`, it bounds `q - B` by `floor((2q - 22)/(11 × 10^(n - m - 1)))`. Each choice of low digits of `B` already fixes a minimum gap from `q`, so a branch can be discarded as soon as that minimum exceeds the bound.

The search builds the low digits first. At every depth the cross terms computed so far must be divisible by the corresponding power of ten, because any higher digit contributes a multiple of that power. A lookup table supplies only digit pairs that satisfy the next carry condition. At the final digit, dividing the carry by `q - B` determines `x` exactly. Workers draw independent units-digit pairs from a shared queue. A common trailing zero in both factors can be removed for the search and restored afterward, since both products then gain two trailing zeros.

If every aligned digit product has two digits, both rules agree. Otherwise the unpadded suffix has at most `2m - 1` digits and its value is at most `P <= yB`. Meanwhile `xqB >= x × 10^(2m - 1)`, so equality with the shorter concatenation requires `B = 10^(m - 1)` and an unpadded suffix of exactly `2m - 1` digits. For that value of `B`, each aligned product occupies one place, giving a suffix of length `m`. This leaves `m = 1` and `B = 1`, which works for every `n`-digit `A`.

The solver uses exact 64-bit arithmetic for safe carry updates and switches to arbitrary-precision integers when needed. The number of branches depends strongly on both digit counts. Searches with large `m` and a small difference `n - m` can still take a long time.

## Verification

Run `python tests/test_solver.py ./solver` or `python tests/test_solver.py solver.exe`. The checks compare both output sets against exhaustive multiplication on small inputs and verify large-integer cases across the switch from 64-bit to arbitrary-precision arithmetic.
