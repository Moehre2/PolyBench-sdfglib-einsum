#pragma once

#include <sdfg/analysis/analysis.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/data_flow/memlet.h>
#include <sdfg/symbolic/symbolic.h>

#include <nlohmann/json_fwd.hpp>
#include <string>

#include "sdfg/structured_control_flow/structured_loop.h"
#include "sdfg/transformations/transformation.h"

namespace sdfg {
namespace transformations {

class MyLoopDistribute : public Transformation {
    structured_control_flow::StructuredLoop& loop_;

    bool subset_contains(data_flow::Subset& subset, symbolic::Symbol& sym);

   public:
    MyLoopDistribute(structured_control_flow::StructuredLoop& loop);

    virtual std::string name() const override;

    virtual bool can_be_applied(builder::StructuredSDFGBuilder& builder,
                                analysis::AnalysisManager& analysis_manager) override;

    virtual void apply(builder::StructuredSDFGBuilder& builder,
                       analysis::AnalysisManager& analysis_manager) override;

    virtual void to_json(nlohmann::json& j) const override;

    static MyLoopDistribute from_json(builder::StructuredSDFGBuilder& builder,
                                      const nlohmann::json& j);
};

}  // namespace transformations
}  // namespace sdfg