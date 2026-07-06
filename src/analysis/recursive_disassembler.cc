#include "analysis/recursive_disassembler.h"

#include <unordered_set>

#include "common/logger.h"
#include "disasm/instruction.h"

namespace fnfinder::analysis {

RecursiveDisassembler::RecursiveDisassembler(const CodeMap& codeMap,
                                             disasm::IDisassembler& disassembler,
                                             std::size_t maxInstructions)
    : codeMap_(codeMap),
      disassembler_(disassembler),
      maxInstructions_(maxInstructions) {}

RecursiveResult RecursiveDisassembler::discover(const std::vector<Address>& seeds) const {
  RecursiveResult result;

  std::unordered_set<Address> scheduled;
  std::vector<Address> worklist;

  const auto schedule = [&](Address addr) {
    if ((addr % kAArch64InstructionSize) != 0) {
      return;
    }
    if (!codeMap_.contains(addr)) {
      return;
    }
    if (scheduled.insert(addr).second) {
      worklist.push_back(addr);
    }
  };

  for (Address seed : seeds) {
    if (codeMap_.contains(seed)) {
      result.functionStarts.insert(seed);
    }
    schedule(seed);
  }

  while (!worklist.empty()) {
    if (result.instructionSizes.size() >= maxInstructions_) {
      logWarn("recursive traversal hit the instruction cap; results may be partial");
      break;
    }
    const Address addr = worklist.back();
    worklist.pop_back();

    std::size_t available = 0;
    const std::uint8_t* code = codeMap_.at(addr, available);
    if (code == nullptr) {
      continue;
    }
    disasm::Instruction insn;
    if (!disassembler_.decode(code, available, addr, insn)) {
      continue;
    }
    result.instructionSizes[addr] = insn.size;
    const Address fallThrough = addr + insn.size;

    switch (insn.flow) {
      case disasm::FlowType::kDirectCall:
        if (insn.hasTarget) {
          if (codeMap_.contains(insn.target)) {
            result.functionStarts.insert(insn.target);
          }
          schedule(insn.target);
        }
        schedule(fallThrough);
        break;
      case disasm::FlowType::kIndirectCall:
        schedule(fallThrough);
        break;
      case disasm::FlowType::kConditionalBranch:
        if (insn.hasTarget) {
          schedule(insn.target);
        }
        schedule(fallThrough);
        break;
      case disasm::FlowType::kUnconditionalBranch:
        if (insn.hasTarget) {
          schedule(insn.target);
        }
        break;
      case disasm::FlowType::kIndirectBranch:
      case disasm::FlowType::kReturn:
        break;
      case disasm::FlowType::kSequential:
        schedule(fallThrough);
        break;
    }
  }

  logInfo("recursive traversal: decoded " +
          std::to_string(result.instructionSizes.size()) + " instruction(s), " +
          std::to_string(result.functionStarts.size()) + " function entr(y/ies)");
  return result;
}

}
