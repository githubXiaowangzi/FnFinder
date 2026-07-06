#ifndef FNFINDER_ANALYSIS_CONTROL_FLOW_INSTRUCTION_CACHE_H_
#define FNFINDER_ANALYSIS_CONTROL_FLOW_INSTRUCTION_CACHE_H_

#include <cstddef>
#include <unordered_map>

#include "analysis/code_map.h"
#include "analysis/control_flow/decoded_instruction.h"
#include "common/types.h"
#include "disasm/disassembler.h"

namespace fnfinder::analysis::control_flow {

class InstructionCache {
 public:
  InstructionCache(const CodeMap& codeMap, disasm::IDisassembler& disassembler);

  const DecodedInstruction* at(Address address);

  bool isExecutable(Address address) const { return codeMap_.contains(address); }

  const CodeMap& codeMap() const { return codeMap_; }

  std::size_t decodedCount() const { return decoded_; }

 private:
  const CodeMap& codeMap_;
  disasm::IDisassembler& disassembler_;

  struct Entry {
    DecodedInstruction instruction;
    bool valid = false;
  };
  std::unordered_map<Address, Entry> cache_;
  std::size_t decoded_ = 0;
};

}

#endif
