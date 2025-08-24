#include "my_loop_distribute.h"

#include <sdfg/analysis/analysis.h>
#include <sdfg/analysis/data_dependency_analysis.h>
#include <sdfg/analysis/scope_analysis.h>
#include <sdfg/analysis/users.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/data_flow/access_node.h>
#include <sdfg/data_flow/memlet.h>
#include <sdfg/structured_control_flow/block.h>
#include <sdfg/structured_control_flow/map.h>
#include <sdfg/structured_control_flow/structured_loop.h>
#include <sdfg/symbolic/symbolic.h>
#include <sdfg/transformations/transformation.h>

#include <string>
#include <unordered_map>
#include <utility>

namespace sdfg {
namespace transformations {

bool MyLoopDistribute::subset_contains(data_flow::Subset& subset, symbolic::Symbol& sym) {
    for (auto& expr : subset) {
        if (symbolic::uses(expr, sym)) return true;
    }
    return false;
}

MyLoopDistribute::MyLoopDistribute(structured_control_flow::StructuredLoop& loop) : loop_(loop) {};

std::string MyLoopDistribute::name() const { return "MyLoopDistribute"; };

bool MyLoopDistribute::can_be_applied(builder::StructuredSDFGBuilder& builder,
                                      analysis::AnalysisManager& analysis_manager) {
    if (this->loop_.root().size() != 2) return false;
    if (!this->loop_.root().at(0).second.assignments().empty()) return false;

    auto* block1 = dynamic_cast<structured_control_flow::Block*>(&this->loop_.root().at(0).first);
    if (!block1) return false;

    auto* block2 = dynamic_cast<structured_control_flow::Block*>(&this->loop_.root().at(1).first);
    if (!block2) return false;

    std::unordered_map<std::string, data_flow::Subset> block1_writes;
    for (auto* block1_sink_node : block1->dataflow().sinks()) {
        if (auto* block1_sink = dynamic_cast<data_flow::AccessNode*>(block1_sink_node)) {
            for (auto& iedge : block1->dataflow().in_edges(*block1_sink_node)) {
                block1_writes.insert({block1_sink->data(), iedge.subset()});
            }
        }
    }

    std::unordered_map<std::string, data_flow::Subset> block2_reads;
    for (auto* block2_source_node : block2->dataflow().sources()) {
        if (auto* block2_source = dynamic_cast<data_flow::AccessNode*>(block2_source_node)) {
            for (auto& oedge : block2->dataflow().out_edges(*block2_source_node)) {
                block2_reads.insert({block2_source->data(), oedge.subset()});
            }
        }
    }

    for (auto [container1, subset1] : block1_writes) {
        for (auto [container2, subset2] : block2_reads) {
            if (container1 == container2) {
                if (!this->subset_contains(subset1, this->loop_.indvar()) ||
                    !this->subset_contains(subset2, this->loop_.indvar()))
                    return false;
            }
            auto container1_sym = symbolic::symbol(container1);
            if (this->subset_contains(subset2, container1_sym)) return false;
        }
    }

    return true;
};

void MyLoopDistribute::apply(builder::StructuredSDFGBuilder& builder,
                             analysis::AnalysisManager& analysis_manager) {
    auto& sdfg = builder.subject();

    auto indvar = this->loop_.indvar();
    auto condition = this->loop_.condition();
    auto update = this->loop_.update();
    auto init = this->loop_.init();

    auto& body = this->loop_.root();
    auto& child = body.at(0).first;

    auto& analysis = analysis_manager.get<analysis::ScopeAnalysis>();
    auto parent =
        static_cast<structured_control_flow::Sequence*>(analysis.parent_scope(&this->loop_));
    structured_control_flow::StructuredLoop* new_loop;
    if (auto map_stmt = dynamic_cast<structured_control_flow::Map*>(&this->loop_)) {
        structured_control_flow::ScheduleType schedule_type = map_stmt->schedule_type();
        new_loop = &builder
                        .add_map_before(*parent, this->loop_, indvar, condition, init, update,
                                        schedule_type, {}, this->loop_.debug_info())
                        .first;
    } else {
        new_loop = &builder
                        .add_for_before(*parent, this->loop_, indvar, condition, init, update,
                                        this->loop_.debug_info())
                        .first;
    }
    builder.insert(child, this->loop_.root(), new_loop->root(), child.debug_info());

    // Replace indvar in new loop
    std::string new_indvar = builder.find_new_name(indvar->get_name());
    builder.add_container(new_indvar, sdfg.type(indvar->get_name()));
    new_loop->replace(indvar, symbolic::symbol(new_indvar));

    analysis_manager.invalidate_all();
};

void MyLoopDistribute::to_json(nlohmann::json& j) const {
    j["transformation_type"] = this->name();
    j["loop_element_id"] = this->loop_.element_id();
};

MyLoopDistribute MyLoopDistribute::from_json(builder::StructuredSDFGBuilder& builder,
                                             const nlohmann::json& desc) {
    auto loop_id = desc["loop_element_id"].get<size_t>();
    auto element = builder.find_element_by_id(loop_id);
    if (!element) {
        throw InvalidTransformationDescriptionException("Element with ID " +
                                                        std::to_string(loop_id) + " not found.");
    }
    auto loop = dynamic_cast<structured_control_flow::StructuredLoop*>(element);

    return MyLoopDistribute(*loop);
};

}  // namespace transformations
}  // namespace sdfg
