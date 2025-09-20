#!/usr/bin/env python3

from os import path, environ
import subprocess
import re
from math import isnan
from sys import stdout

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
    "linear-algebra/solvers/cholesky",
    "linear-algebra/solvers/durbin",
    "linear-algebra/solvers/gramschmidt",
    "linear-algebra/solvers/lu",
    "linear-algebra/solvers/ludcmp",
    "linear-algebra/solvers/trisolv",
    "medley/deriche",
    "medley/floyd-warshall",
    "medley/nussinov",
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

out = {}

def get_check_output(exec: str, type: str, omp_nthreads: int = 1, mkl_nthreads: int = 1) -> tuple[bool, dict[str, list[float]]]:
    print(f"Run {exec} with OMP_NTHREADS={omp_nthreads}, MKL_NTHREADS={mkl_nthreads}")
    out = subprocess.run(exec, capture_output=True, env=environ.update({"OMP_NUM_THREADS": str(omp_nthreads), "MKL_NUM_THREAD": str(mkl_nthreads)})).stderr.decode()
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

def get_run_output(exec: str, omp_nthreads: int, mkl_nthreads: int) -> float:
    out = subprocess.run(exec, capture_output=True, env=environ.update({"OMP_NUM_THREADS": str(omp_nthreads), "MKL_NUM_THREAD": str(mkl_nthreads)})).stdout.decode()
    result = float("nan")
    try:
        result = float(out.strip())
    except Exception:
        print(f"Could not convert to float in {exec} with OMP_NTHREADS={omp_nthreads}, MKL_NTHREADS={mkl_nthreads}")
    return result

def check_status(version_short: str, version_long: str, bench: str) -> str:
    check_exec = path.join(".", "bin", version_long, "check", bench)
    print(f"Check if {check_exec} exists")
    if not path.isfile(check_exec):
        return "unavailable"
    run_exec = path.join(".", "bin", version_long, "run", bench)
    print(f"Check if {run_exec} exists")
    if not path.isfile(run_exec):
        return "unavailable"
    ref_out_res, ref_out = get_check_output(path.join(".", "bin", "ref", "check", bench), "ref")
    if not ref_out_res:
        return "corrupt"
    opt_out_res, opt_out = get_check_output(check_exec, version_short)
    if not opt_out_res:
        return "corrupt"
    key_differences = set(ref_out.keys()).symmetric_difference(set(opt_out.keys()))
    if len(key_differences) > 0:
        print(f"Variable {key_differences.pop()} does not occur in both outputs...")
        return "mismatch"
    for key in ref_out:
        if len(ref_out[key]) != len(opt_out[key]):
            print(f"{key}: Different output lengths...")
            return "mismatch"
        for i in range(len(ref_out[key])):
            if abs(ref_out[key][i] - opt_out[key][i]) > 0.011:
                print(f"{key}: Values do not match at {i} ({ref_out[key][i]} != {opt_out[key][i]})...")
                return "mismatch"
    opt2_out_res, opt2_out = get_check_output(check_exec, version_short, omp_nthreads=OMP_NTHREADS, mkl_nthreads=MKL_NTHREADS)
    if not opt2_out_res:
        return "corrupt"
    key_differences2 = set(ref_out.keys()).symmetric_difference(set(opt2_out.keys()))
    if len(key_differences2) > 0:
        print(f"Variable {key_differences2.pop()} does not occur in both outputs...")
        return "mismatch"
    for key in ref_out:
        if len(ref_out[key]) != len(opt2_out[key]):
            print(f"{key}: Different output lengths...")
            return "mismatch"
        for i in range(len(ref_out[key])):
            if abs(ref_out[key][i] - opt2_out[key][i]) > 0.011:
                print(f"{key}: Values do not match at {i} ({ref_out[key][i]} != {opt2_out[key][i]})...")
                return "unstable"
    return "good"

def benchmark(version_short: str, version_long: str) -> int:
    result = 0
    for bench in BENCHMARKS:
        stdout.flush()
        print(f"Got benchmark {bench}")
        out[bench] = {"status": "unknown"}
        status = check_status(version_short, version_long, bench)
        out[bench]["status"] = status
        print(f"Got status {status}")
        if not status in ["unstable", "good"]:
            if status != "unavailable":
                result += 1
            continue
        out[bench]["data"] = []
        run_exec = path.join(".", "bin", version_long, "run", bench)
        print(f"Run {run_exec} with OMP_NTHREADS={OMP_NTHREADS}, MKL_NTHREADS={MKL_NTHREADS}")
        for i in range(REPS):
            print(f"Reptition {i + 1}/{REPS}")
            time = get_run_output(run_exec, OMP_NTHREADS, MKL_NTHREADS)
            if isnan(time):
                result += 1
                break
            out[bench]["data"].append(time)
            print(f"Took {time}")
    stdout.flush()
    return result

if __name__ == "__main__":
    VERSIONS = {
        "ref": "ref",
        "opt_mkl": "optimized_mkl",
        "opt_mkl3": "optimized_mkl3",
        "intel": "intel",
        "polly": "polly",
        "pluto": "pluto",
        "opt_cublas": "optimized_cublas"
    }
    from sys import argv
    from os import makedirs
    import json
    if len(argv) < 2 or (not argv[1] in VERSIONS):
        print("Usage: benchmark.py [version name]")
        print(f"Available versions: {VERSIONS.keys()}")
        exit(1)
    version = argv[1]
    print(f"Got version {version}")
    result = 1
    try:
        result = benchmark(version, VERSIONS[version])
    except Exception as e:
        if hasattr(e, "message"):
            print(e.message) # type: ignore
        else:
            print(e)
    if not path.exists("results"):
        makedirs("results")
        print("Created results/ folder")
    with open(f"results/{version}.json", "w+") as outfile:
        outfile.write(json.dumps(out, indent=True))
        outfile.write("\n")
    print(f"Output written to file results/{version}.json")
    exit(result)