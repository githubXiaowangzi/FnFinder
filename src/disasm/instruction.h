#ifndef FNFINDER_DISASM_INSTRUCTION_H_
#define FNFINDER_DISASM_INSTRUCTION_H_

#include <cstdint>
#include <string>

#include "common/types.h"

namespace fnfinder::disasm {

enum class FlowType {
  kSequential,
  kConditionalBranch,
  kUnconditionalBranch,
  kDirectCall,
  kIndirectBranch,
  kIndirectCall,
  kReturn,
};

struct Instruction {
  Address address = 0;
  std::uint32_t size = kAArch64InstructionSize;
  std::string mnemonic;
  std::string operands;

  FlowType flow = FlowType::kSequential;

  bool hasTarget = false;
  Address target = 0;

  Address endAddress() const { return address + size; }

  bool isCall() const {
    return flow == FlowType::kDirectCall || flow == FlowType::kIndirectCall;
  }

  bool hasFallThrough() const {
    switch (flow) {
      case FlowType::kSequential:
      case FlowType::kConditionalBranch:
      case FlowType::kDirectCall:
      case FlowType::kIndirectCall:
        return true;
      case FlowType::kUnconditionalBranch:
      case FlowType::kIndirectBranch:
      case FlowType::kReturn:
        return false;
    }
    return true;
  }

  bool isTerminator() const {
    return flow == FlowType::kUnconditionalBranch ||
           flow == FlowType::kIndirectBranch || flow == FlowType::kReturn;
  }
};

}

#endif
