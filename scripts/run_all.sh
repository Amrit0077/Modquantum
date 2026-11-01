#!/usr/bin/env bash
# Reproduces every result reported in the paper. Run from the repository root after building.
set -e
EXE=${EXE:-./build/modquantum}
OUT=${OUT:-results}
mkdir -p "$OUT"
# 1. CHSH: 100 trials x 4 setting pairs x 2000 shots, seeds 0..99
$EXE chsh --trials 100 --shots 2000 --seed 0 --out "$OUT" > "$OUT/chsh_console.txt"
# 2. BB84: one illustrative session (100 qubits, 10-bit QBER sample, seed 0)
$EXE qkd --qubits 100 --sample 10 --seed 0 --out "$OUT" > "$OUT/qkd_console.txt"
# 3. BB84 ensembles: intercept-resend with probability 0, 0.25, 0.5, 1 (1000 sessions each, seeds 1000..1999)
for cfg in "100 10" "1000 100"; do
  set -- $cfg
  for eve in 0 0.25 0.5 1; do
    $EXE qkd --qubits $1 --sample $2 --eve $eve --sessions 1000 --seed 1000 --out "$OUT" > /dev/null
  done
done
# 4. Figures and tables
python3 scripts/make_report.py "$OUT"
