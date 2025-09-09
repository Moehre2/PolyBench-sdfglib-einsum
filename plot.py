#!/usr/bin/env python3

import json
from os.path import isfile
from statistics import mean
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

BENCHMARKS = {
    "datamining": {
        "benchmarks": ["correlation", "covariance"],
        "nrows": 1,
        "ncols": 2
    },
    "linear-algebra/blas": {
        "benchmarks": ["gemm", "gemver", "gesummv", "symm", "syr2k", "syrk", "trmm"],
        "nrows": 2,
        "ncols": 4
    },
    "linear-algebra/kernels": {
        "benchmarks": ["2mm", "3mm", "atax", "bicg", "doitgen", "mvt"],
        "nrows": 2,
        "ncols": 3
    },
    "linear-algebra/solvers": {
        "benchmarks": ["cholesky", "durbin", "gramschmidt", "lu", "ludcmp", "trisolv"],
        "nrows": 2,
        "ncols": 3
    },
    "medley": {
        "benchmarks": ["deriche", "floyd-warshall", "nussinov"],
        "nrows": 1,
        "ncols": 3
    },
    "stencils": {
        "benchmarks": ["adi", "fdtd-2d", "heat-3d", "jacobi-1d", "jacobi-2d", "seidel-2d"],
        "nrows": 2,
        "ncols": 3
    }
}
WIDTH=6.4
HEIGHT=4.8

results = {}

def plot_benchmark(versions: dict[str, dict[str, str]], ref: str, group: str, benchmark: str, ax = None) -> None:
    if ax is None:
        ax = plt.gca()
    bench_path = f"{group}/{benchmark}"
    x = []
    y = []
    colors = []
    edgecolors = []
    hatches = []
    x.append(versions[ref]["name"])
    y.append(1.0)
    ref_time = mean(results[ref][bench_path]["data"])
    colors.append(versions[ref]["color1"])
    edgecolors.append(versions[ref]["color1"])
    hatches.append("")
    for version, version_data in versions.items():
        if version == ref:
            continue
        x.append(version_data["name"])
        if results[version][bench_path]["status"] in ["good", "unstable"]:
            y.append(mean(results[version][bench_path]["data"]) / ref_time)
            if results[version][bench_path]["status"] == "good":
                colors.append(version_data["color1"])
                edgecolors.append(version_data["color1"])
                hatches.append("")
            else:
                colors.append("#ffffff")
                edgecolors.append(version_data["color2"])
                hatches.append("/")
        else:
            y.append(0.0)
            colors.append("#ffffff")
            edgecolors.append("#ffffff")
            hatches.append("")
    ax.grid(axis="y", zorder=0)
    nodata_y = max(y) * 0.1
    for bar in ax.bar(x, y, color=colors, edgecolor=edgecolors, hatch=hatches, zorder=3):
        if bar.get_height() == 0.0:
            ax.text(bar.get_x() + 0.4, nodata_y, "x", fontsize="xx-large", fontweight="bold", horizontalalignment="center", zorder=4)
    ax.set(ylabel="Normalized runtime", title=benchmark)

def plot_group(versions: dict[str, dict[str, str]], ref: str, group: str) -> None:
    nrows = BENCHMARKS[group]["nrows"]
    ncols = BENCHMARKS[group]["ncols"]
    fig, axs = plt.subplots(nrows=nrows, ncols=ncols, squeeze=False, figsize=(WIDTH * ncols, HEIGHT * nrows), height_ratios=[HEIGHT] * nrows)
    bench = 0
    for row in range(nrows):
        for col in range(ncols):
            if bench >= len(BENCHMARKS[group]["benchmarks"]):
                fig.delaxes(axs[row][col])
            else:
                plot_benchmark(versions, ref, group, BENCHMARKS[group]["benchmarks"][bench], axs[row][col]) # type: ignore
                bench += 1
    #fig.suptitle(group, fontsize="x-large", fontweight="demi")
    fig.subplots_adjust(bottom=(0.15 / nrows))
    patches = []
    patches.append(mpatches.Patch(color=versions[ref]["color1"], label=versions[ref]["name"]))
    for version, version_data in versions.items():
        if version != ref:
            patches.append(mpatches.Patch(color=version_data["color1"], label=version_data["name"]))
    fig.legend(handles=patches, loc="lower center", ncols=len(patches))
    outfile_png = "plots/" + group.replace("/", "_") + ".png"
    outfile_pdf = "plots/" + group.replace("/", "_") + ".pdf"
    fig.savefig(outfile_png)
    fig.savefig(outfile_pdf, bbox_inches="tight")
    plt.close(fig)

def compute_results(versions: dict[str, dict[str, str]], ref: str) -> None:
    if not ref in versions:
        print(f"{ref} not in versions")
        exit(1)
    for version in versions.keys():
        file_path = f"results/{version}.json"
        if not isfile(file_path):
            print(f"File does not exist: {file_path}")
            exit(1)
        data = None
        with open(file_path, "r") as file:
            data = json.load(file)
        if data == None:
            print(f"Could not load JSON from file: {file_path}")
            exit(1)
        results[version] = data

def plot_all(versions: dict[str, dict[str, str]], ref: str) -> None:
    compute_results(versions, ref)
    for group in BENCHMARKS.keys():
        plot_group(versions, ref, group)

def plot_all_single(versions: dict[str, dict[str, str]], ref: str) -> None:
    compute_results(versions, ref)
    for group, group_data in BENCHMARKS.items():
        for bench in group_data["benchmarks"]:
            fig, ax = plt.subplots(figsize=(WIDTH, HEIGHT))
            plot_benchmark(versions, ref, group, bench, ax)
            outfile_png = "plots/single/" + group.replace("/", "_") + "_" + bench + ".png"
            outfile_pdf = "plots/single/" + group.replace("/", "_") + "_" + bench + ".pdf"
            fig.savefig(outfile_png)
            fig.savefig(outfile_pdf, bbox_inches="tight")
            plt.close(fig)

if __name__ == "__main__":
    VERSIONS = {
        "ref": {
            "name": "ref",
            "color1": "#7a7a7a",
            "color2": "#a8a8a8"
        },
        "opt_c": {
            "name": "optimized (C)",
            "color1": "#cc1c2f",
            "color2": "#ff7078"
        },
        "intel": {
            "name": "intel",
            "color1": "#0072b4",
            "color2": "#79abe2"
        },
        "polly": {
            "name": "polly",
            "color1": "#f17b51",
            "color2": "#f6a173"
        },
        "pluto": {
            "name": "pluto",
            "color1": "#6d60bb",
            "color2": "#a79fe1"
        }
    }
    REF = "ref"
    from sys import argv
    from os import path, makedirs
    mode = None
    if len(argv) == 1:
        mode = "thesis"
    elif len(argv) != 2:
        print("Usage: plot.py [thesis|presentation]")
        exit(1)
    else:
        mode = argv[1]
    if not path.exists("plots/"):
        makedirs("plots")
    if mode == "thesis":
        plot_all(VERSIONS, REF)
    elif mode == "presentation":
        if not path.exists("plots/single/"):
            makedirs("plots/single/")
        plot_all_single(VERSIONS, REF)
    else:
        print(f"Unknown mode: {mode}")
        exit(1)