#ifndef FNFINDER_ANALYSIS_FUNCTION_BOUNDARY_RESOLVER_H_
#define FNFINDER_ANALYSIS_FUNCTION_BOUNDARY_RESOLVER_H_

#include <cstdint>
#include <map>
#include <unordered_map>
#include <vector>

#include "analysis/code_map.h"
#include "common/types.h"

namespace fnfinder::analysis {

struct FunctionBounds {
  Address start = 0;
  Address end = 0;
};

class FunctionBoundaryResolver {
 public:
  explicit FunctionBoundaryResolver(const CodeMap& codeMap);

  std::vector<FunctionBounds> resolve(
      const std::vector<Address>& sortedStarts,
      const std::unordered_map<Address, std::uint64_t>& authoritativeSizes,
      const std::map<Address, std::uint32_t>& instructionSizes) const;

 private:
  Address regionEndFor(Address addr) const;

  const CodeMap& codeMap_;
};

}

#endif
