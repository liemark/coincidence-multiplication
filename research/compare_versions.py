"""Compare two release builds on fixed inputs, checking both full result sets."""

import argparse
import re
import subprocess
from pathlib import Path
from statistics import median
from time import perf_counter


def run(exe, legacy, n, m, threads):
    start = perf_counter()
    out = subprocess.check_output([str(exe)], input=f"{n}\n{m}\n{threads}\n\n", text=True)
    seconds = perf_counter()-start
    divider = ("Solutions that satisfy the no-zero-padding rule:" if legacy
               else "Solutions without zero padding:")
    padded, simple = out.split(divider)
    sets = []
    for section in (padded, simple):
        entries = re.findall(r"^\s*A = (\d+), B = (\d+)$", section, re.MULTILINE)
        assert len(set(entries)) == len(entries)
        sets.append(set(entries))
    if legacy:
        totals = [re.search(r"Found a total of (\d+) solution groups", out),
                  re.search(r"Found a total of (\d+) special solutions", out)]
    else:
        totals = [re.search(r"Fixed-width total: (\d+)", out), re.search(r"No-padding total: (\d+)", out)]
    assert all(len(s) == int(total[1]) for s, total in zip(sets, totals))
    return seconds, sets


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("legacy", type=Path)
    parser.add_argument("current", type=Path)
    args = parser.parse_args()
    builds = [args.legacy.resolve(), args.current.resolve()]
    for n, m, threads in [(8, 7, 1), (8, 7, 8), (10, 7, 1)]:
        samples = [[], []]
        reference = None
        for repeat in range(3):
            for index in ((0, 1) if repeat % 2 == 0 else (1, 0)):
                elapsed, sets = run(builds[index], index == 0, n, m, threads)
                if reference is None:
                    reference = sets
                assert sets == reference
                samples[index].append(elapsed)
        before, after = map(median, samples)
        print(f"{n}x{m} threads={threads} pairs={list(map(len, reference))} "
              f"median={before:.4f}s/{after:.4f}s ratio={before/after:.1f} "
              f"samples(s)={[[round(v, 4) for v in row] for row in samples]}", flush=True)
