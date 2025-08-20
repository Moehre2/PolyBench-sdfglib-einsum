#!/usr/bin/env python3

from os.path import join, isfile
import subprocess

def get_benchmark_output(exec: str) -> float:
    if not isfile(exec):
        print(f"{exec} does not exist...")
        exit(1)
    out = subprocess.run(exec, capture_output=True).stdout.decode()
    return float(out.strip())

def run(benchmark: str, reps: int) -> None:
    print(f"{benchmark}: ", end="")
    ref_exec = join("bin", "run", "ref", benchmark)
    opt_exec = join("bin", "run", "optimized_c", benchmark)
    ref_time = 0.0
    opt_time = 0.0
    for _ in range(reps):
        ref_time += get_benchmark_output(ref_exec)
    for _ in range(reps):
        opt_time += get_benchmark_output(opt_exec)
    ref_time /= reps
    opt_time /= reps
    diff = ref_time / opt_time
    print(f"{ref_time:.7f} vs. {opt_time:.7f} => x{diff:.2f}")

if __name__ == "__main__":
    ### BENCHMARKS ###
    BENCHMARKS = [
        "datamining/correlation",
        "datamining/covariance",
        "linear-algebra/blas/gemm",
        "linear-algebra/blas/gemver",
        "linear-algebra/blas/gesummv",
        "linear-algebra/blas/symm",
        "linear-algebra/blas/syr2k",
        "linear-algebra/blas/syrk",
        "linear-algebra/blas/trmm",
        "linear-algebra/kernels/2mm",
        "linear-algebra/kernels/3mm",
        "linear-algebra/kernels/atax",
        "linear-algebra/kernels/bicg",
        "linear-algebra/kernels/doitgen",
        "linear-algebra/kernels/mvt",
        "stencils/heat-3d"
    ]
    REPS = 40
    ### BENCHMARKS ###
    from sys import argv
    if len(argv) < 2:
        print("Usage: check.py [benchmark names]")
        exit(1)
    benchmark_names = {bench.split("/")[-1]: bench for bench in BENCHMARKS}
    benchmark_names_args = [argv[i] for i in range(1, len(argv))]
    if "all" in benchmark_names_args:
        benchmark_names_args = list(benchmark_names.keys())
    for benchmark_names_arg in benchmark_names_args:
        if not benchmark_names_arg in benchmark_names:
            print(f"Unknown benchmark: {benchmark_names_arg}")
            print(f"Available benchmarks: {list(benchmark_names.keys())}")
            exit(1)
    benchmarks_args = [benchmark_names[bench] for bench in benchmark_names_args]
    for bench in benchmarks_args:
        run(bench, REPS)
