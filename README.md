# MODQuantum: CHSH Bell test and BB84 QKD simulation on Quantum++

C++17 programs that accompany the paper *Simulation-Based Validation of the CHSH Bell Inequality
and BB84 Quantum Key Distribution Using MODQuantum, a Quantum++-Based C++ Toolkit*.
They are built on [Quantum++ (qpp) 6.0](https://github.com/softwareQinc/qpp) (MIT licence, by V. Gheorghiu),
which is bundled under `third_party/` so that the only external dependency you must install is **Eigen 3**.

## Build

Requirements: a C++17 compiler (GCC, Clang, MSVC, MinGW), CMake >= 3.14, Eigen 3 (headers).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release            # add -DEIGEN3_INCLUDE_DIR=/path/to/eigen3 if needed
cmake --build build --config Release
```
Windows (Visual Studio): `cmake -S . -B build` then `cmake --build build --config Release`;
the executable is `build/Release/modquantum.exe`.

## Run

```text
modquantum chsh [--trials 100] [--shots 2000] [--seed 0] [--out results]
modquantum qkd  [--qubits 100] [--sample 10] [--eve 0] [--flip 0] [--threshold 0.11]
                [--sessions 1] [--seed 0] [--out results]
```

* `chsh` prepares |Phi+> = (|00>+|11>)/sqrt(2), then for each of the four setting pairs
  (a=0, a'=90, b=45, b'=-45 degrees; observable O(t) = cos t Z + sin t X) samples `--shots`
  joint outcomes with the Born rule (`qpp::measure` in the joint eigenbasis of O(x) (x) O(y)).
  **`--shots` is per setting pair**, so a trial uses 4 x shots measurements.
  E(x,y) is the mean of the +/-1 products and
  `S = E(a,b) + E(a,b') + E(a',b) - E(a',b')`. Trial *i* re-seeds the generator with `seed + i`.
  Output: `chsh_trials.csv`, `chsh_summary.txt`.
* `qkd` runs BB84: random bits and bases, preparation in |0>,|1>,|+>,|-> (`st.z0, st.z1, st.x0, st.x1`),
  optional intercept-resend eavesdropper (`--eve` = fraction of qubits intercepted) and bit-flip channel
  noise (`--flip`; note an X error only affects Z-basis qubits), Bob's random-basis measurement,
  sifting, QBER estimation on `--sample` random sifted bits, abort if QBER > `--threshold`.
  One session prints the per-qubit table; `--sessions M` runs M sessions and writes a CSV plus a summary.
  **No error correction, privacy amplification or authentication is simulated: the output is a raw sifted
  key, not a secure key.**

## Reproduce the paper's results

```bash
./scripts/run_all.sh        # needs python3 (+ matplotlib for the figure)
```
Everything is deterministic given the seeds: running it twice gives byte-identical CSV files.
The files in `results/` were produced by exactly this script.

## Layout
`src/` and `include/` our code; `third_party/qpp` Quantum++ headers and licence;
`third_party/qasmtools` header-only dependency of Quantum++ (QASM parser); `scripts/` run + report; `results/` outputs.

## Licence
Our code: MIT (add your name in `LICENSE`). Quantum++ and qasmtools keep their own licences in `third_party/`.
