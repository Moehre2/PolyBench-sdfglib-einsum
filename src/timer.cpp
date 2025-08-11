#include "timer.h"

#include <sdfg/analysis/analysis.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/element.h>
#include <sdfg/structured_control_flow/block.h>

#include <cstddef>
#include <string>

#include "benchmarks.h"
#include "polybench_node.h"

namespace sdfg {
namespace passes {

PolyBenchTimerInstrumentation::PolyBenchTimerInstrumentation(const CodeRegion& code_region)
    : Pass(), code_region_(code_region) {};

std::string PolyBenchTimerInstrumentation::name() { return "PolyBenchTimerInstrumentation"; }

bool PolyBenchTimerInstrumentation::run_pass(builder::StructuredSDFGBuilder& builder,
                                             analysis::AnalysisManager& analysis_manager) {
    auto& root = builder.subject().root();

    bool seen_scop = false, seen_endscop = false;
    size_t scop_index, endscop_index;
    for (size_t i = 0; i < root.size(); ++i) {
        auto& debug_info = root.at(i).first.debug_info();
        if (!debug_info.has()) continue;
        if (!seen_scop && this->code_region_.in_regions(debug_info.start_line()) &&
            debug_info.start_line() > this->code_region_.scop()) {
            scop_index = i;
            seen_scop = true;
        }
        if (!seen_endscop && this->code_region_.in_regions(debug_info.start_line()) &&
            debug_info.start_line() > this->code_region_.endscop()) {
            endscop_index = i;
            seen_endscop = true;
        }
    }
    if (!seen_scop) return false;
    if (!seen_endscop) endscop_index = root.size();
    if (endscop_index < scop_index) return false;

    structured_control_flow::Block* endscop_block;
    if (endscop_index == root.size()) {
        endscop_block = &builder.add_block(root);
    } else {
        endscop_block = &builder.add_block_before(root, root.at(endscop_index).first).first;
    }
    builder.add_library_node<polybench::PolyBenchNode, const polybench::PolyBenchNodeType>(
        *endscop_block, DebugInfo(), polybench::StopAndPrintInstruments);

    auto& scop_block = builder.add_block_before(root, root.at(scop_index).first).first;
    builder.add_library_node<polybench::PolyBenchNode, const polybench::PolyBenchNodeType>(
        scop_block, DebugInfo(), polybench::StartInstruments);

    return true;
}

}  // namespace passes
}  // namespace sdfg