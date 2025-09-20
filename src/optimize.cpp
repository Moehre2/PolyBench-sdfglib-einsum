#include "optimize.h"

#include <sdfg/analysis/analysis.h>
#include <sdfg/blas/blas_dispatcher.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/codegen/code_generators/c_code_generator.h>
#include <sdfg/codegen/code_generators/cpp_code_generator.h>
#include <sdfg/codegen/dispatchers/node_dispatcher_registry.h>
#include <sdfg/codegen/utils.h>
#include <sdfg/einsum/einsum_dispatcher.h>
#include <sdfg/serializer/json_serializer.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json_fwd.hpp>
#include <string>

#include "benchmarks.h"
#include "einsum_pipeline.h"
#include "polybench_node.h"
#include "timer.h"

void generate_main(sdfg::codegen::PrettyPrinter& stream, Benchmark* benchmark,
                   const sdfg::StructuredSDFG& sdfg, bool check, BLASImplementation impl) {
    if (impl == CUBLAS) {
        stream << "#include <cstdio>" << std::endl
               << "#include <cstring>" << std::endl
               << std::endl
               << "/* Include polybench common header. */" << std::endl
               << "#include <polybench.cuh>" << std::endl
               << std::endl
               << "/* Include generated header */" << std::endl
               << "#include \"generated.cuh\"" << std::endl
               << std::endl;
    } else {
        stream << "#include <stdio.h>" << std::endl
               << "#include <string.h>" << std::endl
               << std::endl
               << "/* Include polybench common header. */" << std::endl
               << "#include <polybench.h>" << std::endl
               << std::endl
               << "/* Include generated header */" << std::endl
               << "#include \"generated.h\"" << std::endl
               << std::endl;
    }
    stream << "/* " << benchmark->name() << " */" << std::endl;
    for (auto& dataset_size : benchmark->dataset_sizes()) {
        stream << "#define " << dataset_size.macroName << " ";
        if (check)
            stream << dataset_size.medium_size;
        else
            stream << dataset_size.extralarge_size;
        stream << std::endl;
    }
    stream << "#define DATA_TYPE double" << std::endl
           << "#define DATA_PRINTF_MODIFIER \"%0.2lf \"" << std::endl
           << std::endl
           << "/* DCE code. Must scan the entire live-out data." << std::endl
           << "   Can be used also to check the correctness of the output. */" << std::endl
           << "static void print_array(";
    for (size_t dataset_size : benchmark->print_variables_dataset_sizes()) {
        stream << "int " << benchmark->dataset_sizes().at(dataset_size).name << ", ";
    }
    stream << std::endl;
    stream.setIndent(24);
    for (size_t i = 0; i < benchmark->print_variables().size(); ++i) {
        const Variable& variable = benchmark->variables().at(benchmark->print_variables().at(i));
        if (i > 0) stream << "," << std::endl;
        stream << "DATA_TYPE ";
        if (variable.type() == Scalar) {
            stream << variable.name();
        } else {
            stream << "POLYBENCH_" << std::to_string(variable.arity()) << "D(" << variable.name();
            for (size_t dim : variable.dimensions())
                stream << ", " << benchmark->dataset_sizes().at(dim).macroName;
            for (size_t dim : variable.dimensions())
                stream << ", " << benchmark->dataset_sizes().at(dim).name;
            stream << ")";
        }
    }
    stream << ") {" << std::endl;
    stream.setIndent(2);
    stream << "POLYBENCH_DUMP_START;" << std::endl;
    for (size_t print_variable : benchmark->print_variables()) {
        const Variable& variable = benchmark->variables().at(print_variable);
        stream << "POLYBENCH_DUMP_BEGIN(\"" << variable.name() << "\");" << std::endl;
        for (size_t i = 0; i < variable.dimensions().size(); ++i) {
            stream << "for (int i_" << std::to_string(i) << " = 0; i_" << std::to_string(i) << " < "
                   << benchmark->dataset_sizes().at(variable.dimensions().at(i)).name << "; i_"
                   << std::to_string(i) << "++) {" << std::endl;
            stream.setIndent(stream.indent() + 2);
        }
        if (variable.dimensions().size() > 0) {
            const std::string dim_name0 =
                benchmark->dataset_sizes().at(variable.dimensions().at(0)).name;
            stream << "if (";
            if (variable.dimensions().size() == 1) {
                stream << "i_0";
            } else {
                stream << "(";
                for (size_t i = 0; i < variable.dimensions().size(); ++i) {
                    if (i > 0) stream << " + ";
                    stream << "i_" << std::to_string(i);
                    for (size_t j = 0; j < variable.dimensions().size() - i - 1; ++j)
                        stream << " * " << dim_name0;
                }
                stream << ")";
            }
            stream << " % 20 == 0) fprintf(POLYBENCH_DUMP_TARGET, \"\\n\");" << std::endl;
        }
        stream << "fprintf(POLYBENCH_DUMP_TARGET, DATA_PRINTF_MODIFIER, " << variable.name();
        for (size_t i = 0; i < variable.dimensions().size(); ++i)
            stream << "[i_" << std::to_string(i) << "]";
        stream << ");" << std::endl;
        for (size_t i = 0; i < variable.dimensions().size(); ++i) {
            stream.setIndent(stream.indent() - 2);
            stream << "}" << std::endl;
        }
        stream << "POLYBENCH_DUMP_END(\"" << variable.name() << "\");" << std::endl;
    }
    stream << "POLYBENCH_DUMP_FINISH;" << std::endl;
    stream.setIndent(0);
    stream << "}" << std::endl << std::endl << "int main(int argc, char** argv) {" << std::endl;
    stream.setIndent(2);
    stream << "/* Retrieve problem size. */" << std::endl;
    for (auto& dataset_size : benchmark->dataset_sizes()) {
        stream << "int " << dataset_size.name << " = " << dataset_size.macroName << ";"
               << std::endl;
    }
    stream << std::endl << "/* Variable declaration/allocation. */" << std::endl;
    for (auto& variable : benchmark->variables()) {
        if (variable.type() == Scalar) {
            stream << "DATA_TYPE " << variable.name() << ";" << std::endl;
        } else {
            stream << "POLYBENCH_" << std::to_string(variable.arity()) << "D_ARRAY_DECL("
                   << variable.name() << ", DATA_TYPE";
            for (size_t dim : variable.dimensions())
                stream << ", " << benchmark->dataset_sizes().at(dim).macroName;
            for (size_t dim : variable.dimensions())
                stream << ", " << benchmark->dataset_sizes().at(dim).name;
            stream << ");" << std::endl;
        }
    }
    stream << std::endl
           << "/* Call generated function. */" << std::endl
           << sdfg.name() << "(" << std::endl;
    stream.setIndent(10);
    if (impl == CUBLAS) {
        sdfg::codegen::CPPLanguageExtension le;
        for (size_t i = 0; i < benchmark->call_variables().size(); ++i) {
            if (i > 0) stream << ", " << std::endl;
            stream << le.type_cast(
                "POLYBENCH_ARRAY(" +
                    benchmark->variables().at(benchmark->call_variables().at(i)).name() + ")",
                sdfg.type(sdfg.arguments().at(i)));
        }
    } else {
        for (size_t i = 0; i < benchmark->call_variables().size(); ++i) {
            if (i > 0) stream << ", " << std::endl;
            stream << "POLYBENCH_ARRAY("
                   << benchmark->variables().at(benchmark->call_variables().at(i)).name() << ")";
        }
    }
    stream << ");" << std::endl;
    stream.setIndent(2);
    stream << std::endl
           << "/* Prevent dead-code elimination. All live-out data must be printed" << std::endl
           << "   by the function call in argument. */" << std::endl
           << "polybench_prevent_dce(print_array(";
    for (size_t dataset_size : benchmark->print_variables_dataset_sizes()) {
        stream << benchmark->dataset_sizes().at(dataset_size).name << ", ";
    }
    stream << std::endl;
    stream.setIndent(36);
    for (size_t i = 0; i < benchmark->print_variables().size(); ++i) {
        if (i > 0) stream << "," << std::endl;
        stream << "POLYBENCH_ARRAY("
               << benchmark->variables().at(benchmark->print_variables().at(i)).name() << ")";
    }
    stream << "));" << std::endl;
    stream.setIndent(2);
    stream << std::endl << "/* Be clean. */" << std::endl;
    for (auto& variable : benchmark->variables()) {
        if (variable.type() == Scalar) continue;
        stream << "POLYBENCH_FREE_ARRAY(" << variable.name() << ");" << std::endl;
    }
    stream << std::endl << "return 0;" << std::endl;
    stream.setIndent(0);
    stream << "}" << std::endl;
}

sdfg::blas::BLASImplementation convert_blas_impl(BLASImplementation impl) {
    switch (impl) {
        case MKL:
            return sdfg::blas::BLASImplementation_CBLAS;
        case MKL3:
            return sdfg::blas::BLASImplementation_CBLAS;
        case CUBLAS:
            return sdfg::blas::BLASImplementation_CUBLAS;
        default:
            return sdfg::blas::BLASImplementation_CBLAS;
    }
}

int optimize(BLASImplementation impl, int argc, char* argv[]) {
    register_benchmarks(impl);

    if (argc != 3) {
        std::cerr << "Usage: optimize [check|run] [benchmark name]" << std::endl
                  << "Available benchmarks: " << BenchmarkRegistry::instance().dump_benchmarks()
                  << std::endl;
        return 1;
    }

    std::string argv_1(argv[1]);
    bool check;
    if (argv_1 == "check") {
        check = true;
    } else if (argv_1 == "run") {
        check = false;
    } else {
        std::cerr << "Usage: optimize [check|run] [benchmark name]" << std::endl
                  << "Available benchmarks: " << BenchmarkRegistry::instance().dump_benchmarks()
                  << std::endl;
        return 1;
    }

    Benchmark* benchmark = BenchmarkRegistry::instance().get_benchmark(argv[2]);
    if (!benchmark) {
        std::cerr << "Unknown benchmark: " << argv[2] << std::endl
                  << "Available benchmarks: " << BenchmarkRegistry::instance().dump_benchmarks()
                  << std::endl;
        return 1;
    }

    sdfg::codegen::register_default_dispatchers();
    sdfg::serializer::register_default_serializers();

    sdfg::einsum::register_einsum_dispatcher();
    sdfg::blas::register_blas_dispatchers(convert_blas_impl(impl));

    sdfg::polybench::register_polybench_dispatcher();

    const std::string jsonFile(benchmark->json_path(check));
    std::ifstream stream(jsonFile);
    if (!stream.good()) {
        std::cerr << "Could not open file: " << jsonFile << std::endl;
        return 1;
    }
    nlohmann::json json = nlohmann::json::parse(stream);

    sdfg::serializer::JSONSerializer serializer;
    auto sdfg = serializer.deserialize(json);

    sdfg::builder::StructuredSDFGBuilder builder(sdfg);
    sdfg::analysis::AnalysisManager analysis_manager(builder.subject());

    sdfg::passes::PolyBenchTimerInstrumentation pass(benchmark->code_region());
    if (!pass.run(builder, analysis_manager)) {
        std::cerr << "Error: Could not add polybench instrumentation to SDFG" << std::endl;
        return 1;
    }

    sdfg::passes::EinsumPipeline einsum_pipeline(impl);
    einsum_pipeline.run(builder, analysis_manager);

    if (impl == CUBLAS) {
        sdfg::codegen::CPPCodeGenerator generator(builder.subject());
        if (!generator.generate()) {
            std::cerr << "Error: Could not generate CUDA sources" << std::endl;
            return 1;
        }

        std::filesystem::create_directories(benchmark->out_path(check));

        if (!generator.as_source(benchmark->out_header_path(check),
                                 benchmark->out_source_path(check))) {
            std::cerr << "Error: Could not output CUDA sources" << std::endl;
            std::cerr << benchmark->out_header_path(check) << std::endl;
            return 1;
        }

        std::ofstream out_header;
        out_header.open(benchmark->out_header_path(check), std::ios_base::app);
        out_header << std::endl
                   << "#include <cstdio>" << std::endl
                   << "#include <polybench.cuh>" << std::endl
                   << "#include <cuda.h>" << std::endl
                   << "#include <cublas_v2.h>" << std::endl
                   << generator.function_definition() << ";" << std::endl;
        out_header.close();
    } else {
        sdfg::codegen::CCodeGenerator generator(builder.subject());
        if (!generator.generate()) {
            std::cerr << "Error: Could not generate C sources" << std::endl;
            return 1;
        }

        std::filesystem::create_directories(benchmark->out_path(check));

        if (!generator.as_source(benchmark->out_header_path(check),
                                 benchmark->out_source_path(check))) {
            std::cerr << "Error: Could not output C sources" << std::endl;
            std::cerr << benchmark->out_header_path(check) << std::endl;
            return 1;
        }

        std::ofstream out_header;
        out_header.open(benchmark->out_header_path(check), std::ios_base::app);
        out_header << std::endl
                   << "#include <polybench.h>" << std::endl
                   << "#include <mkl.h>" << std::endl
                   << generator.function_definition() << ";" << std::endl;
        out_header.close();
    }

    sdfg::codegen::PrettyPrinter main_stream;
    generate_main(main_stream, benchmark, builder.subject(), check, impl);
    std::ofstream out_main;
    out_main.open(benchmark->out_main_path(check));
    if (!out_main.good()) {
        std::cerr << "Error" << std::endl;
    }
    out_main << main_stream.str();
    out_main.close();

    return 0;
}
