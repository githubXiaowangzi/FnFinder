#include "analysis/function_boundary_resolver.h"

#include <algorithm>

namespace fnfinder::analysis {

FunctionBoundaryResolver::FunctionBoundaryResolver(const CodeMap& codeMap)
    : codeMap_(codeMap) {}

Address FunctionBoundaryResolver::regionEndFor(Address addr) const {
  const CodeRegion* region = codeMap_.regionContaining(addr);
  return region != nullptr ? region->end : addr;
}

std::vector<FunctionBounds> FunctionBoundaryResolver::resolve(
    const std::vector<Address>& sortedStarts,
    const std::unordered_map<Address, std::uint64_t>& authoritativeSizes,
    const std::map<Address, std::uint32_t>& instructionSizes) const {
  std::vector<Address> kept;
  kept.reserve(sortedStarts.size());
  Address authoritativeCoverEnd = 0;
  for (Address start : sortedStarts) {
    if (start < authoritativeCoverEnd) {
      continue;
    }
    kept.push_back(start);
    auto it = authoritativeSizes.find(start);
    if (it != authoritativeSizes.end() && it->second > 0) {
      authoritativeCoverEnd = std::max(authoritativeCoverEnd, start + it->second);
    }
  }

  std::vector<FunctionBounds> result;
  result.reserve(kept.size());
  for (std::size_t i = 0; i < kept.size(); ++i) {
    const Address start = kept[i];
    const Address regionEnd = regionEndFor(start);
    Address nextStart = (i + 1 < kept.size()) ? kept[i + 1] : regionEnd;
    if (nextStart > regionEnd) {
      nextStart = regionEnd;
    }
    if (nextStart <= start) {
      nextStart = regionEnd;
    }

    Address end = 0;
    auto authIt = authoritativeSizes.find(start);
    if (authIt != authoritativeSizes.end() && authIt->second > 0) {
      end = start + authIt->second;
    } else {
      Address maxEnd = 0;
      auto it = instructionSizes.lower_bound(start);
      const auto stop = instructionSizes.lower_bound(nextStart);
      for (; it != stop; ++it) {
        maxEnd = std::max(maxEnd, it->first + it->second);
      }
      end = (maxEnd > start) ? maxEnd : nextStart;
    }

    end = std::min(end, regionEnd);
    end = std::min(end, nextStart);
    if (end <= start) {
      end = std::min<Address>(regionEnd, start + kAArch64InstructionSize);
    }
    if (end <= start) {
      continue;
    }

    result.push_back(FunctionBounds{start, end});
  }

  return result;
}

}
