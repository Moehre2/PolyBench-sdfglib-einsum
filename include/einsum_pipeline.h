#pragma once

#include <sdfg/analysis/analysis.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/einsum/einsum_node.h>
#include <sdfg/passes/pass.h>
#include <sdfg/structured_control_flow/block.h>
#include <sdfg/structured_control_flow/sequence.h>
#include <sdfg/structured_control_flow/structured_loop.h>

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace sdfg {
namespace passes {

class EinsumPipeline : public Pass {
    std::vector<
        std::pair<std::vector<std::reference_wrapper<structured_control_flow::StructuredLoop>>,
                  structured_control_flow::Block&>>
    get_einsum_loops(builder::StructuredSDFGBuilder& builder);

    std::vector<std::pair<structured_control_flow::StructuredLoop&, einsum::EinsumNode&>>
    get_einsum_node_loops(builder::StructuredSDFGBuilder& builder);

    std::vector<std::reference_wrapper<einsum::EinsumNode>> get_einsum_nodes(
        builder::StructuredSDFGBuilder& builder);

    void block_fusion(builder::StructuredSDFGBuilder& builder,
                      analysis::AnalysisManager& analysis_manager,
                      structured_control_flow::Sequence& parent,
                      structured_control_flow::Sequence& node);

   public:
    EinsumPipeline();

    virtual std::string name() override;

    virtual bool run_pass(builder::StructuredSDFGBuilder& builder,
                          analysis::AnalysisManager& analysis_manager) override;
};

}  // namespace passes
}  // namespace sdfg