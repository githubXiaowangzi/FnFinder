#ifndef FNFINDER_DISASM_AARCH64_AARCH64_MNEMONICS_H_
#define FNFINDER_DISASM_AARCH64_AARCH64_MNEMONICS_H_

#include <string>

namespace fnfinder::disasm::aarch64 {

bool isConditionalBranch(const std::string& mnemonic);
bool isReturn(const std::string& mnemonic);
bool isIndirectCall(const std::string& mnemonic);
bool isIndirectBranch(const std::string& mnemonic);

}

#endif
