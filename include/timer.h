#pragma once

#include <sdfg/analysis/analysis.h>
#include <sdfg/builder/structured_sdfg_builder.h>
#include <sdfg/passes/pass.h>

#include <string>

#include "benchmarks.h"

namespace sdfg {
namespace passes {

class PolyBenchTimerInstrumentation : public Pass {
    const CodeRegion& code_region_;

   public:
    PolyBenchTimerInstrumentation(const CodeRegion& code_region);

    virtual std::string name() override;

    virtual bool run_pass(builder::StructuredSDFGBuilder& builder,
                          analysis::AnalysisManager& analysis_manager) override;
};

}  // namespace passes
}  // namespace sdfg
