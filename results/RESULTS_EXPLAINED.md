# Your results, in plain English

These notes explain the output files in this folder. The files themselves are raw program output and
should not be edited: running `scripts/run_all.sh` regenerates them exactly (same seeds, same numbers).

## 1. The CHSH test  (`chsh_trials.csv`, `chsh_summary.txt`)

We prepared an entangled pair of qubits 100 separate times. Each time we measured it 2,000 times for each
of four angle pairs (8,000 measurements per trial, 800,000 in total) and computed the Bell number S.

* A classical (non-quantum) explanation can never give S above **2**.
* Quantum mechanics allows at most **2.828**.
* We got an average of **2.8315** (spread 0.0350), with every one of the 100 trials above 2.
* Lowest trial: **2.7450 (trial 59)**. Highest trial: **2.9120 (trial 77)**. Individual trials can land slightly above 2.828 by chance; that is noise, not a broken rule.
* The four averaged correlators were 0.7065, 0.7082, 0.7082 and -0.7086. Three positive and one negative is exactly what theory predicts.

**How to read `chsh_trials.csv`:** one row per trial. `seed` is the random seed (re-using it gives the same row),
`E_ab ... E_apbp` are the four correlators, `S` is the Bell number, and `verdict` says ENTANGLED when S is above 2.

**Bottom line:** the simulation reproduces the quantum prediction, and the scatter is the size you would expect from 2,000 shots.
Because we prepared an entangled state on purpose, this shows the code is correct. It does not discover entanglement.

## 2. The BB84 key exchange  (`qkd_session.txt`)

One example session with 100 qubits and no eavesdropper:

* 52 bits survived sifting (Alice and Bob happened to use the same basis).
* 10 of them were sacrificed to check for errors; the error rate (QBER) was 0%.
* 42 bits were left as the raw key.

Important: this is **not** a secure key. Error correction, privacy amplification and authentication are not simulated.

## 3. What happens when someone is listening  (`qkd_summary_eve*.txt`, `qkd_sessions_*.csv`)

1,000 sessions per row. "Abort rate" is how often the error check caught a problem (error estimate above 11%).

| Eavesdropper intercepts | Expected QBER | QBER (100 qubits, 10-bit check) | Abort rate | QBER (1,000 qubits, 100-bit check) | Abort rate |
|---|---|---|---|---|---|
| 0 | 0.00% | 0.0% | 0.0% | 0.0% | 0.0% |
| 0.25 | 6.25% | 6.5% | 13.8% | 6.2% | 2.1% |
| 0.5 | 12.50% | 12.7% | 37.0% | 12.8% | 63.2% |
| 1 | 25.00% | 25.5% | 75.9% | 25.0% | 100.0% |

The take-away: the error rate follows the theory (a quarter of the intercepted fraction), but a tiny 10-bit check
misses a full attack about one time in four. A 100-bit check catches it every time.

## 4. The picture  (`fig_chsh_histogram.png`)

A histogram of the 100 S values. They bunch around 2.83, close to the quantum limit, and the classical limit of 2 is far off to the left.
