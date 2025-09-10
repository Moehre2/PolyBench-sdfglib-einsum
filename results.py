#!/usr/bin/env python3

from typing import Any
from os.path import isfile
import json
from statistics import mean
from math import isnan

def get_data(version: str) -> dict[str, dict[str, Any]]:
    filepath = f"results/{version}.json"
    if not isfile(filepath):
        print(f"File does not exist: {filepath}")
        exit(1)
    data = None
    with open(filepath, "r") as file:
        data = json.load(file)
    if data == None:
        print(f"Could not load JSON from file: {filepath}")
        exit(1)
    return dict(sorted(data.items()))

def calculate_data(raw_data: dict[str, dict[str, Any]], raw_data_ref: dict[str, dict[str, Any]]) -> dict[str, dict[str, Any]]:
    result = {}
    for benchmark in raw_data_ref:
        bench = benchmark.split("/")[-1]
        result[bench] = {}
        status = raw_data[benchmark]["status"]
        result[bench]["status"] = status
        if status in ["good", "unstable"]:
            avg_ref = mean(raw_data_ref[benchmark]["data"])
            result[bench]["avg"] = avg_ref / mean(raw_data[benchmark]["data"])
            result[bench]["min"] = avg_ref / min(raw_data[benchmark]["data"])
            result[bench]["max"] = avg_ref / max(raw_data[benchmark]["data"])
        else:
            result[bench]["avg"] = float("nan")
            result[bench]["min"] = float("nan")
            result[bench]["max"] = float("nan")
    return result

def print_results(version: str) -> None:
    raw_data_ref = get_data("ref")
    raw_data = get_data(version)
    data = calculate_data(raw_data, raw_data_ref)
    max_name_len = max(4, max([len(bench) for bench in data.keys()]))
    max_status_len = max(6, max([len(bench_data["status"]) for bench_data in data.values()]))
    max_avg_len = max(3, max([len(f"x{bench_data['avg']:.2f}") for bench_data in data.values()]))
    max_min_len = max(3, max([len(f"x{bench_data['min']:.2f}") for bench_data in data.values()]))
    max_max_len = max(3, max([len(f"x{bench_data['max']:.2f}") for bench_data in data.values()]))
    print(f"{'*' * (max_name_len + max_status_len + max_avg_len + max_min_len + max_max_len + 16)}")
    print(f"* Name{' ' * (max_name_len - 4)} * Status{' ' * (max_status_len - 6)} * {' ' * (max_avg_len - 3)}Avg * {' ' * (max_min_len - 3)}Min * {' ' * (max_max_len - 3)}Max *")
    print(f"{'*' * (max_name_len + max_status_len + max_avg_len + max_min_len + max_max_len + 16)}")
    for bench, bench_data in data.items():
        avg_str = "-" if isnan(bench_data["avg"]) else f"x{bench_data['avg']:.2f}"
        min_str = "-" if isnan(bench_data["min"]) else f"x{bench_data['min']:.2f}"
        max_str = "-" if isnan(bench_data["max"]) else f"x{bench_data['max']:.2f}"
        print(f"* {bench}{' ' * (max_name_len - len(bench))} * {bench_data['status']}{' ' * (max_status_len - len(bench_data['status']))} * {' ' * (max_avg_len - len(avg_str))}{avg_str} * {' ' * (max_min_len - len(min_str))}{min_str} * {' ' * (max_max_len - len(max_str))}{max_str} *")
    print(f"{'*' * (max_name_len + max_status_len + max_avg_len + max_min_len + max_max_len + 16)}")

if __name__ == "__main__":
    from sys import argv
    if len(argv) != 2:
        print("Usage: results.py [json file]")
        exit(1)
    print_results(argv[1])