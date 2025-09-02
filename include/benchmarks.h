#pragma once

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct DatasetSize {
    std::string name;
    std::string macroName;
    int medium_size;
    int extralarge_size;
};

enum VariableType { Scalar, Array1D, Array2D, Array3D, Array4D, Array5D };

class Variable {
    const VariableType type_;
    const std::string name_;
    const std::vector<size_t> dimensions_;

   public:
    Variable(const std::string name);
    Variable(const std::string name, const size_t dim1);
    Variable(const std::string name, const size_t dim1, const size_t dim2);
    Variable(const std::string name, const size_t dim1, const size_t dim2, const size_t dim3);
    Variable(const std::string name, const size_t dim1, const size_t dim2, const size_t dim3,
             const size_t dim4);
    Variable(const std::string name, const size_t dim1, const size_t dim2, const size_t dim3,
             const size_t dim4, const size_t dim5);

    VariableType type() const;
    size_t arity() const;

    const std::string& name() const;

    const std::vector<size_t>& dimensions() const;
};

class CodeRegion {
    const size_t scop_, endscop_;
    const std::vector<std::pair<size_t, size_t>> regions_;

   public:
    CodeRegion(size_t scop, size_t endscop, std::vector<std::pair<size_t, size_t>> regions);

    size_t scop() const;
    size_t endscop() const;

    const std::vector<std::pair<size_t, size_t>>& regions() const;
    bool in_regions(size_t line) const;
};

class Benchmark {
    const std::string name_;
    const std::string path_;
    const std::vector<DatasetSize> dataset_sizes_;
    const std::vector<Variable> variables_;
    const std::vector<size_t> call_variables_;
    const std::vector<size_t> print_variables_;
    const CodeRegion code_region_;

   public:
    Benchmark(const std::string name, const std::string path,
              const std::vector<DatasetSize> dataset_sizes, const std::vector<Variable> variables_,
              const std::vector<size_t> call_variables, const std::vector<size_t> print_variables,
              const CodeRegion code_region);

    const std::string& name() const;

    std::string json_path(bool check = true) const;

    std::string out_path(bool check = true) const;
    std::filesystem::path out_header_path(bool check = true) const;
    std::filesystem::path out_source_path(bool check = true) const;
    std::filesystem::path out_main_path(bool check = true) const;

    const std::vector<DatasetSize>& dataset_sizes() const;

    const std::vector<Variable>& variables() const;

    const std::vector<size_t>& call_variables() const;

    const std::vector<size_t>& print_variables() const;
    std::unordered_set<size_t> print_variables_dataset_sizes() const;

    const CodeRegion& code_region() const;
};

class BenchmarkRegistry {
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Benchmark*> benchmarks_;

   public:
    ~BenchmarkRegistry();

    static BenchmarkRegistry& instance() {
        static BenchmarkRegistry registry;
        return registry;
    }

    void register_benchmark(const std::string name, const std::string path,
                            const std::vector<DatasetSize> dataset_sizes,
                            const std::vector<Variable> variables,
                            const std::vector<size_t> call_variables,
                            const std::vector<size_t> print_variables,
                            const CodeRegion code_region);

    Benchmark* get_benchmark(const std::string name);

    std::string dump_benchmarks();
};

inline void register_benchmarks() {
    BenchmarkRegistry::instance().register_benchmark(
        "correlation", "datamining/correlation", {{"n", "N", 260, 3000}, {"m", "M", 240, 2600}},
        {{"float_n"}, {"data", 0, 1}, {"corr", 1, 1}, {"mean", 1}, {"stddev", 1}}, {3, 4, 2, 1, 2},
        {2}, {78, 122, {{31, 38}, {73, 123}}});
    BenchmarkRegistry::instance().register_benchmark(
        "covariance", "datamining/covariance", {{"n", "N", 260, 3000}, {"m", "M", 240, 2600}},
        {{"float_n"}, {"data", 0, 1}, {"cov", 1, 1}, {"mean", 1}}, {3, 2, 1, 2}, {2},
        {72, 94, {{30, 36}, {70, 95}}});
    BenchmarkRegistry::instance().register_benchmark(
        "gemm", "linear-algebra/blas/gemm",
        {{"ni", "NI", 200, 2000}, {"nj", "NJ", 220, 2300}, {"nk", "NK", 240, 2600}},
        {{"alpha"}, {"beta"}, {"C", 0, 1}, {"A", 0, 2}, {"B", 2, 1}}, {4, 3, 2}, {2},
        {88, 97, {{33, 45}, {79, 98}}});
    BenchmarkRegistry::instance().register_benchmark(
        "gemver", "linear-algebra/blas/gemver", {{"n", "N", 400, 4000}},
        {{"alpha"},
         {"beta"},
         {"A", 0, 0},
         {"u1", 0},
         {"v1", 0},
         {"u2", 0},
         {"v2", 0},
         {"w", 0},
         {"x", 0},
         {"y", 0},
         {"z", 0}},
        {7, 2, 8, 10, 9, 3, 4, 5, 6}, {7}, {99, 116, {{39, 58}, {97, 116}}});
    BenchmarkRegistry::instance().register_benchmark(
        "gesummv", "linear-algebra/blas/gesummv", {{"n", "N", 250, 2800}},
        {{"alpha"}, {"beta"}, {"A", 0, 0}, {"B", 0, 0}, {"tmp", 0}, {"x", 0}, {"y", 0}},
        {4, 6, 2, 5, 3, 6}, {6}, {82, 94, {{33, 44}, {80, 95}}});
    BenchmarkRegistry::instance().register_benchmark(
        "symm", "linear-algebra/blas/symm", {{"m", "M", 200, 2000}, {"n", "N", 240, 2600}},
        {{"alpha"}, {"beta"}, {"C", 0, 1}, {"A", 0, 0}, {"B", 0, 1}}, {3, 2, 4}, {2},
        {92, 103, {{33, 47}, {81, 104}}});
    BenchmarkRegistry::instance().register_benchmark(
        "syr2k", "linear-algebra/blas/syr2k", {{"n", "N", 240, 2600}, {"m", "M", 200, 2000}},
        {{"alpha"}, {"beta"}, {"C", 0, 0}, {"A", 0, 1}, {"B", 0, 1}}, {2, 3, 4, 2}, {2},
        {87, 97, {{33, 45}, {79, 98}}});
    BenchmarkRegistry::instance().register_benchmark(
        "syrk", "linear-algebra/blas/syrk", {{"n", "N", 240, 2600}, {"m", "M", 200, 2000}},
        {{"alpha"}, {"beta"}, {"C", 0, 0}, {"A", 0, 1}}, {2, 3, 2}, {2},
        {82, 91, {{32, 41}, {74, 92}}});
    BenchmarkRegistry::instance().register_benchmark(
        "trmm", "linear-algebra/blas/trmm", {{"m", "M", 200, 2000}, {"n", "N", 240, 2600}},
        {{"alpha"}, {"A", 0, 0}, {"B", 0, 1}}, {2, 1}, {2}, {85, 92, {{31, 43}, {75, 93}}});
    BenchmarkRegistry::instance().register_benchmark(
        "2mm", "linear-algebra/kernels/2mm",
        {{"ni", "NI", 180, 1600},
         {"nj", "NJ", 190, 1800},
         {"nk", "NK", 210, 2200},
         {"nl", "NL", 220, 2400}},
        {{"alpha"}, {"beta"}, {"tmp", 0, 1}, {"A", 0, 2}, {"B", 2, 1}, {"C", 1, 3}, {"D", 0, 3}},
        {4, 6, 2, 5, 3, 6}, {6}, {87, 103, {{34, 49}, {85, 104}}});
    BenchmarkRegistry::instance().register_benchmark(
        "3mm", "linear-algebra/kernels/3mm",
        {{"ni", "NI", 180, 1600},
         {"nj", "NJ", 190, 1800},
         {"nk", "NK", 200, 2000},
         {"nl", "NL", 210, 2200},
         {"nm", "NM", 220, 2400}},
        {{"E", 0, 1}, {"A", 0, 2}, {"B", 2, 1}, {"F", 1, 3}, {"C", 1, 4}, {"D", 4, 3}, {"G", 0, 3}},
        {2, 5, 0, 3, 6, 4, 1, 6}, {6}, {83, 108, {{32, 45}, {81, 109}}});
    BenchmarkRegistry::instance().register_benchmark(
        "atax", "linear-algebra/kernels/atax", {{"m", "M", 390, 1800}, {"n", "N", 410, 2200}},
        {{"A", 0, 1}, {"x", 1}, {"y", 1}, {"tmp", 0}}, {0, 2, 3, 1, 2}, {2},
        {73, 84, {{30, 38}, {71, 85}}});
    BenchmarkRegistry::instance().register_benchmark(
        "bicg", "linear-algebra/kernels/bicg", {{"n", "N", 410, 2200}, {"m", "M", 390, 1800}},
        {{"A", 0, 1}, {"s", 1}, {"q", 0}, {"p", 1}, {"r", 0}}, {4, 1, 2, 0, 3, 1, 2}, {1, 2},
        {82, 94, {{31, 39}, {80, 95}}});
    BenchmarkRegistry::instance().register_benchmark(
        "doitgen", "linear-algebra/kernels/doitgen",
        {{"nr", "NR", 50, 250}, {"nq", "NQ", 40, 220}, {"np", "NP", 60, 270}},
        {{"A", 0, 1, 2}, {"sum", 2}, {"C4", 2, 2}}, {2, 1, 0}, {0}, {72, 83, {{30, 38}, {70, 84}}});
    BenchmarkRegistry::instance().register_benchmark(
        "mvt", "linear-algebra/kernels/mvt", {{"n", "N", 400, 4000}},
        {{"A", 0, 0}, {"x1", 0}, {"x2", 0}, {"y_1", 0}, {"y_2", 0}}, {2, 0, 4, 1, 3}, {1, 2},
        {87, 94, {{33, 43}, {85, 95}}});
    // Problem with cholesky: Multiple SDFG JSON files. No motivation to merge and adapt test
    // framework.
    //
    // Problem with durbin: BlockFusion reorders two blocks with a dependency...
    // BenchmarkRegistry::instance().register_benchmark(
    //     "durbin", "linear-algebra/solvers/durbin", {{"n", "N", 400, 4000}},
    //     {{"r", 0}, {"y", 0}, {"z", 0}}, {1, 2, 0, 1}, {1}, {72, 93, {{29, 34}, {65, 94}}});
    BenchmarkRegistry::instance().register_benchmark(
        "gramschmidt", "linear-algebra/solvers/gramschmidt",
        {{"m", "M", 200, 2000}, {"n", "N", 240, 2600}}, {{"A", 0, 1}, {"R", 1, 1}, {"Q", 0, 1}},
        {1, 0, 2, 1}, {1, 2}, {88, 106, {{31, 40}, {84, 107}}});
    // Problem with lu: Multiple SDFG JSON files. Not motivation to merge and adapt test framework.
    //
    // Problem with ludcmp: Multiple SDFG JSON files. Not motivation to merge and adapt test
    // framework.
    BenchmarkRegistry::instance().register_benchmark(
        "trisolv", "linear-algebra/solvers/trisolv", {{"n", "N", 400, 4000}},
        {{"L", 0, 0}, {"x", 0}, {"b", 0}}, {2, 1, 0}, {1}, {73, 81, {{31, 39}, {71, 82}}});
    BenchmarkRegistry::instance().register_benchmark(
        "deriche", "medley/deriche", {{"w", "W", 720, 7680}, {"h", "H", 480, 4320}},
        {{"imgIn", 0, 1}, {"imgOut", 0, 1}, {"y1", 0, 1}, {"y2", 0, 1}}, {2, 3, 1, 0, 1}, {1},
        {82, 154, {{30, 37}, {72, 154}}});
    // Problem with floyd-warshall: No SDFG JSON with DATA_TYPE double.
    //
    // Problem with nussinov: No SDFG JSON with DATA_TYPE double.
    BenchmarkRegistry::instance().register_benchmark(
        "adi", "stencils/adi", {{"n", "N", 200, 2000}, {"tsteps", "TSTEPS", 100, 1000}},
        {{"u", 0, 0}, {"v", 0, 0}, {"p", 0, 0}, {"q", 0, 0}}, {1, 2, 3, 0}, {0},
        {79, 127, {{29, 35}, {73, 127}}});
    BenchmarkRegistry::instance().register_benchmark(
        "fdtd-2d", "stencils/fdtd-2d",
        {{"tmax", "TMAX", 100, 1000}, {"nx", "NX", 200, 2000}, {"ny", "NY", 240, 2600}},
        {{"ex", 1, 2}, {"ey", 1, 2}, {"hz", 1, 2}, {"_fict_", 0}}, {0, 2, 1, 3, 0, 2}, {0, 1, 2},
        {100, 118, {{34, 44}, {98, 118}}});
    BenchmarkRegistry::instance().register_benchmark(
        "heat-3d", "stencils/heat-3d", {{"n", "N", 40, 200}, {"tsteps", "TSTEPS", 100, 1000}},
        {{"A", 0, 0, 0}, {"B", 0, 0, 0}}, {1, 0}, {0}, {71, 94, {{30, 35}, {69, 95}}});
    BenchmarkRegistry::instance().register_benchmark(
        "jacobi-1d", "stencils/jacobi-1d", {{"n", "N", 400, 4000}, {"tsteps", "TSTEPS", 100, 1000}},
        {{"A", 0}, {"B", 0}}, {1, 0}, {0}, {71, 79, {{30, 36}, {69, 80}}});
    BenchmarkRegistry::instance().register_benchmark(
        "jacobi-2d", "stencils/jacobi-2d", {{"n", "N", 250, 2800}, {"tsteps", "TSTEPS", 100, 1000}},
        {{"A", 0, 0}, {"B", 0, 0}}, {1, 0}, {0}, {72, 82, {{30, 37}, {70, 83}}});
    BenchmarkRegistry::instance().register_benchmark(
        "seidel-2d", "stencils/seidel-2d", {{"n", "N", 400, 4000}, {"tsteps", "TSTEPS", 100, 1000}},
        {{"A", 0, 0}}, {0}, {0}, {67, 74, {{29, 33}, {65, 75}}});
}
