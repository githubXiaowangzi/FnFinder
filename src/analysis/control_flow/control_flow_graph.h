#ifndef FNFINDER_ANALYSIS_CONTROL_FLOW_CONTROL_FLOW_GRAPH_H_
#define FNFINDER_ANALYSIS_CONTROL_FLOW_CONTROL_FLOW_GRAPH_H_

#include <cstddef>
#include <map>
#include <set>

#include "analysis/control_flow/basic_block.h"
#include "common/types.h"

namespace fnfinder::analysis::control_flow {

class ControlFlowGraph {
 public:
  ControlFlowGraph() = default;

  ControlFlowGraph(Address entry, std::map<Address, BasicBlock> blocks,
                   std::set<Address> coveredInstructions, Address extentEnd,
                   bool truncated, bool reachesReturn);

  Address entry() const { return entry_; }

  const std::map<Address, BasicBlock>& blocks() const { return blocks_; }

  const std::set<Address>& coveredInstructions() const {
    return coveredInstructions_;
  }

  Address extentEnd() const { return extentEnd_; }

  std::size_t instructionCount() const { return coveredInstructions_.size(); }

  std::size_t blockCount() const { return blocks_.size(); }

  bool empty() const { return coveredInstructions_.empty(); }

  bool contains(Address addr) const {
    return coveredInstructions_.find(addr) != coveredInstructions_.end();
  }

  bool truncated() const { return truncated_; }

  bool reachesReturn() const { return reachesReturn_; }

 private:
  Address entry_ = 0;
  std::map<Address, BasicBlock> blocks_;
  std::set<Address> coveredInstructions_;
  Address extentEnd_ = 0;
  bool truncated_ = false;
  bool reachesReturn_ = false;
};

}

#endif
