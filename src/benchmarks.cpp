#include "benchmarks.h"

#include <cstddef>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

Variable::Variable(const std::string name) : type_(Scalar), name_(name), dimensions_() {}

Variable::Variable(const std::string name, const size_t dim1)
    : type_(Array1D), name_(name), dimensions_({dim1}) {}

Variable::Variable(const std::string name, const size_t dim1, const size_t dim2)
    : type_(Array2D), name_(name), dimensions_({dim1, dim2}) {}

Variable::Variable(const std::string name, const size_t dim1, const size_t dim2, const size_t dim3)
    : type_(Array3D), name_(name), dimensions_({dim1, dim2, dim3}) {}

Variable::Variable(const std::string name, const size_t dim1, const size_t dim2, const size_t dim3,
                   const size_t dim4)
    : type_(Array4D), name_(name), dimensions_({dim1, dim2, dim3, dim4}) {}

Variable::Variable(const std::string name, const size_t dim1, const size_t dim2, const size_t dim3,
                   const size_t dim4, const size_t dim5)
    : type_(Array4D), name_(name), dimensions_({dim1, dim2, dim3, dim4, dim5}) {}

VariableType Variable::type() const { return this->type_; }

size_t Variable::arity() const {
    switch (this->type()) {
        case Scalar:
            return 0;
        case Array1D:
            return 1;
        case Array2D:
            return 2;
        case Array3D:
            return 3;
        case Array4D:
            return 4;
        case Array5D:
            return 5;
    }
}

const std::string& Variable::name() const { return this->name_; }

const std::vector<size_t>& Variable::dimensions() const { return this->dimensions_; }

CodeRegion::CodeRegion(const size_t scop, const size_t endscop,
                       const std::vector<std::pair<size_t, size_t>> regions)
    : scop_(scop), endscop_(endscop), regions_(regions) {
    if (endscop <= scop) {
        throw std::runtime_error("scop must occur before endscop");
    }

    for (auto& region : regions) {
        if (region.first > region.second) {
            throw std::runtime_error("Regions cannot end before starting");
        }
    }
}

size_t CodeRegion::scop() const { return this->scop_; }

size_t CodeRegion::endscop() const { return this->endscop_; }

const std::vector<std::pair<size_t, size_t>>& CodeRegion::regions() const { return this->regions_; }

bool CodeRegion::in_regions(size_t line) const {
    for (auto& region : this->regions_) {
        if (line >= region.first && line <= region.second) return true;
    }
    return false;
}

Benchmark::Benchmark(const BLASImplementation impl, const std::string name, const std::string path,
                     const std::vector<DatasetSize> dataset_sizes,
                     const std::vector<Variable> variables,
                     const std::vector<size_t> call_variables,
                     const std::vector<size_t> print_variables, const CodeRegion code_region)
    : impl_(impl),
      name_(name),
      path_(path),
      dataset_sizes_(dataset_sizes),
      variables_(variables),
      call_variables_(call_variables),
      print_variables_(print_variables),
      code_region_(code_region) {
    for (auto& variable : variables) {
        for (size_t dim : variable.dimensions()) {
            if (dim >= dataset_sizes.size()) {
                throw std::runtime_error("Variable " + variable.name() +
                                         " references dataset size " + std::to_string(dim) +
                                         " >= " + std::to_string(dataset_sizes.size()));
            }
        }
    }

    for (size_t call_variable : call_variables) {
        if (call_variable >= variables.size()) {
            throw std::runtime_error("Call variables: " + std::to_string(call_variable) +
                                     " >= " + std::to_string(variables.size()));
        }
    }

    for (size_t print_variable : print_variables) {
        if (print_variable >= variables.size()) {
            throw std::runtime_error("Print variables: " + std::to_string(print_variable) +
                                     " >= " + std::to_string(variables.size()));
        }
    }
}

const std::string& Benchmark::name() const { return this->name_; }

std::string Benchmark::json_path(bool check) const {
    if (check)
        return (std::filesystem::path("sdfg_json/check") / (this->path_ + ".json")).string();
    else
        return (std::filesystem::path("sdfg_json/run") / (this->path_ + ".json")).string();
}

std::string Benchmark::out_root_folder() const {
    switch (this->impl_) {
        case MKL:
            return "optimized_mkl";
        case MKL3:
            return "optimized_mkl3";
    }
}

std::string Benchmark::out_path(bool check) const {
    if (check)
        return (std::filesystem::path(this->out_root_folder()) / "check" / this->path_).string();
    else
        return (std::filesystem::path(this->out_root_folder()) / "run" / this->path_).string();
}

std::filesystem::path Benchmark::out_header_path(bool check) const {
    return std::filesystem::path(this->out_path(check)) / "generated.h";
}

std::filesystem::path Benchmark::out_source_path(bool check) const {
    return std::filesystem::path(this->out_path(check)) / "generated.c";
}

std::filesystem::path Benchmark::out_main_path(bool check) const {
    return std::filesystem::path(this->out_path(check)) / (this->name_ + ".c");
}

const std::vector<DatasetSize>& Benchmark::dataset_sizes() const { return this->dataset_sizes_; }

const std::vector<Variable>& Benchmark::variables() const { return this->variables_; }

const std::vector<size_t>& Benchmark::call_variables() const { return this->call_variables_; }

const std::vector<size_t>& Benchmark::print_variables() const { return this->print_variables_; }

std::unordered_set<size_t> Benchmark::print_variables_dataset_sizes() const {
    std::unordered_set<size_t> result;

    for (size_t print_variable : this->print_variables()) {
        for (size_t dim : this->variables().at(print_variable).dimensions()) {
            result.insert(dim);
        }
    }

    return result;
}

const CodeRegion& Benchmark::code_region() const { return this->code_region_; }

BenchmarkRegistry::~BenchmarkRegistry() {
    for (auto benchmark : this->benchmarks_) {
        delete benchmark.second;
    }
}

void BenchmarkRegistry::setBLASImplementation(BLASImplementation impl) { this->impl_ = impl; }

void BenchmarkRegistry::register_benchmark(const std::string name, const std::string path,
                                           const std::vector<DatasetSize> dataset_sizes,
                                           const std::vector<Variable> variables,
                                           const std::vector<size_t> call_variables,
                                           const std::vector<size_t> print_variables,
                                           const CodeRegion code_region) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    if (this->benchmarks_.contains(name)) {
        throw std::runtime_error("Benchmark already registered with name: " + name);
    }
    this->benchmarks_[name] = new Benchmark(impl_, name, path, dataset_sizes, variables,
                                            call_variables, print_variables, code_region);
}

Benchmark* BenchmarkRegistry::get_benchmark(const std::string name) {
    auto it = this->benchmarks_.find(name);
    if (it != this->benchmarks_.end()) return it->second;
    return nullptr;
}

std::string BenchmarkRegistry::dump_benchmarks() {
    std::stringstream result;

    for (auto& benchmark : this->benchmarks_) {
        if (!result.str().empty()) result << ", ";
        result << benchmark.first;
    }

    return result.str();
}
