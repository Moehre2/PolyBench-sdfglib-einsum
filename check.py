#!/usr/bin/env python3

from os.path import join, isfile
import subprocess
import re

def get_benchmark_output(exec: str, type: str, nthreads: int = 1) -> tuple[bool, dict[str, list[float]]]:
    if not isfile(exec):
        print(f"{exec} does not exist...")
        return False, {}
    out = subprocess.run(exec, capture_output=True, env={"OMP_NUM_THREADS": str(nthreads)}).stderr.decode()
    dump_region = re.search("(?<===BEGIN DUMP_ARRAYS==\n)(?s:.)*(?===END   DUMP_ARRAYS==)", out)
    if dump_region == None:
        print(f"Cannot find DUMP_ARRAYS region in {type}...")
        return False, {}
    dumps = re.split("begin dump: ", dump_region.group(0))
    result = {}
    for dump in dumps:
        if dump == "":
            continue
        dump_split_raw = dump.split()
        key = dump_split_raw[0]
        if key != dump_split_raw[-1]:
            print(f"Variable at beginning and end of dump do not match ({key} != {dump_split_raw[-1]})")
            return False, {}
        dump_split = [float(val) for val in dump_split_raw[1:-3] if val != ""]
        result[key] = dump_split
    return True, result

def check(benchmark: str, nthreads: int) -> bool:
    print(f"{benchmark}: ", end="")
    ref_out_res, ref_out = get_benchmark_output(join("bin", "ref", "check", benchmark), "ref")
    if not ref_out_res:
        return False
    opt_out_res, opt_out = get_benchmark_output(join("bin", "optimized_c", "check", benchmark), "opt")
    if not opt_out_res:
        return False
    opt2_out_res, opt2_out = get_benchmark_output(join("bin", "optimized_c", "check", benchmark), "opt", nthreads=nthreads)
    if not opt2_out_res:
        return False
    key_differences = set(ref_out.keys()).symmetric_difference(set(opt_out.keys()))
    if len(key_differences) > 0:
        print(f"Variable {key_differences.pop()} does not occur in both outputs...")
        return False
    key_differences2 = set(ref_out.keys()).symmetric_difference(set(opt2_out.keys()))
    if len(key_differences2) > 0:
        print(f"Variable {key_differences2.pop()} does not occur in both outputs...")
        return False
    for key in ref_out:
        if len(ref_out[key]) != len(opt_out[key]):
            print(f"{key}: Different output lengths...")
        for i in range(len(ref_out[key])):
            if abs(ref_out[key][i] - opt_out[key][i]) > 0.011:
                print(f"{key}: Values do not match at {i} ({ref_out[key][i]} != {opt_out[key][i]})...")
                return False
    stable = True
    for key in ref_out:
        if len(ref_out[key]) != len(opt2_out[key]):
            print(f"{key}: Different output lengths...")
        for i in range(len(ref_out[key])):
            if abs(ref_out[key][i] - opt2_out[key][i]) > 0.011:
                stable = False
                break
        if not stable:
            break
    if stable:
        print("Matches!")
    else:
        print("INSTABLE!")
    return True

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
    NTHREADS=12
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
    all_match = []
    for bench in benchmarks_args:
        all_match.append(check(bench, NTHREADS))
    if False in all_match:
        exit(1)
