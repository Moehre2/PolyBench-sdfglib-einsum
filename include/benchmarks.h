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
    int size;
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
    const std::string json_path_;
    const std::string out_path_;
    const std::vector<DatasetSize> dataset_sizes_;
    const std::vector<Variable> variables_;
    const std::vector<size_t> call_variables_;
    const std::vector<size_t> print_variables_;
    const CodeRegion code_region_;

   public:
    Benchmark(const std::string name, const std::string json_path, const std::string out_path,
              const std::vector<DatasetSize> dataset_sizes, const std::vector<Variable> variables_,
              const std::vector<size_t> call_variables, const std::vector<size_t> print_variables,
              const CodeRegion code_region);

    const std::string& name() const;

    const std::string& json_path() const;

    const std::string& out_path() const;
    std::filesystem::path out_header_path() const;
    std::filesystem::path out_source_path() const;
    std::filesystem::path out_main_path() const;

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

    void register_benchmark(const std::string name, const std::string json_path,
                            const std::string out_path,
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
        "correlation", "sdfg_json/datamining/correlation.json",
        "optimized_c/datamining/correlation", {{"n", "N", 260}, {"m", "M", 240}},
        {{"float_n"}, {"data", 0, 1}, {"corr", 1, 1}, {"mean", 1}, {"stddev", 1}}, {3, 4, 2, 1, 2},
        {2}, {78, 122, {{31, 38}, {73, 123}}});
    BenchmarkRegistry::instance().register_benchmark(
        "covariance", "sdfg_json/datamining/covariance.json", "optimized_c/datamining/covariance",
        {{"n", "N", 260}, {"m", "M", 240}},
        {{"float_n"}, {"data", 0, 1}, {"cov", 1, 1}, {"mean", 1}}, {3, 2, 1, 2}, {2},
        {72, 94, {{30, 36}, {70, 95}}});
    BenchmarkRegistry::instance().register_benchmark(
        "gemm", "sdfg_json/linear-algebra/blas/gemm.json", "optimized_c/linear-algebra/blas/gemm",
        {{"ni", "NI", 200}, {"nj", "NJ", 220}, {"nk", "NK", 240}},
        {{"alpha"}, {"beta"}, {"C", 0, 1}, {"A", 0, 2}, {"B", 2, 1}}, {4, 3, 2}, {2},
        {88, 97, {{33, 45}, {79, 98}}});
    BenchmarkRegistry::instance().register_benchmark(
        "gemver", "sdfg_json/linear-algebra/blas/gemver.json",
        "optimized_c/linear-algebra/blas/gemver", {{"n", "N", 400}},
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
        "gesummv", "sdfg_json/linear-algebra/blas/gesummv.json",
        "optimized_c/linear-algebra/blas/gesummv", {{"n", "N", 250}},
        {{"alpha"}, {"beta"}, {"A", 0, 0}, {"B", 0, 0}, {"tmp", 0}, {"x", 0}, {"y", 0}},
        {4, 6, 2, 5, 3, 6}, {6}, {82, 94, {{33, 44}, {80, 95}}});
    BenchmarkRegistry::instance().register_benchmark(
        "symm", "sdfg_json/linear-algebra/blas/symm.json", "optimized_c/linear-algebra/blas/symm",
        {{"m", "M", 200}, {"n", "N", 240}},
        {{"alpha"}, {"beta"}, {"C", 0, 1}, {"A", 0, 0}, {"B", 0, 1}}, {3, 2, 4}, {2},
        {92, 103, {{33, 47}, {81, 104}}});
    BenchmarkRegistry::instance().register_benchmark(
        "syr2k", "sdfg_json/linear-algebra/blas/syr2k.json",
        "optimized_c/linear-algebra/blas/syr2k", {{"n", "N", 240}, {"m", "M", 200}},
        {{"alpha"}, {"beta"}, {"C", 0, 0}, {"A", 0, 1}, {"B", 0, 1}}, {2, 3, 4, 2}, {2},
        {87, 97, {{33, 45}, {79, 98}}});
    BenchmarkRegistry::instance().register_benchmark(
        "bicg", "sdfg_json/linear-algebra/kernels/bicg.json",
        "optimized_c/linear-algebra/kernels/bicg", {{"n", "N", 410}, {"m", "M", 390}},
        {{"A", 0, 1}, {"s", 1}, {"q", 0}, {"p", 1}, {"r", 0}}, {4, 1, 2, 0, 3, 1, 2}, {1, 2},
        {82, 94, {{31, 39}, {80, 95}}});
    BenchmarkRegistry::instance().register_benchmark(
        "heat-3d", "sdfg_json/stencils/heat-3d.json", "optimized_c/stencils/heat-3d",
        {{"n", "N", 40}, {"tsteps", "TSTEPS", 100}}, {{"A", 0, 0, 0}, {"B", 0, 0, 0}}, {1, 0}, {0},
        {71, 94, {{30, 35}, {69, 95}}});
}
