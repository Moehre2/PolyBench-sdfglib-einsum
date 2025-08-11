#include "polybench_node.h"

#include <sdfg/codegen/dispatchers/block_dispatcher.h>
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

PolyBenchNode::PolyBenchNode(size_t element_id, const DebugInfo& debug_info,
                             const graph::Vertex vertex, data_flow::DataFlowGraph& parent,
                             const PolyBenchNodeType type)
    : data_flow::LibraryNode(element_id, debug_info, vertex, parent, LibraryNodeType_PolyBench, {},
                             {}, false),
      type_(type) {}

PolyBenchNodeType PolyBenchNode::type() const { return this->type_; }

std::unique_ptr<data_flow::DataFlowNode> PolyBenchNode::clone(
    size_t element_id, const graph::Vertex vertex, data_flow::DataFlowGraph& parent) const {
    return std::make_unique<PolyBenchNode>(element_id, this->debug_info(), vertex, parent,
                                           this->type());
}

symbolic::SymbolSet PolyBenchNode::symbols() const { return {}; }

void PolyBenchNode::validate() const {}

void PolyBenchNode::replace(const symbolic::Expression& old_expression,
                            const symbolic::Expression& new_expression) {}

std::string PolyBenchNode::toStr() const {
    switch (this->type()) {
        case StartInstruments:
            return "PolyBenchStartInstruments";
        case StopAndPrintInstruments:
            return "PolyBenchStopAndPrintInstruments";
    }
}

PolyBenchDispatcher::PolyBenchDispatcher(codegen::LanguageExtension& language_extension,
                                         const Function& function,
                                         const data_flow::DataFlowGraph& data_flow_graph,
                                         const data_flow::LibraryNode& node)
    : codegen::LibraryNodeDispatcher(language_extension, function, data_flow_graph, node) {}

void PolyBenchDispatcher::dispatch(codegen::PrettyPrinter& stream) {
    auto& polybench_node = dynamic_cast<const PolyBenchNode&>(this->node_);

    switch (polybench_node.type()) {
        case StartInstruments:
            stream << "polybench_start_instruments;" << std::endl;
            break;
        case StopAndPrintInstruments:
            stream << "polybench_stop_instruments;" << std::endl
                   << "polybench_print_instruments;" << std::endl;
            break;
    }
}

}  // namespace polybench
}  // namespace sdfg