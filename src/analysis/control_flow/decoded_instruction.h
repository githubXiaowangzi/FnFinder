#ifndef FNFINDER_ANALYSIS_CONTROL_FLOW_DECODED_INSTRUCTION_H_
#define FNFINDER_ANALYSIS_CONTROL_FLOW_DECODED_INSTRUCTION_H_

#include <cstdint>

#include "common/types.h"
#include "disasm/instruction.h"

namespace fnfinder::analysis::control_flow {

struct DecodedInstruction {
  Address address = 0;
  std::uint32_t size = kAArch64InstructionSize;
  disasm::FlowType flow = disasm::FlowType::kSequential;
  bool hasTarget = false;
  Address target = 0;

  Address endAddress() const { return address + size; }

  bool isReturn() const { return flow == disasm::FlowType::kReturn; }

  bool isIndirectBranch() const {
    return flow == disasm::FlowType::kIndirectBranch;
  }

  bool isCall() const {
    return flow == disasm::FlowType::kDirectCall ||
           flow == disasm::FlowType::kIndirectCall;
  }

  bool isDirectCall() const { return flow == disasm::FlowType::kDirectCall; }

  bool hasFallThrough() const {
    switch (flow) {
      case disasm::FlowType::kSequential:
      case disasm::FlowType::kConditionalBranch:
      case disasm::FlowType::kDirectCall:
      case disasm::FlowType::kIndirectCall:
        return true;
      case disasm::FlowType::kUnconditionalBranch:
      case disasm::FlowType::kIndirectBranch:
      case disasm::FlowType::kReturn:
        return false;
    }
    return true;
  }

  bool isBlockTerminator() const {
    switch (flow) {
      case disasm::FlowType::kConditionalBranch:
      case disasm::FlowType::kUnconditionalBranch:
      case disasm::FlowType::kIndirectBranch:
      case disasm::FlowType::kReturn:
        return true;
      case disasm::FlowType::kSequential:
      case disasm::FlowType::kDirectCall:
      case disasm::FlowType::kIndirectCall:
        return false;
    }
    return false;
  }
};

}

#endif
