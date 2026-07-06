#include "analysis/linear_sweep_scanner.h"

#include <algorithm>

#include "common/logger.h"
#include "disasm/instruction.h"

namespace fnfinder::analysis {
namespace {

constexpr std::uint32_t kNopWord = 0xd503201fu;

}

LinearSweepScanner::LinearSweepScanner(const CodeMap& codeMap,
                                       disasm::IDisassembler& disassembler)
    : codeMap_(codeMap), disassembler_(disassembler) {}

bool LinearSweepScanner::isPaddingWord(Address addr) const {
  std::size_t available = 0;
  const std::uint8_t* p = codeMap_.at(addr, available);
  if (p == nullptr || available < kAArch64InstructionSize) {
    return false;
  }
  const std::uint32_t word = static_cast<std::uint32_t>(p[0]) |
                             (static_cast<std::uint32_t>(p[1]) << 8) |
                             (static_cast<std::uint32_t>(p[2]) << 16) |
                             (static_cast<std::uint32_t>(p[3]) << 24);
  return word == 0 || word == kNopWord;
}

void LinearSweepScanner::sweepGap(Address start, Address end,
                                 std::vector<Address>& out) const {
  Address a = start;
  bool needLeader = true;
  while (a < end) {
    if (isPaddingWord(a)) {
      a += kAArch64InstructionSize;
      needLeader = true;
      continue;
    }
    std::size_t available = 0;
    const std::uint8_t* code = codeMap_.at(a, available);
    const std::size_t window =
        std::min<std::size_t>(available, static_cast<std::size_t>(end - a));
    disasm::Instruction insn;
    if (code == nullptr || window < kAArch64InstructionSize ||
        !disassembler_.decode(code, window, a, insn)) {
      a += kAArch64InstructionSize;
      needLeader = true;
      continue;
    }
    if (needLeader) {
      out.push_back(a);
      needLeader = false;
    }
    a += insn.size;
    if (insn.isTerminator()) {
      needLeader = true;
    }
  }
}

std::vector<Address> LinearSweepScanner::scan(
    const std::vector<AddressRange>& coveredRanges) const {
  std::vector<Address> candidates;

  for (const CodeRegion& region : codeMap_.regions()) {
    Address pos = region.start;
    for (const AddressRange& covered : coveredRanges) {
      if (covered.second <= region.start || covered.first >= region.end) {
        continue;
      }
      const Address coveredStart = std::max(covered.first, region.start);
      const Address coveredEnd = std::min(covered.second, region.end);
      if (coveredStart > pos) {
        sweepGap(pos, coveredStart, candidates);
      }
      pos = std::max(pos, coveredEnd);
    }
    if (pos < region.end) {
      sweepGap(pos, region.end, candidates);
    }
  }

  logInfo("linear sweep proposed " + std::to_string(candidates.size()) +
          " additional function start(s)");
  return candidates;
}

}
