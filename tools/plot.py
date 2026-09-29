"""Plot swiss_bench results.

Usage: python tools/plot.py RESULTS.json OUTPUT_DIR

Reads Google Benchmark JSON output. When a run has a median aggregate row
(from --benchmark_repetitions), that row is used. Otherwise the median of the
run's iteration rows is used. Writes one PNG per plot and summary.csv, a table
of every plotted value.
"""

import argparse
import csv
import json
import math
import statistics
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter, LogLocator, NullLocator

MAPS = ["V1", "V2", "V3", "Std", "Boost"]

MAP_STYLE = {
    "V1": {"color": "#2a78d6", "marker": "o", "label": "V1 linear probing"},
    "V2": {"color": "#eb6834", "marker": "s", "label": "V2 scalar Swiss table"},
    "V3": {"color": "#1baf7a", "marker": "D", "label": "V3 SSE2 Swiss table"},
    "Std": {"color": "#eda100", "marker": "^", "label": "std::unordered_map"},
    "Boost": {"color": "#e87ba4", "marker": "v", "label": "boost::unordered_flat_map"},
}

SURFACE = "#fcfcfb"
TEXT_PRIMARY = "#0b0b0b"
TEXT_SECONDARY = "#52514e"
HAIRLINE = "#e4e3de"

CACHE_THRESHOLDS = [(32 * 1024, "32 KB"), (512 * 1024, "512 KB"), (16 * 1024 * 1024, "16 MB")]
V3_INT_SLOT_BYTES = 9
DEFAULT_PAIR_STRING_INT_BYTES = 40
E1_TARGET_LOAD = 0.75


def configure_style():
    plt.rcParams.update({
        "figure.facecolor": SURFACE,
        "axes.facecolor": SURFACE,
        "savefig.facecolor": SURFACE,
        "text.color": TEXT_PRIMARY,
        "axes.labelcolor": TEXT_PRIMARY,
        "axes.titlecolor": TEXT_PRIMARY,
        "axes.edgecolor": HAIRLINE,
        "axes.linewidth": 0.8,
        "axes.spines.top": False,
        "axes.spines.right": False,
        "axes.grid": True,
        "grid.color": HAIRLINE,
        "grid.linestyle": "-",
        "grid.linewidth": 0.6,
        "xtick.color": TEXT_SECONDARY,
        "ytick.color": TEXT_SECONDARY,
        "xtick.labelcolor": TEXT_SECONDARY,
        "ytick.labelcolor": TEXT_SECONDARY,
        "legend.frameon": False,
        "legend.labelcolor": TEXT_PRIMARY,
        "font.size": 10,
        "axes.titlesize": 12,
        "axes.titleweight": "bold",
        "axes.titlelocation": "left",
    })


class Run:
    def __init__(self, run_name):
        parts = run_name.split("/")
        self.run_name = run_name
        self.experiment, self.operation, self.map, self.key, self.parameter = parts[:5]
        self.iterations = []
        self.median = None

    def value(self, field):
        if self.median is not None:
            return self.median.get(field)

        values = [row[field] for row in self.iterations if field in row]
        return statistics.median(values) if values else None

    def ns_per_op(self):
        rate = self.value("items_per_second")
        return 1e9 / rate if rate else None


def load_runs(path):
    with open(path, encoding="utf-8") as file:
        data = json.load(file)

    runs = {}
    for row in data.get("benchmarks", []):
        if row.get("error_occurred") or row.get("skipped"):
            print(f"skipping {row['name']}: {row.get('error_message', 'skipped')}", file=sys.stderr)
            continue

        run_name = row.get("run_name", row["name"])
        if len(run_name.split("/")) < 5:
            continue

        run = runs.setdefault(run_name, Run(run_name))
        if row.get("run_type") == "aggregate":
            if row.get("aggregate_name") == "median":
                run.median = row
        else:
            run.iterations.append(row)

    return list(runs.values()), data.get("context", {})


def select(runs, experiment, operation=None, key=None):
    table = defaultdict(list)
    for run in runs:
        if run.experiment != experiment:
            continue
        if operation is not None and run.operation != operation:
            continue
        if key is not None and run.key != key:
            continue
        table[run.map].append(run)

    return table


def coincide(a, b):
    if [x for x, _ in a] != [x for x, _ in b]:
        return False

    return all(math.isclose(ya, yb, rel_tol=0.01) for (_, ya), (_, yb) in zip(a, b))


def draw_maps(axes, series, approximate=()):
    """Draw one line per map in fixed map order.

    Maps whose values coincide are drawn nested: the earlier map wider and
    underneath, so every colour stays visible and the legend names the match.
    """
    cleaned = {}
    for map_name in MAPS:
        points = sorted((x, y) for x, y in series.get(map_name, []) if x is not None and y is not None)
        if points:
            cleaned[map_name] = points

    groups = []
    for map_name, points in cleaned.items():
        for group in groups:
            if coincide(cleaned[group[0]], points):
                group.append(map_name)
                break
        else:
            groups.append([map_name])

    for group in groups:
        for depth, map_name in enumerate(group):
            outer = len(group) - 1 - depth
            style = MAP_STYLE[map_name]
            hollow = map_name in approximate
            label = style["label"]
            if hollow:
                label += " (measured load, approximate)"
            if depth > 0:
                label += f" (same as {group[0]})"

            xs, ys = zip(*cleaned[map_name])
            axes.plot(
                xs, ys,
                color=style["color"],
                linewidth=2 + 2.5 * outer,
                marker=style["marker"],
                markersize=7 + 4 * outer,
                markerfacecolor=SURFACE if hollow else style["color"],
                markeredgecolor=style["color"] if hollow else SURFACE,
                markeredgewidth=1.8 if hollow else 1.2,
                label=label,
                gid=map_name,
                zorder=3 + depth,
            )


def zero_based_y(axes):
    axes.set_ylim(0, axes.dataLim.y1 * 1.08)


def power_of_two_axis(axis):
    axis.set_major_locator(LogLocator(base=2, numticks=30))
    axis.set_minor_locator(NullLocator())
    axis.set_major_formatter(FuncFormatter(lambda value, _: f"$2^{{{round(math.log2(value))}}}$" if value > 0 else ""))


def finish(figure, axes, path, written):
    handles, labels = axes.get_legend_handles_labels()
    order = sorted(range(len(handles)), key=lambda i: MAPS.index(handles[i].get_gid()))
    axes.legend(
        [handles[i] for i in order], [labels[i] for i in order],
        loc="upper left", bbox_to_anchor=(1.02, 1), fontsize=9,
    )
    figure.savefig(path, dpi=150, bbox_inches="tight")
    plt.close(figure)
    written.append(path)


def v3_crossings(slot_bytes):
    """Element counts at which a V3 table sized as bit_ceil(n / 0.75) first exceeds each threshold.

    capacity(n) >= C exactly when n > C * 0.75 / 2, so the table reaches the
    first capacity C with C * slot_bytes > threshold at n = 0.375 * C.
    """
    crossings = []
    for threshold, label in CACHE_THRESHOLDS:
        capacity = 1
        while capacity * slot_bytes <= threshold:
            capacity *= 2

        crossings.append((capacity * E1_TARGET_LOAD / 2, label))

    return crossings


def draw_crossings(axes, crossings, x_low, x_high):
    for x, label in crossings:
        if not x_low <= x <= x_high:
            continue

        axes.axvline(x, color=TEXT_SECONDARY, linestyle=(0, (4, 3)), linewidth=1, zorder=1)
        axes.annotate(
            label, xy=(x, 1), xycoords=("data", "axes fraction"),
            xytext=(3, -3), textcoords="offset points",
            ha="left", va="top", fontsize=8, color=TEXT_SECONDARY,
        )


def plot_e1(runs, context, out_dir, written):
    pair_bytes = int(context.get("pair_string_int_bytes", DEFAULT_PAIR_STRING_INT_BYTES))
    slot_bytes = {"int": V3_INT_SLOT_BYTES, "string": pair_bytes + 1}
    names = {"LookupHit": "successful lookup", "LookupMiss": "unsuccessful lookup", "Insert": "insert into a presized table"}
    for operation, description in names.items():
        for key in ("int", "string"):
            table = select(runs, "E1", operation, key)
            if not table:
                continue

            figure, axes = plt.subplots(figsize=(8, 5))
            series = {m: [(int(run.parameter), run.ns_per_op()) for run in rows] for m, rows in table.items()}
            xs = [x for points in series.values() for x, _ in points]
            draw_maps(axes, series)

            axes.set_xscale("log", base=2)
            power_of_two_axis(axes.xaxis)
            zero_based_y(axes)
            draw_crossings(axes, v3_crossings(slot_bytes[key]), min(xs), max(xs))
            axes.set_title(f"E1: {description}, {key} keys")
            axes.set_xlabel("Elements in the table, n (log scale)")
            axes.set_ylabel("Time per operation (ns)")
            axes.text(
                0, -0.16, f"Dashed lines: a V3-style table ({slot_bytes[key]} B per slot, capacity bit_ceil(n / 0.75)) grows past each size.",
                transform=axes.transAxes, fontsize=8, color=TEXT_SECONDARY,
            )
            finish(figure, axes, out_dir / f"e1_{operation.lower()}_{key}.png", written)


def plot_e2(runs, out_dir, written):
    for operation, description in (("LookupHit", "successful lookup"), ("LookupMiss", "unsuccessful lookup")):
        table = select(runs, "E2", operation, "int")
        if not table:
            continue

        figure, axes = plt.subplots(figsize=(8, 5))
        series = {m: [(run.value("load"), run.ns_per_op()) for run in rows] for m, rows in table.items()}
        draw_maps(axes, series, approximate=("Boost",))

        zero_based_y(axes)
        axes.set_title(f"E2: {description} against load factor, int keys")
        axes.set_xlabel("Load factor (live elements / slots)")
        axes.set_ylabel("Time per operation (ns)")
        finish(figure, axes, out_dir / f"e2_{operation.lower()}.png", written)


def plot_e3(runs, out_dir, written):
    table = select(runs, "E3", "Churn", "int")
    if not table:
        return

    figure, axes = plt.subplots(figsize=(8, 5))
    draw_maps(axes, {m: [(int(run.parameter), run.ns_per_op()) for run in rows] for m, rows in table.items()})
    axes.set_xscale("log", base=2)
    power_of_two_axis(axes.xaxis)
    zero_based_y(axes)
    axes.set_title("E3: churn with $2^{16}$ live int keys")
    axes.set_xlabel("Erase-plus-insert rounds (log scale)")
    axes.set_ylabel("Time per erase plus insert (ns)")
    finish(figure, axes, out_dir / "e3_churn_time.png", written)

    figure, axes = plt.subplots(figsize=(8, 5))
    draw_maps(axes, {m: [(int(run.parameter), run.value("capacity")) for run in rows] for m, rows in table.items()})
    axes.set_xscale("log", base=2)
    axes.set_yscale("log", base=2)
    power_of_two_axis(axes.xaxis)
    power_of_two_axis(axes.yaxis)
    axes.axhline(2 ** 16, color=TEXT_SECONDARY, linewidth=1, zorder=1)
    axes.annotate(
        "live keys: $2^{16}$", xy=(1, 2 ** 16), xycoords=("axes fraction", "data"),
        xytext=(-3, -4), textcoords="offset points", ha="right", va="top", fontsize=8, color=TEXT_SECONDARY,
    )
    axes.set_title("E3: final table capacity after churn")
    axes.set_xlabel("Erase-plus-insert rounds (log scale)")
    axes.set_ylabel("Final capacity (slots or buckets, log scale)")
    finish(figure, axes, out_dir / "e3_capacity.png", written)


def plot_e4(runs, out_dir, written):
    table = select(runs, "E4", "Iterate", "int")
    if not table:
        return

    figure, axes = plt.subplots(figsize=(8, 5))
    draw_maps(axes, {m: [(int(run.parameter), run.ns_per_op()) for run in rows] for m, rows in table.items()})
    axes.set_xscale("log", base=2)
    power_of_two_axis(axes.xaxis)
    zero_based_y(axes)
    axes.set_title("E4: full iteration summing values, int keys")
    axes.set_xlabel("Elements in the table, n (log scale)")
    axes.set_ylabel("Time per element (ns)")
    finish(figure, axes, out_dir / "e4_iterate.png", written)


def plot_e5(runs, out_dir, written):
    for key in ("int", "string"):
        table = select(runs, "E5", "Memory", key)
        if not table:
            continue

        figure, axes = plt.subplots(figsize=(8, 5))
        draw_maps(axes, {m: [(int(run.parameter), run.value("bytes_per_element")) for run in rows] for m, rows in table.items()})
        axes.set_xscale("log", base=2)
        power_of_two_axis(axes.xaxis)
        zero_based_y(axes)
        axes.set_title(f"E5: heap bytes per element, {key} keys")
        axes.set_xlabel("Elements in the table, n (log scale)")
        axes.set_ylabel("Requested heap bytes per element (B)")
        finish(figure, axes, out_dir / f"e5_memory_{key}.png", written)


def write_summary(runs, path, written):
    fields = ["experiment", "operation", "map", "key", "parameter", "ns_per_op", "load", "capacity", "bytes_per_element"]
    with open(path, "w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(fields)
        for run in sorted(runs, key=lambda r: (r.experiment, r.operation, r.key, MAPS.index(r.map) if r.map in MAPS else 99, float(r.parameter))):
            writer.writerow([
                run.experiment, run.operation, run.map, run.key, run.parameter,
                run.ns_per_op(), run.value("load"), run.value("capacity"), run.value("bytes_per_element"),
            ])

    written.append(path)


def main():
    parser = argparse.ArgumentParser(description="Plot swiss_bench Google Benchmark JSON output.")
    parser.add_argument("results", type=Path, help="JSON file written with --benchmark_out")
    parser.add_argument("output_dir", type=Path, help="directory for the PNG files")
    args = parser.parse_args()

    runs, context = load_runs(args.results)
    if not runs:
        sys.exit(f"no usable benchmark rows in {args.results}")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    configure_style()
    written = []
    plot_e1(runs, context, args.output_dir, written)
    plot_e2(runs, args.output_dir, written)
    plot_e3(runs, args.output_dir, written)
    plot_e4(runs, args.output_dir, written)
    plot_e5(runs, args.output_dir, written)
    write_summary(runs, args.output_dir / "summary.csv", written)
    for path in written:
        print(path)


if __name__ == "__main__":
    main()
