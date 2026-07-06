#ifndef FNFINDER_ANALYSIS_CONTROL_FLOW_BASIC_BLOCK_H_
#define FNFINDER_ANALYSIS_CONTROL_FLOW_BASIC_BLOCK_H_

#include <cstdint>
#include <vector>

#include "common/types.h"
#include "disasm/instruction.h"

namespace fnfinder::analysis::control_flow {

enum class EdgeKind {
  kFallThrough,
  kBranchTaken,
  kCallReturn,
};

struct SuccessorEdge {
  Address target = 0;
  EdgeKind kind = EdgeKind::kFallThrough;
};

struct BasicBlock {
  Address start = 0;
  Address end = 0;
  disasm::FlowType terminatorFlow = disasm::FlowType::kSequential;
  std::vector<SuccessorEdge> successors;

  Address size() const { return end - start; }
  bool contains(Address addr) const { return addr >= start && addr < end; }
};

}

#endif
