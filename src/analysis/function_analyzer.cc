#include "analysis/function_analyzer.h"

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "analysis/code_map.h"
#include "analysis/control_flow/function_extent_analyzer.h"
#include "analysis/eh_frame_parser.h"
#include "analysis/function_bounds.h"
#include "analysis/recursive_disassembler.h"
#include "common/logger.h"
#include "elf/elf_symbol.h"
#include "symbol/name_resolver.h"

namespace fnfinder::analysis {

FunctionAnalyzer::FunctionAnalyzer(const elf::ElfImage& image,
                                   disasm::IDisassembler& disassembler,
                                   const symbol::IDemangler& demangler)
    : image_(image), disassembler_(disassembler), demangler_(demangler) {}

AnalysisResult FunctionAnalyzer::analyze(const AnalysisOptions& options) const {
  AnalysisResult result;

  CodeMap codeMap(image_);
  if (codeMap.empty()) {
    logWarn("no executable code found; nothing to analyse");
    return result;
  }

  symbol::NameResolver nameResolver(image_, demangler_, options.demangle);

  std::set<Address> starts;
  std::unordered_map<Address, std::uint64_t> authoritativeSizes;

  const auto addStart = [&](Address addr) {
    if (codeMap.contains(addr)) {
      starts.insert(addr);
    }
  };
  const auto setAuthoritativeSize = [&](Address addr, std::uint64_t size) {
    if (size == 0) {
      return;
    }
    authoritativeSizes.emplace(addr, size);
  };

  if (options.useSymbols) {
    for (const elf::ElfSymbol& sym : image_.symbols()) {
      if (!sym.isFunction() || !sym.isDefined()) {
        continue;
      }
      addStart(sym.value());
      setAuthoritativeSize(sym.value(), sym.size());
    }
  }
  if (options.useEntryPoint && image_.entryPoint() != 0) {
    addStart(image_.entryPoint());
  }
  if (options.useEhFrame) {
    EhFrameParser ehFrameParser(image_);
    for (const EhFrameFunction& fn : ehFrameParser.parse()) {
      addStart(fn.start);
      setAuthoritativeSize(fn.start, fn.size);
    }
  }

  const std::set<Address> strongStarts = starts;

  std::map<Address, std::uint32_t> instructionSizes;
  RecursiveDisassembler recursive(codeMap, disassembler_, options.maxInstructions);

  if (options.useRecursive) {
    std::vector<Address> seeds(starts.begin(), starts.end());
    if (seeds.empty()) {
      for (const CodeRegion& region : codeMap.regions()) {
        seeds.push_back(region.start);
      }
    }
    RecursiveResult discovered = recursive.discover(seeds);
    instructionSizes = std::move(discovered.instructionSizes);
    for (Address start : discovered.functionStarts) {
      addStart(start);
    }
  }

  control_flow::FunctionExtentAnalyzer::Input cfInput;
  cfInput.strongEntries.assign(strongStarts.begin(), strongStarts.end());
  for (Address addr : starts) {
    if (strongStarts.find(addr) == strongStarts.end()) {
      cfInput.softEntries.push_back(addr);
    }
  }
  cfInput.authoritativeSizes = authoritativeSizes;

  control_flow::FunctionExtentAnalyzer::Options cfOptions;
  cfOptions.maxInstructions = options.maxInstructions;

  control_flow::FunctionExtentAnalyzer extentAnalyzer(codeMap, disassembler_);
  const std::vector<FunctionBounds> bounds =
      extentAnalyzer.analyze(cfInput, cfOptions);

  const auto lastInstructionOf = [&](Address start, Address end) -> Address {
    auto it = instructionSizes.lower_bound(end);
    if (it != instructionSizes.begin()) {
      --it;
      if (it->first >= start && it->first + it->second == end) {
        return it->first;
      }
    }
    return (end - start >= kAArch64InstructionSize) ? end - kAArch64InstructionSize
                                                    : start;
  };

  result.functions.reserve(bounds.size());
  for (const FunctionBounds& b : bounds) {
    Function fn;
    fn.start = b.start;
    fn.end = b.end;
    fn.lastInstruction = lastInstructionOf(b.start, b.end);
    fn.size = b.end - b.start;
    fn.name = nameResolver.nameFor(b.start);
    fn.hasSymbol = nameResolver.hasSymbol(b.start);
    result.functions.push_back(std::move(fn));
  }

  std::sort(result.functions.begin(), result.functions.end(),
            [](const Function& a, const Function& b) { return a.start < b.start; });

  logInfo("analysis complete: " + std::to_string(result.count()) + " function(s), " +
          std::to_string(result.namedCount()) + " named by symbols");
  return result;
}

}
