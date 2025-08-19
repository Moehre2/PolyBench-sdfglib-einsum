#include "loop_consume_assignments.h"

#include <sdfg/analysis/analysis.h>
#include <sdfg/analysis/scope_analysis.h>
#include <sdfg/analysis/users.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/structured_control_flow/block.h>
#include <sdfg/structured_control_flow/sequence.h>
#include <sdfg/structured_control_flow/structured_loop.h>
#include <sdfg/symbolic/symbolic.h>
#include <sdfg/transformations/transformation.h>
#include <symengine/basic.h>

#include <cassert>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

namespace sdfg {
namespace transformations {

LoopConsumeAssignments::LoopConsumeAssignments(structured_control_flow::StructuredLoop& loop)
    : loop_(loop) {}

std::string LoopConsumeAssignments::name() const { return "LoopConsumeAssignments"; };

bool LoopConsumeAssignments::can_be_applied(builder::StructuredSDFGBuilder& builder,
                                            analysis::AnalysisManager& analysis_manager) {
    // Get the loop increment
    symbolic::Expression& update = this->loop_.update();
    if (update->get_type_code() != SymEngine::TypeID::SYMENGINE_ADD) return false;
    if (update->get_args().size() != 2) return false;
    symbolic::Expression inc;
    if (symbolic::eq(update->get_args().at(0), this->loop_.indvar())) {
        inc = update->get_args().at(1);
    } else if (symbolic::eq(update->get_args().at(1), this->loop_.indvar())) {
        inc = update->get_args().at(0);
    } else {
        return false;
    }

    // Get the symbols whose assignments depend on the increment of the loop
    std::unordered_map<symbolic::Symbol, size_t, SymEngine::RCPBasicHash, SymEngine::RCPBasicKeyEq>
        inc_symbols;
    auto& body = this->loop_.root();
    for (size_t i = 0; i < body.size(); ++i) {
        for (auto& assign : body.at(i).second.assignments()) {
            if (assign.second->get_type_code() == SymEngine::TypeID::SYMENGINE_ADD &&
                assign.second->get_args().size() == 2 &&
                ((symbolic::eq(assign.second->get_args().at(0), assign.first) &&
                  symbolic::eq(assign.second->get_args().at(1), inc)) ||
                 (symbolic::eq(assign.second->get_args().at(1), assign.first) &&
                  symbolic::eq(assign.second->get_args().at(0), inc))))
                inc_symbols.insert({assign.first, body.at(i).second.element_id()});
        }
    }
    if (inc_symbols.size() == 0) return false;

    // Perform users analysis
    auto& users = analysis_manager.get<analysis::Users>();
    analysis::UsersView users_view(users, this->loop_);

    // Check that each symbol is only write accessed once in the loop
    symbolic::SymbolSet inc_symbols2;
    for (auto& inc_sym : inc_symbols) {
        bool transfer = true;
        for (auto* user : users_view.uses(inc_sym.first->__str__())) {
            if (user && user->use() == analysis::Use::WRITE && user->element() &&
                user->element()->element_id() != inc_sym.second) {
                transfer = false;
                break;
            }
        }
        if (transfer) inc_symbols2.insert(inc_sym.first);
    }
    if (inc_symbols2.size() == 0) return false;

    // Get the parent sequence of the loop
    auto& scope = analysis_manager.get<analysis::ScopeAnalysis>();
    auto* parent =
        static_cast<structured_control_flow::Sequence*>(scope.parent_scope(&this->loop_));
    if (!parent) return false;

    // Determine the index of the loop in the parent sequence
    size_t parent_loop_index;
    for (parent_loop_index = 0; parent_loop_index < parent->size(); ++parent_loop_index) {
        if (parent->at(parent_loop_index).first.element_id() == this->loop_.element_id()) break;
    }
    if (parent_loop_index >= parent->size() || parent_loop_index == 0) return false;

    // Determine the init to all previously determined symbols
    symbolic::SymbolSet symbols;
    for (auto& sym : inc_symbols2) {
        for (long long i = parent_loop_index - 1; i >= 0; --i) {
            for (auto& assign : parent->at(i).second.assignments()) {
                if (symbolic::eq(assign.first, sym)) symbols.insert(sym);
            }
        }
    }
    if (symbols.size() == 0) return false;

    return true;
}

void LoopConsumeAssignments::apply(builder::StructuredSDFGBuilder& builder,
                                   analysis::AnalysisManager& analysis_manager) {
    // Get the loop increment
    symbolic::Expression& update = this->loop_.update();
    symbolic::Expression inc;
    if (symbolic::eq(update->get_args().at(0), this->loop_.indvar())) {
        inc = update->get_args().at(1);
    } else {
        inc = update->get_args().at(0);
    }

    // Get the symbols whose assignments depend on the increment of the loop
    std::unordered_map<symbolic::Symbol, size_t, SymEngine::RCPBasicHash, SymEngine::RCPBasicKeyEq>
        inc_symbols;
    auto& body = this->loop_.root();
    for (size_t i = 0; i < body.size(); ++i) {
        for (auto& assign : body.at(i).second.assignments()) {
            if (assign.second->get_type_code() == SymEngine::TypeID::SYMENGINE_ADD &&
                assign.second->get_args().size() == 2 &&
                ((symbolic::eq(assign.second->get_args().at(0), assign.first) &&
                  symbolic::eq(assign.second->get_args().at(1), inc)) ||
                 (symbolic::eq(assign.second->get_args().at(1), assign.first) &&
                  symbolic::eq(assign.second->get_args().at(0), inc))))
                inc_symbols.insert({assign.first, body.at(i).second.element_id()});
        }
    }

    // Perform users analysis
    auto& users = analysis_manager.get<analysis::Users>();
    analysis::UsersView users_view(users, this->loop_);

    // Check that each symbol is only write accessed once in the loop
    symbolic::SymbolSet inc_symbols2;
    for (auto& inc_sym : inc_symbols) {
        bool transfer = true;
        for (auto* user : users_view.uses(inc_sym.first->__str__())) {
            if (user && user->use() == analysis::Use::WRITE && user->element() &&
                user->element()->element_id() != inc_sym.second) {
                transfer = false;
                break;
            }
        }
        if (transfer) inc_symbols2.insert(inc_sym.first);
    }

    // Get the parent sequence of the loop
    auto& scope = analysis_manager.get<analysis::ScopeAnalysis>();
    auto* parent =
        static_cast<structured_control_flow::Sequence*>(scope.parent_scope(&this->loop_));

    // Determine the index of the loop in the parent sequence
    size_t parent_loop_index;
    for (parent_loop_index = 0; parent_loop_index < parent->size(); ++parent_loop_index) {
        if (parent->at(parent_loop_index).first.element_id() == this->loop_.element_id()) break;
    }

    // Determine the init to all previously determined symbols
    symbolic::SymbolMap symbols;
    for (auto& inc_sym : inc_symbols2) {
        bool found = false;
        for (long long i = parent_loop_index - 1; i >= 0; --i) {
            for (auto& assign : parent->at(i).second.assignments()) {
                if (symbolic::eq(assign.first, inc_sym)) {
                    symbols.insert({inc_sym, assign.second});
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }

    // For every symbols ...
    for (auto& sym : symbols) {
        // ... delete init occurrence
        for (long long i = parent_loop_index - 1; i >= 0; --i) {
            if (parent->at(i).second.assignments().contains(sym.first)) {
                parent->at(i).second.assignments().erase(sym.first);
                break;
            }
        }
        // ... delete update occurrence
        for (size_t i = 0; i < body.size(); ++i) {
            if (body.at(i).second.assignments().contains(sym.first)) {
                body.at(i).second.assignments().erase(sym.first);
                break;
            }
        }
        // ... replace all occurrences with expression dependent on index variable of loop
        symbolic::Expression new_expr =
            symbolic::sub(symbolic::add(this->loop_.indvar(), sym.second), this->loop_.init());
        body.replace(sym.first, new_expr);
    }

    analysis_manager.invalidate_all();
}

void LoopConsumeAssignments::to_json(nlohmann::json& j) const {
    j["transformation_type"] = this->name();
    j["loop_element_id"] = this->loop_.element_id();
}

LoopConsumeAssignments LoopConsumeAssignments::from_json(builder::StructuredSDFGBuilder& builder,
                                                         const nlohmann::json& desc) {
    auto loop_id = desc["loop_element_id"].get<size_t>();
    auto element = builder.find_element_by_id(loop_id);
    if (!element) {
        throw InvalidTransformationDescriptionException("Element with ID " +
                                                        std::to_string(loop_id) + " not found.");
    }
    auto loop = dynamic_cast<structured_control_flow::StructuredLoop*>(element);

    return LoopConsumeAssignments(*loop);
}

}  // namespace transformations
}  // namespace sdfg
