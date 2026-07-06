#include "analysis/control_flow/control_flow_graph.h"

#include <utility>

namespace fnfinder::analysis::control_flow {

ControlFlowGraph::ControlFlowGraph(Address entry,
                                   std::map<Address, BasicBlock> blocks,
                                   std::set<Address> coveredInstructions,
                                   Address extentEnd, bool truncated,
                                   bool reachesReturn)
    : entry_(entry),
      blocks_(std::move(blocks)),
      coveredInstructions_(std::move(coveredInstructions)),
      extentEnd_(extentEnd),
      truncated_(truncated),
      reachesReturn_(reachesReturn) {}

}
