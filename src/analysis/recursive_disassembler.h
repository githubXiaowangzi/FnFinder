#ifndef FNFINDER_ANALYSIS_RECURSIVE_DISASSEMBLER_H_
#define FNFINDER_ANALYSIS_RECURSIVE_DISASSEMBLER_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "analysis/code_map.h"
#include "common/types.h"
#include "disasm/disassembler.h"

namespace fnfinder::analysis {

struct RecursiveResult {
  std::set<Address> functionStarts;

  std::map<Address, std::uint32_t> instructionSizes;
};

class RecursiveDisassembler {
 public:
  RecursiveDisassembler(const CodeMap& codeMap, disasm::IDisassembler& disassembler,
                        std::size_t maxInstructions);

  RecursiveResult discover(const std::vector<Address>& seeds) const;

 private:
  const CodeMap& codeMap_;
  disasm::IDisassembler& disassembler_;
  std::size_t maxInstructions_;
};

}

#endif
