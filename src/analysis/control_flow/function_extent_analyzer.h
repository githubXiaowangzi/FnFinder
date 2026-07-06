#ifndef FNFINDER_ANALYSIS_CONTROL_FLOW_FUNCTION_EXTENT_ANALYZER_H_
#define FNFINDER_ANALYSIS_CONTROL_FLOW_FUNCTION_EXTENT_ANALYZER_H_

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "analysis/code_map.h"
#include "analysis/function_bounds.h"
#include "common/types.h"
#include "disasm/disassembler.h"

namespace fnfinder::analysis::control_flow {

class FunctionExtentAnalyzer {
 public:
  struct Options {
    std::size_t maxInstructions = 8'000'000;
    bool discoverGapFunctions = true;
    bool linearPastIndirectBranch = true;
    std::size_t minGapFunctionInstructions = 2;
  };

  struct Input {
    std::vector<Address> strongEntries;
    std::vector<Address> softEntries;
    std::unordered_map<Address, std::uint64_t> authoritativeSizes;
  };

  FunctionExtentAnalyzer(const CodeMap& codeMap,
                         disasm::IDisassembler& disassembler);

  std::vector<FunctionBounds> analyze(const Input& input,
                                      const Options& options) const;

 private:
  const CodeMap& codeMap_;
  disasm::IDisassembler& disassembler_;
};

}

#endif
