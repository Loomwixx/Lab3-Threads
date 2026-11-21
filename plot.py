#!/usr/bin/env python3
"""
Plots for grade 4.0: computation time T(t) and speedup S(t).

Usage:   python3 plot.py results_d6.csv results_d7.csv results_d8.csv
Input:   CSV files printed by  ./pincrack --bench <d> <tmax>  (header: t,median_s,speedup)
Output:  time.png, speedup.png
"""
import csv
import re
import sys

import matplotlib
matplotlib.use("Agg")  # draw straight to files, no window needed
import matplotlib.pyplot as plt

P_CORES = 4  # Apple M3: 4 performance cores
ALL_CORES = 8  # + 4 efficiency cores

if len(sys.argv) < 2:
    sys.exit("usage: python3 plot.py results_d6.csv [results_d7.csv ...]")

series = []  # (label, [t], [time], [speedup])
for path in sys.argv[1:]:
    ts, times, speedups = [], [], []
    with open(path) as f:
        for row in csv.DictReader(f):
            ts.append(int(row["t"]))
            times.append(float(row["median_s"]))
            speedups.append(float(row["speedup"]))
    m = re.search(r"d(\d+)", path)
    label = f"d = {m.group(1)}" if m else path
    series.append((label, ts, times, speedups))

tmax = max(max(s[1]) for s in series)
markers = "osd^v"


def mark_cores(ax):
    for x, text in ((P_CORES, "4 P-cores"), (ALL_CORES, "8 cores")):
        ax.axvline(x, color="gray", linestyle=":", linewidth=1)
        ax.text(x + 0.1, 0.97, text, transform=ax.get_xaxis_transform(),
                va="top", fontsize=8, color="gray")


# --- T(t) ---
fig, ax = plt.subplots(figsize=(7, 4.5))
for i, (label, ts, times, _) in enumerate(series):
    ax.plot(ts, times, marker=markers[i % len(markers)], label=label)
ax.set_yscale("log")  # d differ by 10x in work, log scale keeps all lines readable
ax.set_xlabel("number of threads t")
ax.set_ylabel("computation time T(t) [s] (median of 5, log scale)")
ax.set_title("PIN search: time vs number of threads")
ax.set_xticks(range(1, tmax + 1))
ax.grid(alpha=0.3, which="both")
mark_cores(ax)
ax.legend()
fig.tight_layout()
fig.savefig("time.png", dpi=150)

# --- S(t) ---
fig, ax = plt.subplots(figsize=(7, 4.5))
ax.plot([1, tmax], [1, tmax], "k--", label="ideal S(t) = t")
for i, (label, ts, _, speedups) in enumerate(series):
    ax.plot(ts, speedups, marker=markers[i % len(markers)], label=label)
ax.set_xlabel("number of threads t")
ax.set_ylabel("speedup S(t) = T(1) / T(t)")
ax.set_title("PIN search: speedup vs number of threads")
ax.set_xticks(range(1, tmax + 1))
ax.set_ylim(bottom=0)
ax.grid(alpha=0.3)
mark_cores(ax)
ax.legend()
fig.tight_layout()
fig.savefig("speedup.png", dpi=150)

print("saved time.png and speedup.png")