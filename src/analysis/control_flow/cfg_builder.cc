#include "analysis/control_flow/cfg_builder.h"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

#include "analysis/control_flow/code_patterns.h"
#include "disasm/instruction.h"

namespace fnfinder::analysis::control_flow {
namespace {

void appendSuccessors(std::vector<SuccessorEdge>& out,
                      const DecodedInstruction& insn,
                      const std::set<Address>& covered) {
  const Address fallThrough = insn.endAddress();
  const auto addIfCovered = [&](Address target, EdgeKind kind) {
    if (covered.find(target) != covered.end()) {
      out.push_back(SuccessorEdge{target, kind});
    }
  };

  switch (insn.flow) {
    case disasm::FlowType::kSequential:
      addIfCovered(fallThrough, EdgeKind::kFallThrough);
      break;
    case disasm::FlowType::kDirectCall:
    case disasm::FlowType::kIndirectCall:
      addIfCovered(fallThrough, EdgeKind::kCallReturn);
      break;
    case disasm::FlowType::kConditionalBranch:
      if (insn.hasTarget) {
        addIfCovered(insn.target, EdgeKind::kBranchTaken);
      }
      addIfCovered(fallThrough, EdgeKind::kFallThrough);
      break;
    case disasm::FlowType::kUnconditionalBranch:
      if (insn.hasTarget) {
        addIfCovered(insn.target, EdgeKind::kBranchTaken);
      }
      break;
    case disasm::FlowType::kIndirectBranch:
    case disasm::FlowType::kReturn:
      break;
  }
}

std::map<Address, BasicBlock> buildBlocks(const std::set<Address>& covered,
                                          Address entry,
                                          InstructionCache& cache) {
  std::map<Address, BasicBlock> blocks;
  if (covered.empty()) {
    return blocks;
  }

  std::set<Address> leaders;
  leaders.insert(entry);
  for (Address a : covered) {
    const DecodedInstruction* insn = cache.at(a);
    if (insn == nullptr) {
      continue;
    }
    if (insn->isBlockTerminator()) {
      const Address fallThrough = insn->endAddress();
      if (covered.find(fallThrough) != covered.end()) {
        leaders.insert(fallThrough);
      }
      const bool isBranch =
          insn->flow == disasm::FlowType::kConditionalBranch ||
          insn->flow == disasm::FlowType::kUnconditionalBranch;
      if (isBranch && insn->hasTarget &&
          covered.find(insn->target) != covered.end()) {
        leaders.insert(insn->target);
      }
    }
  }

  BasicBlock* current = nullptr;
  Address previousEnd = 0;
  for (Address a : covered) {
    const DecodedInstruction* insn = cache.at(a);
    if (insn == nullptr) {
      continue;
    }
    const bool discontinuous = (current != nullptr) && (a != previousEnd);
    const bool isLeader = leaders.find(a) != leaders.end();
    if (current == nullptr || isLeader || discontinuous) {
      BasicBlock block;
      block.start = a;
      current = &blocks.emplace(a, block).first->second;
    }
    current->end = insn->endAddress();
    current->terminatorFlow = insn->flow;
    current->successors.clear();
    appendSuccessors(current->successors, *insn, covered);
    previousEnd = insn->endAddress();
  }

  return blocks;
}

}

CfgBuilder::CfgBuilder(InstructionCache& cache) : cache_(cache) {}

ControlFlowGraph CfgBuilder::build(const CfgBuildRequest& request) const {
  const Address entry = request.entry;
  const Address upperBound = request.upperBound;

  std::set<Address> visited;
  std::unordered_set<Address> enqueued;
  std::vector<Address> worklist;

  Address coveredEnd = entry;
  bool truncated = false;
  bool reachesReturn = false;

  const auto isSchedulable = [&](Address target) {
    if (target < entry || target >= upperBound) {
      return false;
    }
    if ((target % kAArch64InstructionSize) != 0) {
      return false;
    }
    if (!cache_.isExecutable(target)) {
      return false;
    }
    if (request.boundaries != nullptr &&
        request.boundaries->find(target) != request.boundaries->end()) {
      return false;
    }
    return true;
  };
  const auto schedule = [&](Address target) {
    if (!isSchedulable(target)) {
      return;
    }
    if (enqueued.insert(target).second) {
      worklist.push_back(target);
    }
  };

  if ((entry % kAArch64InstructionSize) == 0 && entry < upperBound &&
      cache_.isExecutable(entry)) {
    enqueued.insert(entry);
    worklist.push_back(entry);
  }

  while (!worklist.empty()) {
    if (visited.size() >= request.maxInstructions) {
      truncated = true;
      break;
    }
    const Address a = worklist.back();
    worklist.pop_back();
    if (visited.find(a) != visited.end()) {
      continue;
    }

    if (isHardStopWord(cache_.codeMap(), a)) {
      continue;
    }

    const DecodedInstruction* insn = cache_.at(a);
    if (insn == nullptr) {
      continue;
    }
    if (a + insn->size > upperBound) {
      continue;
    }

    visited.insert(a);
    coveredEnd = std::max(coveredEnd, insn->endAddress());
    const Address fallThrough = insn->endAddress();

    switch (insn->flow) {
      case disasm::FlowType::kSequential:
        schedule(fallThrough);
        break;
      case disasm::FlowType::kDirectCall:
        if (insn->hasTarget && request.callTargetsOut != nullptr) {
          request.callTargetsOut->insert(insn->target);
        }
        schedule(fallThrough);
        break;
      case disasm::FlowType::kIndirectCall:
        schedule(fallThrough);
        break;
      case disasm::FlowType::kConditionalBranch:
        if (insn->hasTarget) {
          schedule(insn->target);
        }
        schedule(fallThrough);
        break;
      case disasm::FlowType::kUnconditionalBranch:
        if (insn->hasTarget) {
          schedule(insn->target);
        }
        break;
      case disasm::FlowType::kIndirectBranch:
        if (request.linearPastIndirectBranch) {
          schedule(fallThrough);
        }
        break;
      case disasm::FlowType::kReturn:
        reachesReturn = true;
        break;
    }
  }

  std::map<Address, BasicBlock> blocks;
  if (request.buildBasicBlocks) {
    blocks = buildBlocks(visited, entry, cache_);
  }

  return ControlFlowGraph(entry, std::move(blocks), std::move(visited),
                          coveredEnd, truncated, reachesReturn);
}

}
