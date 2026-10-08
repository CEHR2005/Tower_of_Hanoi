#!/usr/bin/env python3
"""Run each sample in a fresh process; write raw samples and a median table."""
import argparse
import csv
import hashlib
import os
import platform
import random
import statistics
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", default="build/hanoi_bench")
    parser.add_argument("--output", default="benchmarks/results.csv")
    parser.add_argument("--sizes", nargs="+", type=int, default=[16, 20, 24, 26])
    parser.add_argument("--repeats", type=int, default=5)
    parser.add_argument("--cpu", type=int)
    args = parser.parse_args()
    if args.repeats < 1 or any(n < 0 or n > 30 for n in args.sizes):
        parser.error("repeats >= 1 and sizes in [0,30] required")
    if args.cpu is not None:
        os.sched_setaffinity(0, {args.cpu})
    algorithms = ["recursive", "iterative", "bitwise", "formula"]
    # Warm code/CPU before randomized, balanced sampling; startup is outside C++ timer.
    for algorithm in algorithms:
        subprocess.run([args.binary, algorithm, str(min(max(args.sizes), 20))], check=True, stdout=subprocess.DEVNULL)
    jobs = [(n, a, r) for n in args.sizes for a in algorithms for r in range(args.repeats)]
    random.Random(42).shuffle(jobs)
    rows = []
    checksums = {}
    for n, algorithm, repeat in jobs:
        output = subprocess.check_output([args.binary, algorithm, str(n)], text=True)
        row = next(csv.reader([output.strip()]))
        assert len(row) == 7 and row[0] == algorithm and int(row[1]) == n
        assert int(row[2]) == 2**n - 1 and float(row[3]) > 0 and int(row[5]) > 0
        checksums.setdefault(n, row[6])
        assert checksums[n] == row[6], f"checksum mismatch at N={n}"
        rows.append([repeat, *row])
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w") as file:
        writer = csv.writer(file)
        writer.writerow(["repeat", "algorithm", "n", "moves", "seconds", "moves_per_second", "peak_rss_kib", "checksum"])
        writer.writerows(rows)
    metadata = [platform.platform(), "binary_sha256=" + hashlib.sha256(Path(args.binary).read_bytes()).hexdigest(), "affinity=" + str(sorted(os.sched_getaffinity(0))),
                "command=" + repr(vars(args)), "compiler=" + subprocess.check_output(["c++", "--version"], text=True),
                "os_release=" + Path("/etc/os-release").read_text(),
                "cpuinfo=" + next((line for line in Path("/proc/cpuinfo").read_text().splitlines() if line.startswith("model name")), "unknown")]
    output.with_suffix(".environment.txt").write_text("\n".join(metadata) + "\n")
    print("| N | Algorithm | Median s | Range s | M moves/s | Peak RSS KiB (max) |")
    print("|---:|---|---:|---:|---:|---:|")
    for n in sorted(set(args.sizes)):
        for algorithm in algorithms:
            samples = [r for r in rows if int(r[2]) == n and r[1] == algorithm]
            times = [float(r[4]) for r in samples]
            median = statistics.median(times)
            print(f"| {n} | {algorithm} | {median:.6f} | {min(times):.6f}–{max(times):.6f} | {(2**n-1)/median/1e6:.2f} | {max(int(r[6]) for r in samples)} |")


if __name__ == "__main__":
    main()
