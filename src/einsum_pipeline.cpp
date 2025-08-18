#include "einsum_pipeline.h"

#include <sdfg/analysis/analysis.h>
#include <sdfg/analysis/loop_analysis.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/codegen/utils.h>
#include <sdfg/einsum/einsum_node.h>
#include <sdfg/passes/pass.h>
#include <sdfg/passes/structured_control_flow/block_fusion.h>
#include <sdfg/passes/structured_control_flow/loop_normalization.h>
#include <sdfg/structured_control_flow/block.h>
#include <sdfg/structured_control_flow/control_flow_node.h>
#include <sdfg/structured_control_flow/if_else.h>
#include <sdfg/structured_control_flow/return.h>
#include <sdfg/structured_control_flow/sequence.h>
#include <sdfg/structured_control_flow/structured_loop.h>
#include <sdfg/structured_control_flow/while.h>
#include <sdfg/transformations/einsum2blas.h>
#include <sdfg/transformations/einsum_expand.h>
#include <sdfg/transformations/einsum_lift.h>
#include <sdfg/transformations/loop_distribute.h>

#include <functional>
#include <iostream>
#include <list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "my_loop_distribute.h"

namespace sdfg {

void dump_sdfg(codegen::PrettyPrinter& stream, structured_control_flow::ControlFlowNode* node) {
    if (auto* loop = dynamic_cast<structured_control_flow::StructuredLoop*>(node)) {
        stream << "StructuredLoop:" << std::endl;
        stream.setIndent(stream.indent() + 2);
        dump_sdfg(stream, &loop->root());
        stream.setIndent(stream.indent() - 2);
    } else if (dynamic_cast<structured_control_flow::Block*>(node)) {
        stream << "Block" << std::endl;
    } else if (auto* sequence = dynamic_cast<structured_control_flow::Sequence*>(node)) {
        stream << "Sequence:" << std::endl;
        stream.setIndent(stream.indent() + 2);
        for (size_t i = 0; i < sequence->size(); ++i) {
            dump_sdfg(stream, &sequence->at(i).first);
        }
        stream.setIndent(stream.indent() - 2);
    } else if (auto* if_else = dynamic_cast<structured_control_flow::IfElse*>(node)) {
        stream << "IfElse:" << std::endl;
        stream.setIndent(stream.indent() + 2);
        for (size_t i = 0; i < if_else->size(); ++i) {
            dump_sdfg(stream, &if_else->at(i).first);
        }
        stream.setIndent(stream.indent() - 2);
    } else if (auto* while_loop = dynamic_cast<structured_control_flow::While*>(node)) {
        stream << "While:" << std::endl;
        stream.setIndent(stream.indent() + 2);
        dump_sdfg(stream, &while_loop->root());
        stream.setIndent(stream.indent() - 2);
    } else if (dynamic_cast<structured_control_flow::Break*>(node)) {
        stream << "Break" << std::endl;
    } else if (dynamic_cast<structured_control_flow::Continue*>(node)) {
        stream << "Continue" << std::endl;
    } else if (dynamic_cast<structured_control_flow::Return*>(node)) {
        stream << "Return" << std::endl;
    } else {
        throw std::runtime_error("Unsupported control flow node type");
    }
}

namespace passes {

std::vector<std::pair<std::vector<std::reference_wrapper<structured_control_flow::StructuredLoop>>,
                      structured_control_flow::Block&>>
EinsumPipeline::get_einsum_loops(builder::StructuredSDFGBuilder& builder) {
    std::vector<
        std::pair<std::vector<std::reference_wrapper<structured_control_flow::StructuredLoop>>,
                  structured_control_flow::Block&>>
        result;
    std::list<structured_control_flow::ControlFlowNode*> queue = {&builder.subject().root()};
    while (!queue.empty()) {
        auto* current = queue.front();
        queue.pop_front();

        if (auto* loop = dynamic_cast<structured_control_flow::StructuredLoop*>(current)) {
            std::vector<std::reference_wrapper<structured_control_flow::StructuredLoop>> loop_nest;
            structured_control_flow::StructuredLoop* current_loop = loop;
            bool success = true;
            while (success) {
                success = false;
                if (current_loop->root().size() != 1) break;
                loop_nest.push_back(*current_loop);
                if (auto* next_loop = dynamic_cast<structured_control_flow::StructuredLoop*>(
                        &current_loop->root().at(0).first)) {
                    current_loop = next_loop;
                    success = true;
                } else if (auto* block = dynamic_cast<structured_control_flow::Block*>(
                               &current_loop->root().at(0).first)) {
                    result.push_back({loop_nest, *block});
                    success = true;
                    break;
                }
            }
            for (size_t i = 0; i < loop->root().size(); ++i) {
                queue.push_back(&loop->root().at(i).first);
            }
        } else if (auto* block = dynamic_cast<structured_control_flow::Block*>(current)) {
            result.push_back({{}, *block});
        } else if (auto* sequence = dynamic_cast<structured_control_flow::Sequence*>(current)) {
            for (size_t i = 0; i < sequence->size(); ++i) {
                queue.push_back(&sequence->at(i).first);
            }
        } else if (auto* if_else = dynamic_cast<structured_control_flow::IfElse*>(current)) {
            for (size_t i = 0; i < if_else->size(); ++i) {
                queue.push_back(&if_else->at(i).first);
            }
        } else if (auto* while_loop = dynamic_cast<structured_control_flow::While*>(current)) {
            for (size_t i = 0; i < while_loop->root().size(); ++i) {
                queue.push_back(&while_loop->root().at(i).first);
            }
        } else if (dynamic_cast<structured_control_flow::Break*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Continue*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Return*>(current)) {
            continue;
        } else {
            throw std::runtime_error("Unsupported control flow node type");
        }
    }
    return result;
}

std::vector<std::pair<structured_control_flow::StructuredLoop&, einsum::EinsumNode&>>
EinsumPipeline::get_einsum_node_loops(builder::StructuredSDFGBuilder& builder) {
    std::vector<std::pair<structured_control_flow::StructuredLoop&, einsum::EinsumNode&>> result;

    std::list<structured_control_flow::ControlFlowNode*> queue = {&builder.subject().root()};
    while (!queue.empty()) {
        auto* current = queue.front();
        queue.pop_front();

        if (auto* loop = dynamic_cast<structured_control_flow::StructuredLoop*>(current)) {
            for (size_t i = 0; i < loop->root().size(); ++i) {
                if (auto* block =
                        dynamic_cast<structured_control_flow::Block*>(&loop->root().at(i).first)) {
                    for (auto& node : block->dataflow().nodes()) {
                        if (auto* einsum_node = dynamic_cast<einsum::EinsumNode*>(&node))
                            result.push_back({*loop, *einsum_node});
                    }
                } else {
                    queue.push_back(&loop->root().at(i).first);
                }
            }
        } else if (dynamic_cast<structured_control_flow::Block*>(current)) {
            continue;
        } else if (auto* sequence = dynamic_cast<structured_control_flow::Sequence*>(current)) {
            for (size_t i = 0; i < sequence->size(); ++i) {
                queue.push_back(&sequence->at(i).first);
            }
        } else if (auto* if_else = dynamic_cast<structured_control_flow::IfElse*>(current)) {
            for (size_t i = 0; i < if_else->size(); ++i) {
                queue.push_back(&if_else->at(i).first);
            }
        } else if (auto* while_loop = dynamic_cast<structured_control_flow::While*>(current)) {
            for (size_t i = 0; i < while_loop->root().size(); ++i) {
                queue.push_back(&while_loop->root().at(i).first);
            }
        } else if (dynamic_cast<structured_control_flow::Break*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Continue*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Return*>(current)) {
            continue;
        } else {
            throw std::runtime_error("Unsupported control flow node type");
        }
    }

    return result;
}

std::vector<std::reference_wrapper<einsum::EinsumNode>> EinsumPipeline::get_einsum_nodes(
    builder::StructuredSDFGBuilder& builder) {
    std::vector<std::reference_wrapper<einsum::EinsumNode>> result;

    std::list<structured_control_flow::ControlFlowNode*> queue = {&builder.subject().root()};
    while (!queue.empty()) {
        auto* current = queue.front();
        queue.pop_front();

        if (auto* loop = dynamic_cast<structured_control_flow::StructuredLoop*>(current)) {
            for (size_t i = 0; i < loop->root().size(); ++i) {
                queue.push_back(&loop->root().at(i).first);
            }
        } else if (auto* block = dynamic_cast<structured_control_flow::Block*>(current)) {
            for (auto& node : block->dataflow().nodes()) {
                if (auto* einsum_node = dynamic_cast<einsum::EinsumNode*>(&node))
                    result.push_back(*einsum_node);
            }
        } else if (auto* sequence = dynamic_cast<structured_control_flow::Sequence*>(current)) {
            for (size_t i = 0; i < sequence->size(); ++i) {
                queue.push_back(&sequence->at(i).first);
            }
        } else if (auto* if_else = dynamic_cast<structured_control_flow::IfElse*>(current)) {
            for (size_t i = 0; i < if_else->size(); ++i) {
                queue.push_back(&if_else->at(i).first);
            }
        } else if (auto* while_loop = dynamic_cast<structured_control_flow::While*>(current)) {
            for (size_t i = 0; i < while_loop->root().size(); ++i) {
                queue.push_back(&while_loop->root().at(i).first);
            }
        } else if (dynamic_cast<structured_control_flow::Break*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Continue*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Return*>(current)) {
            continue;
        } else {
            throw std::runtime_error("Unsupported control flow node type");
        }
    }

    return result;
}

void EinsumPipeline::block_fusion(builder::StructuredSDFGBuilder& builder,
                                  analysis::AnalysisManager& analysis_manager,
                                  structured_control_flow::Sequence& parent,
                                  structured_control_flow::Sequence& node) {
    BlockFusion block_fusion(builder, analysis_manager);
    if (block_fusion.accept(parent, node)) std::cout << "Applied BlockFusion" << std::endl;

    std::list<structured_control_flow::ControlFlowNode*> queue;
    for (size_t i = 0; i < node.size(); ++i) queue.push_back(&node.at(i).first);
    while (!queue.empty()) {
        auto* current = queue.front();
        queue.pop_front();

        if (auto* loop = dynamic_cast<structured_control_flow::StructuredLoop*>(current)) {
            this->block_fusion(builder, analysis_manager, node, loop->root());
        } else if (dynamic_cast<structured_control_flow::Block*>(current)) {
            continue;
        } else if (auto* sequence = dynamic_cast<structured_control_flow::Sequence*>(current)) {
            this->block_fusion(builder, analysis_manager, node, *sequence);
        } else if (auto* if_else = dynamic_cast<structured_control_flow::IfElse*>(current)) {
            for (size_t i = 0; i < if_else->size(); ++i) {
                queue.push_back(&if_else->at(i).first);
            }
        } else if (auto* while_loop = dynamic_cast<structured_control_flow::While*>(current)) {
            this->block_fusion(builder, analysis_manager, node, while_loop->root());
        } else if (dynamic_cast<structured_control_flow::Break*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Continue*>(current)) {
            continue;
        } else if (dynamic_cast<structured_control_flow::Return*>(current)) {
            continue;
        } else {
            throw std::runtime_error("Unsupported control flow node type");
        }
    }
}

EinsumPipeline::EinsumPipeline() : Pass() {}

std::string EinsumPipeline::name() { return "EinsumPipeline"; }

bool EinsumPipeline::run_pass(builder::StructuredSDFGBuilder& builder,
                              analysis::AnalysisManager& analysis_manager) {
    bool applied;

    // LoopNormalization
    LoopNormalization loop_normalization;
    if (loop_normalization.run(builder, analysis_manager))
        std::cout << "Applied LoopNormalization" << std::endl;

    // LoopDistribute & MyLoopDistribute
    do {
        applied = false;
        auto& loop_analysis = analysis_manager.get<analysis::LoopAnalysis>();
        for (auto* node : loop_analysis.loops()) {
            if (auto* loop = dynamic_cast<structured_control_flow::StructuredLoop*>(node)) {
                transformations::LoopDistribute transformation(*loop);
                if (transformation.can_be_applied(builder, analysis_manager)) {
                    transformation.apply(builder, analysis_manager);
                    std::cout << "Applied LoopDistribute" << std::endl;
                    applied = true;
                    break;
                }
                transformations::MyLoopDistribute my_transformation(*loop);
                if (my_transformation.can_be_applied(builder, analysis_manager)) {
                    my_transformation.apply(builder, analysis_manager);
                    std::cout << "Applied MyLoopDistribute" << std::endl;
                    applied = true;
                    break;
                }
            }
        }
    } while (applied);

    // BlockFusion
    this->block_fusion(builder, analysis_manager, builder.subject().root(),
                       builder.subject().root());

    // EinsumLift
    do {
        applied = false;
        auto einsum_loops = this->get_einsum_loops(builder);
        for (auto& einsum_loop : einsum_loops) {
            transformations::EinsumLift transformation(einsum_loop.first, einsum_loop.second);
            if (transformation.can_be_applied(builder, analysis_manager)) {
                transformation.apply(builder, analysis_manager);
                std::cout << "Applied EinsumLift" << std::endl;
                applied = true;
                break;
            }
        }
    } while (applied);

    // EinsumExpand
    do {
        applied = false;
        auto einsum_node_loops = this->get_einsum_node_loops(builder);
        for (auto& einsum_node_loop : einsum_node_loops) {
            transformations::EinsumExpand transformation(einsum_node_loop.first,
                                                         einsum_node_loop.second);
            if (transformation.can_be_applied(builder, analysis_manager)) {
                transformation.apply(builder, analysis_manager);
                std::cout << "Applied EinsumExpand" << std::endl;
                applied = true;
                break;
            }
        }
    } while (applied);

    // Einsum2BLAS
    do {
        applied = false;
        auto einsum_nodes = this->get_einsum_nodes(builder);
        for (auto einsum_node : einsum_nodes) {
            transformations::Einsum2BLAS transformation(einsum_node.get());
            if (transformation.can_be_applied(builder, analysis_manager)) {
                transformation.apply(builder, analysis_manager);
                std::cout << "Applied Einsum2BLAS" << std::endl;
                applied = true;
                break;
            }
        }
    } while (applied);

    // codegen::PrettyPrinter stream;
    // dump_sdfg(stream, &builder.subject().root());
    // std::cout << stream.str() << std::endl;

    return true;
}

}  // namespace passes
}  // namespace sdfg
