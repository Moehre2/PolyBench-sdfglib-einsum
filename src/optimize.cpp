#include <sdfg/analysis/analysis.h>
#include <sdfg/blas/blas_dispatcher.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/codegen/code_generators/c_code_generator.h>
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
#include "polybench_node.h"
#include "timer.h"

void generate_main(sdfg::codegen::PrettyPrinter& stream, Benchmark* benchmark,
                   const std::string sdfg_name) {
    stream << "#include <stdio.h>" << std::endl
           << "#include <string.h>" << std::endl
           << std::endl
           << "/* Include polybench common header. */" << std::endl
           << "#include <polybench.h>" << std::endl
           << std::endl
           << "/* Include generated header */" << std::endl
           << "#include \"generated.h\"" << std::endl
           << std::endl
           << "/* " << benchmark->name() << " */" << std::endl;
    for (auto& dataset_size : benchmark->dataset_sizes()) {
        stream << "#define " << dataset_size.macroName << " " << std::to_string(dataset_size.size)
               << std::endl;
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
           << sdfg_name << "(" << std::endl;
    stream.setIndent(10);
    for (size_t i = 0; i < benchmark->call_variables().size(); ++i) {
        if (i > 0) stream << ", " << std::endl;
        stream << "POLYBENCH_ARRAY("
               << benchmark->variables().at(benchmark->call_variables().at(i)).name() << ")";
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

int main(int argc, char* argv[]) {
    register_benchmarks();

    if (argc != 2) {
        std::cerr << "Usage: optimize [benchmark name]" << std::endl
                  << "Available benchmarks: " << BenchmarkRegistry::instance().dump_benchmarks()
                  << std::endl;
        return 1;
    }

    Benchmark* benchmark = BenchmarkRegistry::instance().get_benchmark(argv[1]);
    if (!benchmark) {
        std::cerr << "Unknown benchmark: " << argv[1] << std::endl
                  << "Available benchmarks: " << BenchmarkRegistry::instance().dump_benchmarks()
                  << std::endl;
        return 1;
    }

    sdfg::codegen::register_default_dispatchers();
    sdfg::serializer::register_default_serializers();

    sdfg::einsum::register_einsum_dispatcher();
    sdfg::blas::register_blas_dispatchers();

    sdfg::polybench::register_polybench_dispatcher();

    const std::string jsonFile(benchmark->json_path());
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

    sdfg::codegen::CCodeGenerator generator(builder.subject());
    if (!generator.generate()) {
        std::cerr << "Error: Could not generate C sources" << std::endl;
        return 1;
    }

    std::filesystem::create_directories(benchmark->out_path());

    if (!generator.as_source(benchmark->out_header_path(), benchmark->out_source_path())) {
        std::cerr << "Error: Could not output C sources" << std::endl;
        std::cerr << benchmark->out_header_path() << std::endl;
        return 1;
    }

    std::ofstream out_header;
    out_header.open(benchmark->out_header_path(), std::ios_base::app);
    out_header << std::endl
               << "#include <polybench.h>" << std::endl
               << "#include <cblas.h>" << std::endl
               << generator.function_definition() << ";" << std::endl;
    out_header.close();

    sdfg::codegen::PrettyPrinter main_stream;
    generate_main(main_stream, benchmark, builder.subject().name());
    std::ofstream out_main;
    out_main.open(benchmark->out_main_path());
    if (!out_main.good()) {
        std::cerr << "Error" << std::endl;
    }
    out_main << main_stream.str();
    out_main.close();

    return 0;
}
