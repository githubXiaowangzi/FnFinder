#ifndef FNFINDER_ANALYSIS_CONTROL_FLOW_CODE_PATTERNS_H_
#define FNFINDER_ANALYSIS_CONTROL_FLOW_CODE_PATTERNS_H_

#include <cstdint>

#include "analysis/code_map.h"
#include "common/types.h"

namespace fnfinder::analysis::control_flow {

inline constexpr std::uint32_t kZeroWord = 0x00000000u;
inline constexpr std::uint32_t kNopWord = 0xd503201fu;

bool readWord(const CodeMap& codeMap, Address addr, std::uint32_t& word);

bool isPaddingWord(const CodeMap& codeMap, Address addr);

bool isHardStopWord(const CodeMap& codeMap, Address addr);

}

#endif
