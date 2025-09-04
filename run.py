#!/usr/bin/env python3

from os import environ
from os.path import join, isfile
import subprocess

def get_benchmark_output(exec: str, omp_nthreads: int, mkl_nthreads: int) -> float:
    if not isfile(exec):
        print(f"{exec} does not exist...")
        exit(1)
    out = subprocess.run(exec, capture_output=True, env=environ.update({"OMP_NUM_THREADS": str(omp_nthreads), "MKL_NUM_THREAD": str(mkl_nthreads)})).stdout.decode()
    try:
        result = float(out.strip())
    except Exception:
        print(f"Could not convert to float in {exec}")
        exit()
    return result

def run(benchmark: str, reps: int, omp_nthreads: int, mkl_nthreads: int) -> tuple[float, float]:
    print(f"{benchmark}: ", end="")
    ref_exec = join("bin", "ref", "run", benchmark)
    opt_exec = join("bin", "optimized_c", "run", benchmark)
    ref_time = 0.0
    opt_time = 0.0
    for _ in range(reps):
        ref_time += get_benchmark_output(ref_exec, omp_nthreads, mkl_nthreads)
    for _ in range(reps):
        opt_time += get_benchmark_output(opt_exec, omp_nthreads, mkl_nthreads)
    ref_time /= reps
    opt_time /= reps
    diff = ref_time / opt_time
    print(f"{ref_time:.7f} vs. {opt_time:.7f} => x{diff:.2f}")
    return (ref_time, opt_time)

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
        "linear-algebra/solvers/gramschmidt",
        "linear-algebra/solvers/trisolv",
        "medley/deriche",
        "stencils/adi",
        "stencils/fdtd-2d",
        "stencils/heat-3d",
        "stencils/jacobi-1d",
        "stencils/jacobi-2d",
        "stencils/seidel-2d"
    ]
    REPS = 5
    OMP_NTHREADS = 4
    MKL_NTHREADS = 24
    ### BENCHMARKS ###
    from sys import argv
    if len(argv) < 2:
        print("Usage: check.py [benchmark names]")
        exit(1)
    benchmark_names = {bench.split("/")[-1]: bench for bench in BENCHMARKS}
    benchmark_names_args = {argv[i] for i in range(1, len(argv))}
    if "all" in benchmark_names_args:
        benchmark_names_args.remove("all")
        benchmark_names_args.update(benchmark_names.keys())
    if "important" in benchmark_names_args:
        benchmark_names_args.remove("important")
        benchmark_names_args.update(["correlation", "covariance", "gemm", "gemver", "gesummv", "syrk", "2mm", "3mm", "atax", "bicg", "doitgen", "mvt", "gramschmidt", "trisolv"])
    for benchmark_names_arg in benchmark_names_args:
        if not benchmark_names_arg in benchmark_names:
            print(f"Unknown benchmark: {benchmark_names_arg}")
            print(f"Available benchmarks: {list(benchmark_names.keys())}")
            exit(1)
    benchmarks_args = [benchmark_names[bench] for bench in benchmark_names_args]
    time = (0.0, 0.0)
    for bench in benchmarks_args:
        time = tuple(map(sum, zip(run(bench, REPS, OMP_NTHREADS, MKL_NTHREADS), time)))
    if len(benchmarks_args) == 1:
        exit()
    print()
    ref_time = time[0] / len(benchmarks_args)
    opt_time = time[1] / len(benchmarks_args)
    diff = ref_time / opt_time
    print(f"Average: {ref_time:.7f} vs. {opt_time:.7f} => x{diff:.2f}")
