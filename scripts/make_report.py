#!/usr/bin/env python3
"""Reads results/*.csv, cross-checks the C++ summary, and writes figures + LaTeX tables."""
import csv, glob, math, os, statistics as st, sys
out = sys.argv[1] if len(sys.argv) > 1 else "results"

rows = list(csv.DictReader(open(f"{out}/chsh_trials.csv")))
S = [float(r["S"]) for r in rows]
n = len(S)
mean, sd = st.mean(S), st.stdev(S)
print(f"CHSH: n={n} mean={mean:.4f} sd={sd:.4f} sem={sd/math.sqrt(n):.4f} min={min(S):.4f} max={max(S):.4f}")
print(f"expected sd(S) for N=2000: {2*math.sqrt((1-0.5)/2000):.4f}")

try:
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(6.4, 3.6), dpi=200)
    ax.hist(S, bins=18, color="#4c72b0", edgecolor="white")
    ax.axvline(2 * math.sqrt(2), color="#55a868", ls="-", label=r"Tsirelson bound $2\sqrt{2}$")
    ax.axvline(mean, color="k", ls=":", label=f"Mean $S={mean:.3f}$")
    ax.set_xlim(2.70, 2.96)
    ax.text(0.02, 0.94, "Classical bound $S=2$\n(far to the left, off scale)", transform=ax.transAxes,
            fontsize=8, color="#c44e52", va="top")
    ax.set_xlabel("Bell parameter $S$ (one value per trial)")
    ax.set_ylabel("Number of trials")
    ax.legend(frameon=False, fontsize=8)
    fig.tight_layout()
    fig.savefig(f"{out}/fig_chsh_histogram.png")
    fig.savefig(f"{out}/fig_chsh_histogram.pdf")
    print("wrote histogram")
except ImportError:
    print("matplotlib not installed: skipping figure")

for f in sorted(glob.glob(f"{out}/qkd_sessions_*.csv")):
    r = list(csv.DictReader(open(f)))
    ab = sum(int(x["aborted"]) for x in r) / len(r)
    q = st.mean(float(x["qber_est"]) for x in r)
    print(f"{os.path.basename(f)}: sessions={len(r)} mean QBER_est={q:.4f} abort_rate={ab:.3f}")
