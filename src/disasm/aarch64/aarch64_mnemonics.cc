#include "disasm/aarch64/aarch64_mnemonics.h"

#include "common/string_util.h"

namespace fnfinder::disasm::aarch64 {

bool isConditionalBranch(const std::string& mnemonic) {
  if (strutil::startsWith(mnemonic, "b.")) {
    return true;
  }
  return mnemonic == "cbz" || mnemonic == "cbnz" || mnemonic == "tbz" ||
         mnemonic == "tbnz";
}

bool isReturn(const std::string& mnemonic) {
  return mnemonic == "ret" || mnemonic == "retaa" || mnemonic == "retab" ||
         mnemonic == "eret" || mnemonic == "eretaa" || mnemonic == "eretab" ||
         mnemonic == "drps";
}

bool isIndirectCall(const std::string& mnemonic) {
  return mnemonic == "blr" || mnemonic == "blraa" || mnemonic == "blrab" ||
         mnemonic == "blraaz" || mnemonic == "blrabz";
}

bool isIndirectBranch(const std::string& mnemonic) {
  return mnemonic == "br" || mnemonic == "braa" || mnemonic == "brab" ||
         mnemonic == "braaz" || mnemonic == "brabz";
}

}
