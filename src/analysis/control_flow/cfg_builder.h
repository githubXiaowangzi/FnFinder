#ifndef FNFINDER_ANALYSIS_CONTROL_FLOW_CFG_BUILDER_H_
#define FNFINDER_ANALYSIS_CONTROL_FLOW_CFG_BUILDER_H_

#include <cstddef>
#include <set>

#include "analysis/control_flow/control_flow_graph.h"
#include "analysis/control_flow/instruction_cache.h"
#include "common/types.h"

namespace fnfinder::analysis::control_flow {

struct CfgBuildRequest {
  Address entry = 0;
  Address upperBound = 0;
  const std::set<Address>* boundaries = nullptr;
  std::size_t maxInstructions = 8'000'000;
  bool linearPastIndirectBranch = true;
  std::set<Address>* callTargetsOut = nullptr;
  bool buildBasicBlocks = false;
};

class CfgBuilder {
 public:
  explicit CfgBuilder(InstructionCache& cache);

  ControlFlowGraph build(const CfgBuildRequest& request) const;

 private:
  InstructionCache& cache_;
};

}

#endif
