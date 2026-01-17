# 🏆 PS_TESTER v5.0 - The Ultimate Push_swap Tester

**The most comprehensive tester for 42/1337 push_swap project - if your push_swap passes this, it's 100% bulletproof for evaluations!**

## ✨ What's New in v5.0

- 🔍 **Separate Checker Statistics** - Clear distinction between push_swap and checker results
- 💀 **Crash Detection** - Detects segfaults/crashes in both push_swap AND checker
- ☠️ **Memory Leak Detection for Checker** - Valgrind testing for checker program too!
- 📊 **Improved Final Results** - Beautiful tree-style output with emojis

## 📋 Requirements

- `g++` or `gcc` (C++17 support)
- `valgrind` (optional, for memory leak tests)

**No g++?** Use gcc instead:
```bash
gcc -std=c++17 -O3 -pthread push_swap_ultimate_tester.cpp -o ps_tester -lstdc++
```

**Neither installed?**
```bash
sudo apt install build-essential  # Installs both gcc and g++
```

## ⚡ Quick Start

```bash
# Compile
make

# Run (add YOUR push_swap path!)
./ps_tester ./push_swap

# Quick mode (no valgrind, faster)
./ps_tester ./push_swap --quick --no-valgrind

# With checker (for bonus)
./ps_tester ./push_swap ./checker_linux

# Test checker only
./ps_tester ./push_swap ./checker --checker-only
```

### Makefile Commands

```bash
make          # Compile the tester
make clean    # Remove log files
make fclean   # Remove logs + binary
make re       # Recompile
make help     # Show all commands
```

## 📊 Test Coverage Summary

| Category | Tests | Description |
|----------|-------|-------------|
| **Error Handling** | 90+ | Letters, signs, overflow, duplicates, formats |
| **All 3-elem permutations** | 6 | Complete coverage |
| **All 4-elem permutations** | 24 | Complete coverage (from gemartin) |
| **All 5-elem permutations** | **120** | **COMPLETE coverage - ALL permutations!** |
| **Already Sorted** | 13+ | Empty to 77 elements, must be 0 ops |
| **Big Number Ranges** | 18+ | INT_MIN area, various 500-number ranges |
| **Checker Invalid Ops** | 24+ | All uppercase, extra chars, spaces, etc. |
| **Checker Memory Leaks** | 8+ | Valgrind testing for checker program |
| **Checker Crash Tests** | 35+ | Segfault/crash detection for checker |
| **Memory Leak Tests** | 20+ | All error cases, overflow scenarios |
| **Performance** | 100+ | Size 3, 5, 10, 50, 100, 500 benchmarks |

**TOTAL: 450+ unique test cases!**

## 📈 Final Results Display

The tester now shows **separate statistics** for push_swap and checker:

```
╔══════════════════════════════════════════════════════════════════╗
║ FINAL RESULTS                                                    ║
╚══════════════════════════════════════════════════════════════════╝

  📊 PUSH_SWAP:
  ├─ Total Tests:    150
  ├─ Passed:         150
  ├─ Failed:         0
  ├─ Memory Leaks:   0
  └─ Crashes:        0
     Success Rate:   100.0%

  🔍 CHECKER (Bonus):
  ├─ Total Tests:    43
  ├─ Passed:         43
  ├─ Failed:         0
  ├─ Memory Leaks:   0
  └─ Crashes:        0
     Success Rate:   100.0%
```

### Detection Indicators

| Status | Meaning |
|--------|---------|
| ✓ PASS | Test passed |
| ✗ FAIL | Test failed (wrong output) |
| ☠ LEAK | Memory leak detected |
| 💀 SEGV | Crash/Segfault detected |

## 🎯 Edge Cases Tested

### Error Handling (Must Print "Error")

**Letters:**
- `a`, `abc`, `hello world`
- `1a`, `a1`, `1a2`, `111a11`, `111a111`
- `42 a 41` - letter in middle of list
- `42 41 40 45 101 x 202 -1 224 3` - letter deep in list
- `42 -2 10 11 0 90 45 500 -200 e` - letter at end

**Signs:**
- `+`, `-` (single sign)
- `++5`, `--5`, `++123`, `--123`, `--21345` (double signs)
- `+-5`, `-+5` (mixed signs)
- `5-`, `5+` (trailing signs)
- `111-1`, `3333-3333`, `4222-4222` (sign in middle)
- `3+3`, `111+111` (plus in middle)
- `2147483647+1` (plus after INT_MAX)
- `- 5` (space after sign)

**Overflow:**
- `2147483648` (INT_MAX + 1)
- `2147483649` (INT_MAX + 2)
- `-2147483649` (INT_MIN - 1)
- `-2147483650` (INT_MIN - 2)
- `99999999999999999999` (20 digits)
- `99999999999999999999999999` (26 digits)
- `555555555555555555555555555555555555555555555555` (50+ digits - leak detection!)
- **`1 2 3 99999999999999999999`** ← YOUR SPECIFIC TEST CASE!
- `42 41 40 99999999999999999999999999` (overflow at end)
- `-1 -2 -99999999999999999999` (negative overflow in list)

**Duplicates:**
- `1 1`, `0 0`, `42 42` (simple)
- `-1 -1`, `-3 -2 -2` (negative)
- `3 +3`, `1 +1 -1` (with plus sign)
- `0 -0`, `0 +0`, `0 -0 1 -1`, `0 +0 1 -1` (zero variations)
- `10 -1 -2 -3 -4 -5 -6 90 99 10` (duplicate at ends)
- `0 1 2 3 4 5 0` (duplicate first and last)
- `42 -42 -42` (negative duplicate)

**Hidden Duplicates (Leading Zeros):**
- `1 01`, `8 008 12` (hidden by leading zeros)
- `-01 -001` (negative leading zeros)
- `00000001 1 9 3` (many leading zeros)
- `111111 -4 3 03` (mixed with 03)
- `00000003 003 9 1` (both with leading zeros)
- `0000000000000000000000009 000000000000000000000009` (huge leading zeros)
- `-000 -0000` (negative zeros)
- `-00042 -000042` (negative with leading zeros)

**Format:**
- Empty string `""`
- Just spaces `"   "`
- Tab character
- Newline character `\n`
- Multiple empty strings

### All 150 Small Permutations

- **6 permutations** of 3 elements (1,2,3)
- **24 permutations** of 4 elements (1,2,3,4)
- **120 permutations** of 5 elements (1,2,3,4,5) ← ALL OF THEM!

### Already Sorted Tests (Must Output 0 Operations)

| Input | Expected |
|-------|----------|
| Empty | 0 ops |
| `1` | 0 ops |
| `1 2` | 0 ops |
| `1 2 3` | 0 ops |
| `0 1 2 3 4` | 0 ops |
| `6 7 8` | 0 ops |
| `1 2 3 4 5 6 7 8 9` | 0 ops |
| `1...30` (30 numbers) | 0 ops |
| `1...50` (50 numbers) | 0 ops |
| `1...77` (77 numbers) | 0 ops |
| `2147483645 2147483646 2147483647` | 0 ops |
| `-2147483648 -2147483647 -2147483646` | 0 ops |

### Big Number Range Tests (500 numbers shuffled)

| Range | Count |
|-------|-------|
| 0 to 499 | 500 |
| 5000 to 5499 | 500 |
| 50000 to 50499 | 500 |
| 500000 to 500499 | 500 |
| 5000000 to 5000499 | 500 |
| 50000000 to 50000499 | 500 |
| -500 to -1 | 500 |
| -50000 to -49501 | 500 |
| 100 to 599 | 500 |
| 250 to 720 | 471 |
| 10000 to 10479 | 480 |
| 100 to 450 | 351 |
| -500 to -50 | 451 |
| -500 to -9 | 492 |
| 0 to 450 | 451 |
| -1 to 498 | 500 |
| **-2147483648 to -2147483149** | 500 (INT_MIN area!) |

### Checker Invalid Instructions (Must Print "Error")

| Invalid Instruction | Reason |
|---------------------|--------|
| `SA`, `SB`, `PA`, `PB` | Uppercase |
| `RA`, `RB`, `RRA`, `RRB`, `RRR` | Uppercase |
| `saa`, `paa`, `raa`, `rraa` | Extra character |
| `sa ` | Trailing space |
| ` sa` | Leading space |
| `sa\n\n` | Extra newline |
| `swap`, `push_a`, `push_b`, `rotate` | Wrong name |
| `hello world` | Garbage |
| `123` | Number |
| Empty line | Just newline |
| `sa\nswap\n` | Mixed valid/invalid |

### Memory Leak Tests

All error cases above are also tested with Valgrind to ensure NO memory leaks on error paths:
- Invalid characters
- Overflow (including 50+ digit numbers)
- **`1 2 3 99999999999999999999`** - valid then overflow
- Duplicates (all variations)
- Sign errors
- Leading zeros duplicates

## 📈 42 Grading Thresholds

| Size | 5/5 ⭐⭐⭐⭐⭐ | 4/5 ⭐⭐⭐⭐ | 3/5 ⭐⭐⭐ | 2/5 ⭐⭐ |
|------|-----|-----|-----|-----|
| 3 | ≤3 | ≤4 | ≤5 | ≤6 |
| 5 | ≤12 | ≤15 | ≤18 | ≤22 |
| 100 | ≤700 | ≤900 | ≤1100 | ≤1300 |
| 500 | ≤5500 | ≤7000 | ≤8500 | ≤10000 |

## 🔧 Options

| Flag | Description |
|------|-------------|
| `--no-valgrind` | Skip memory leak detection (faster) |
| `--quick` | Fewer iterations, faster testing |
| `--stress` | Extra stress tests (more iterations) |
| `--html` | Generate HTML report |
| `--checker-only` | Only test checker program (bonus) |

## 📁 Output Files

- `trace.log` - Detailed trace of all operations
- `errors.txt` - Failed tests with inputs for debugging
- `report.html` - Visual HTML report (with --html)

## 🧪 Test Sources Integrated

This tester combines edge cases from:
- **gemartin's tester** (45+ error tests, all 4/5-elem permutations)
- **push_swap_ultimate_tester** (comprehensive 5000+ line tester)
- **42 evaluation checklist** (all required test cases)
- **Custom edge cases** (overflow in lists, leading zeros, sign variations)

## ✅ If Your push_swap Passes All Tests

🎉 **Congratulations!** Your implementation handles:
- All possible error cases the evaluators can throw at it
- All small input permutations (no edge case missed)
- Already sorted inputs correctly (0 operations)
- Big numbers near INT_MIN and INT_MAX
- Memory management (no leaks on ANY path)
- Performance requirements for 100 and 500 elements

**Your push_swap is 100% bulletproof for 42 evaluations!**

---

*Created by mjabri - The tester that leaves no edge case untested!* 🚀
