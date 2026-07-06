#include "analysis/control_flow/instruction_cache.h"

#include "common/types.h"
#include "disasm/instruction.h"

namespace fnfinder::analysis::control_flow {

InstructionCache::InstructionCache(const CodeMap& codeMap,
                                   disasm::IDisassembler& disassembler)
    : codeMap_(codeMap), disassembler_(disassembler) {}

const DecodedInstruction* InstructionCache::at(Address address) {
  if ((address % kAArch64InstructionSize) != 0) {
    return nullptr;
  }

  auto it = cache_.find(address);
  if (it != cache_.end()) {
    return it->second.valid ? &it->second.instruction : nullptr;
  }

  Entry& entry = cache_[address];

  std::size_t available = 0;
  const std::uint8_t* code = codeMap_.at(address, available);
  if (code == nullptr || available < kAArch64InstructionSize) {
    entry.valid = false;
    return nullptr;
  }

  disasm::Instruction raw;
  if (!disassembler_.decode(code, available, address, raw)) {
    entry.valid = false;
    return nullptr;
  }

  DecodedInstruction& out = entry.instruction;
  out.address = raw.address;
  out.size = raw.size;
  out.flow = raw.flow;
  out.hasTarget = raw.hasTarget;
  out.target = raw.target;
  entry.valid = true;
  ++decoded_;
  return &out;
}

}
