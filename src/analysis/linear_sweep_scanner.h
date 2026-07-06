#ifndef FNFINDER_ANALYSIS_LINEAR_SWEEP_SCANNER_H_
#define FNFINDER_ANALYSIS_LINEAR_SWEEP_SCANNER_H_

#include <cstdint>
#include <utility>
#include <vector>

#include "analysis/code_map.h"
#include "common/types.h"
#include "disasm/disassembler.h"

namespace fnfinder::analysis {

using AddressRange = std::pair<Address, Address>;

class LinearSweepScanner {
 public:
  LinearSweepScanner(const CodeMap& codeMap, disasm::IDisassembler& disassembler);

  std::vector<Address> scan(const std::vector<AddressRange>& coveredRanges) const;

 private:
  bool isPaddingWord(Address addr) const;

  void sweepGap(Address start, Address end, std::vector<Address>& out) const;

  const CodeMap& codeMap_;
  disasm::IDisassembler& disassembler_;
};

}

#endif
