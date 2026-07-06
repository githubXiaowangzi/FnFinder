#include "analysis/control_flow/function_extent_analyzer.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

#include "analysis/control_flow/cfg_builder.h"
#include "analysis/control_flow/code_patterns.h"
#include "analysis/control_flow/control_flow_graph.h"
#include "analysis/control_flow/instruction_cache.h"
#include "common/logger.h"
#include "common/string_util.h"

namespace fnfinder::analysis::control_flow {
namespace {

Address alignUp(Address addr) {
  const Address mask = kAArch64InstructionSize - 1;
  return (addr + mask) & ~mask;
}

}

FunctionExtentAnalyzer::FunctionExtentAnalyzer(const CodeMap& codeMap,
                                               disasm::IDisassembler& disassembler)
    : codeMap_(codeMap), disassembler_(disassembler) {}

std::vector<FunctionBounds> FunctionExtentAnalyzer::analyze(
    const Input& input, const Options& options) const {
  InstructionCache cache(codeMap_, disassembler_);
  CfgBuilder builder(cache);

  const auto regionEndFor = [&](Address addr) -> Address {
    const CodeRegion* region = codeMap_.regionContaining(addr);
    return region != nullptr ? region->end : addr;
  };
  const auto isUsable = [&](Address addr) {
    return (addr % kAArch64InstructionSize) == 0 && codeMap_.contains(addr);
  };

  std::vector<Address> strongSorted;
  strongSorted.reserve(input.strongEntries.size());
  for (Address addr : input.strongEntries) {
    if (isUsable(addr)) {
      strongSorted.push_back(addr);
    }
  }
  std::sort(strongSorted.begin(), strongSorted.end());
  strongSorted.erase(std::unique(strongSorted.begin(), strongSorted.end()),
                     strongSorted.end());

  const auto nextStrongAfter = [&](Address addr) -> Address {
    auto it = std::upper_bound(strongSorted.begin(), strongSorted.end(), addr);
    return it != strongSorted.end() ? *it : kInvalidAddress;
  };

  std::set<Address> entryBoundaries;
  for (Address addr : input.strongEntries) {
    if (isUsable(addr)) entryBoundaries.insert(addr);
  }
  for (Address addr : input.softEntries) {
    if (isUsable(addr)) entryBoundaries.insert(addr);
  }

  const auto cfgExtentEnd = [&](Address entry, Address upperBound) -> Address {
    CfgBuildRequest request;
    request.entry = entry;
    request.upperBound = upperBound;
    request.boundaries = &entryBoundaries;
    request.maxInstructions = options.maxInstructions;
    request.linearPastIndirectBranch = options.linearPastIndirectBranch;
    const ControlFlowGraph cfg = builder.build(request);
    if (cfg.truncated()) {
      logWarn("control-flow extent for " + strutil::toHex(entry, true, 16, false) +
              " hit the instruction cap; extent is a lower bound");
    }
    Address end = cfg.extentEnd();
    if (end <= entry) {
      end = std::min<Address>(regionEndFor(entry), entry + kAArch64InstructionSize);
    }
    return end;
  };

  struct Candidate {
    Address addr = 0;
    bool strong = false;
  };
  std::vector<Candidate> candidates;
  candidates.reserve(input.strongEntries.size() + input.softEntries.size());
  for (Address addr : input.strongEntries) {
    if (isUsable(addr)) {
      candidates.push_back(Candidate{addr, true});
    }
  }
  for (Address addr : input.softEntries) {
    if (isUsable(addr)) {
      candidates.push_back(Candidate{addr, false});
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const Candidate& a, const Candidate& b) {
              if (a.addr != b.addr) return a.addr < b.addr;
              return a.strong && !b.strong;
            });

  std::vector<FunctionBounds> emitted;
  emitted.reserve(candidates.size());

  Address coverEnd = 0;
  Address lastAddr = kInvalidAddress;
  std::size_t suppressed = 0;

  for (const Candidate& c : candidates) {
    if (c.addr == lastAddr) {
      continue;
    }
    lastAddr = c.addr;

    if (c.addr < coverEnd) {
      ++suppressed;
      continue;
    }

    const Address rEnd = regionEndFor(c.addr);
    if (rEnd <= c.addr) {
      continue;
    }

    Address end = 0;
    const auto authIt = input.authoritativeSizes.find(c.addr);
    if (c.strong && authIt != input.authoritativeSizes.end() &&
        authIt->second > 0) {
      const std::uint64_t room = rEnd - c.addr;
      end = (authIt->second >= room) ? rEnd : c.addr + authIt->second;
    } else {
      const Address upper = std::min<Address>(nextStrongAfter(c.addr), rEnd);
      end = std::min<Address>(cfgExtentEnd(c.addr, upper), rEnd);
    }

    if (end <= c.addr) {
      continue;
    }
    emitted.push_back(FunctionBounds{c.addr, end});
    coverEnd = std::max(coverEnd, end);
  }

  const std::size_t referencedCount = emitted.size();

  std::size_t gapCount = 0;
  if (options.discoverGapFunctions) {
    std::sort(emitted.begin(), emitted.end(),
              [](const FunctionBounds& a, const FunctionBounds& b) {
                return a.start < b.start;
              });

    std::vector<FunctionBounds> gapFunctions;

    for (const CodeRegion& region : codeMap_.regions()) {
      Address pos = region.start;
      const auto carveGap = [&](Address gapStart, Address gapEnd) {
        Address cursor = alignUp(gapStart);
        while (cursor + kAArch64InstructionSize <= gapEnd) {
          if (isPaddingWord(codeMap_, cursor)) {
            cursor += kAArch64InstructionSize;
            continue;
          }
          const DecodedInstruction* insn = cache.at(cursor);
          if (insn == nullptr) {
            cursor += kAArch64InstructionSize;
            continue;
          }
          const Address upper =
              std::min<Address>(nextStrongAfter(cursor), gapEnd);
          CfgBuildRequest request;
          request.entry = cursor;
          request.upperBound = upper;
          request.boundaries = &entryBoundaries;
          request.maxInstructions = options.maxInstructions;
          request.linearPastIndirectBranch = options.linearPastIndirectBranch;
          const ControlFlowGraph cfg = builder.build(request);
          Address end = std::min<Address>(cfg.extentEnd(), gapEnd);

          const bool plausible =
              cfg.reachesReturn() ||
              cfg.instructionCount() >= options.minGapFunctionInstructions;
          if (end <= cursor || cfg.empty() || !plausible) {
            cursor += kAArch64InstructionSize;
            continue;
          }
          gapFunctions.push_back(FunctionBounds{cursor, end});
          ++gapCount;
          cursor = alignUp(end);
        }
      };

      for (const FunctionBounds& fn : emitted) {
        if (fn.end <= region.start || fn.start >= region.end) {
          continue;
        }
        const Address covStart = std::max(fn.start, region.start);
        const Address covEnd = std::min(fn.end, region.end);
        if (covStart > pos) {
          carveGap(pos, covStart);
        }
        pos = std::max(pos, covEnd);
      }
      if (pos < region.end) {
        carveGap(pos, region.end);
      }
    }

    emitted.insert(emitted.end(), gapFunctions.begin(), gapFunctions.end());
  }

  std::sort(emitted.begin(), emitted.end(),
            [](const FunctionBounds& a, const FunctionBounds& b) {
              if (a.start != b.start) return a.start < b.start;
              return a.end < b.end;
            });

  std::vector<FunctionBounds> result;
  result.reserve(emitted.size());
  for (const FunctionBounds& fn : emitted) {
    if (fn.end <= fn.start) {
      continue;
    }
    if (!result.empty() && fn.start < result.back().end) {
      continue;
    }
    result.push_back(fn);
  }

  logInfo("control-flow boundary analysis: " + std::to_string(result.size()) +
          " function(s) [" + std::to_string(referencedCount) +
          " referenced, " + std::to_string(gapCount) + " gap-discovered, " +
          std::to_string(suppressed) + " interior start(s) demoted]");
  return result;
}

}
