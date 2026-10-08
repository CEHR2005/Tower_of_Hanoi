#!/usr/bin/env python3
"""Black-box checks; standard library only."""
import subprocess
import sys

exe = sys.argv[1]

def run(*args, ok=True):
    result = subprocess.run([exe, *map(str, args)], capture_output=True, text=True, timeout=10)
    assert (result.returncode == 0) == ok, (args, result)
    if not ok:
        assert "error:" in result.stderr, result
    return result.stdout

assert "Usage:" in run("--help")
for n in [0, 1, 2, 32, 63, 64]:
    assert run(n, "--count") == f"{2**n - 1}\n"
expected = "1 A -> C\n2 A -> B\n1 C -> B\n3 A -> C\n1 B -> A\n2 B -> C\n1 A -> C\n"
checksums = []
for algorithm in ["recursive", "iterative", "bitwise", "formula"]:
    assert run(3, "--algorithm", algorithm, "--all") == expected + "generated=7 total=7 complete=yes\n"
    checksums.append(run(10, "--algorithm", algorithm, "--mode", "quiet", "--all"))
assert len(set(checksums)) == 1
assert run(10, "--mode", "quiet", "--all") == checksums[0]
assert "generated=1000 total=2047 complete=no" in run(11)
assert "generated=1000000 total=1048575 complete=no" in run(20, "--mode", "quiet")
assert "generated=0 total=7 complete=no" in run(3, "--limit", 0)
assert "generated=0 total=0 complete=yes" in run(0)
assert "generated=1 total=1 complete=yes" in run(1, "--limit", 100)
assert "generated=1 total=18446744073709551615 complete=no" in run(64, "--limit", 1)
assert "generated=7" in run(3, "--limit", 2**64-1)
ascii_text = run(2, "--mode", "ascii", "--all", "--delay-ms", 0)
assert ascii_text.count("Step ") == 4 and "\x1b" not in ascii_text
assert ascii_text == (
    "Step 0\n ===     |      |    \n=====    |      |    \n  A      B      C    \n"
    "Step 1\n  |      |      |    \n=====   ===     |    \n  A      B      C    \n"
    "Step 2\n  |      |      |    \n  |     ===   =====  \n  A      B      C    \n"
    "Step 3\n  |      |     ===   \n  |      |    =====  \n  A      B      C    \n"
    "generated=3 total=3 complete=yes\n"
), repr(ascii_text)
assert run(1, "--mode", "ansi", "--all").count("\x1b[2J\x1b[H") == 2
assert "Step 1" in run(1, "--mode", "ascii", "--delay-ms", 1)
for args in [[], ["-1"], ["65"], ["1x"], [""], ["+1"], [str(2**64)],
             ["3", "--mode", "bad"], ["3", "--algorithm", "bad"], ["3", "--limit"],
             ["3", "--limit", "-1"], ["3", "--limit", str(2**64)],
             ["3", "--limit", "0", "--all"], ["3", "--all", "--limit", "1"],
             ["3", "--delay-ms", "1"], ["3", "--mode", "ascii", "--delay-ms", "60001"],
             ["3", "--count", "--all"], ["3", "--count", "--mode", "quiet"],
             ["3", "--count", "--algorithm", "formula"], ["3", "--count", "--count"],
             ["3", "--mode", "quiet", "--mode", "moves"], ["3", "--unknown"]]:
    run(*args, ok=False)
with open("/dev/full", "w") as full:
    result = subprocess.run([exe, "1", "--count"], stdout=full, stderr=subprocess.PIPE, text=True)
    assert result.returncode != 0 and "output failed" in result.stderr
print("CLI tests passed")
