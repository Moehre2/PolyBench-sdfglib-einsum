#pragma once

#include <sdfg/codegen/dispatchers/block_dispatcher.h>
#include <sdfg/codegen/dispatchers/node_dispatcher_registry.h>
#include <sdfg/codegen/language_extension.h>
#include <sdfg/codegen/utils.h>
#include <sdfg/data_flow/data_flow_graph.h>
#include <sdfg/data_flow/data_flow_node.h>
#include <sdfg/data_flow/library_node.h>
#include <sdfg/element.h>
#include <sdfg/function.h>
#include <sdfg/graph/graph.h>
#include <sdfg/symbolic/symbolic.h>

#include <cstddef>
#include <memory>
#include <string>

namespace sdfg {
namespace polybench {

inline data_flow::LibraryNodeCode LibraryNodeType_PolyBench("PolyBench");

enum PolyBenchNodeType { StartInstruments, StopAndPrintInstruments };

class PolyBenchNode : public data_flow::LibraryNode {
    const PolyBenchNodeType type_;

   public:
    PolyBenchNode(size_t element_id, const DebugInfo& debug_info, const graph::Vertex vertex,
                  data_flow::DataFlowGraph& parent, const PolyBenchNodeType type);

    PolyBenchNode(const PolyBenchNode&) = delete;
    PolyBenchNode& operator=(const PolyBenchNode&) = delete;

    virtual ~PolyBenchNode() = default;

    PolyBenchNodeType type() const;

    virtual std::unique_ptr<data_flow::DataFlowNode> clone(
        size_t element_id, const graph::Vertex vertex,
        data_flow::DataFlowGraph& parent) const override;

    virtual symbolic::SymbolSet symbols() const override;

    virtual void validate() const override;

    virtual void replace(const symbolic::Expression& old_expression,
                         const symbolic::Expression& new_expression) override;

    virtual std::string toStr() const override;
};

class PolyBenchDispatcher : public codegen::LibraryNodeDispatcher {
   public:
    PolyBenchDispatcher(codegen::LanguageExtension& language_extension, const Function& function,
                        const data_flow::DataFlowGraph& data_flow_graph,
                        const data_flow::LibraryNode& node);

    virtual void dispatch(codegen::PrettyPrinter& stream) override;
};

inline void register_polybench_dispatcher() {
    codegen::LibraryNodeDispatcherRegistry::instance().register_library_node_dispatcher(
        LibraryNodeType_PolyBench.value(),
        [](codegen::LanguageExtension& language_extension, const Function& function,
           const data_flow::DataFlowGraph& data_flow_graph, const data_flow::LibraryNode& node) {
            return std::make_unique<PolyBenchDispatcher>(language_extension, function,
                                                         data_flow_graph, node);
        });
}

}  // namespace polybench
}  // namespace sdfg
