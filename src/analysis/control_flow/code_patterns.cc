#include "analysis/control_flow/code_patterns.h"

#include <cstddef>

namespace fnfinder::analysis::control_flow {

bool readWord(const CodeMap& codeMap, Address addr, std::uint32_t& word) {
  std::size_t available = 0;
  const std::uint8_t* p = codeMap.at(addr, available);
  if (p == nullptr || available < kAArch64InstructionSize) {
    return false;
  }
  word = static_cast<std::uint32_t>(p[0]) |
         (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) |
         (static_cast<std::uint32_t>(p[3]) << 24);
  return true;
}

bool isPaddingWord(const CodeMap& codeMap, Address addr) {
  std::uint32_t word = 0;
  if (!readWord(codeMap, addr, word)) {
    return true;
  }
  return word == kZeroWord || word == kNopWord;
}

bool isHardStopWord(const CodeMap& codeMap, Address addr) {
  std::uint32_t word = 0;
  if (!readWord(codeMap, addr, word)) {
    return true;
  }
  return word == kZeroWord;
}

}
