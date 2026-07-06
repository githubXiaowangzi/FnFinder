#include "disasm/capstone_disassembler.h"

#include <string>

#include "common/exception.h"
#include "common/string_util.h"
#include "disasm/capstone_compat.h"

namespace fnfinder::disasm {
namespace {

bool isConditionalMnemonic(const char* mnemonic) {
  const std::string m = mnemonic;
  if (strutil::startsWith(m, "b.")) {
    return true;
  }
  return m == "cbz" || m == "cbnz" || m == "tbz" || m == "tbnz";
}

}

CapstoneDisassembler::CapstoneDisassembler() {
  csh handle = 0;
  const cs_err err = cs_open(FN_CS_ARCH, CS_MODE_LITTLE_ENDIAN, &handle);
  if (err != CS_ERR_OK) {
    throw DisassemblerError(std::string("cs_open failed: ") + cs_strerror(err));
  }
  cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

  cs_insn* insn = cs_malloc(handle);
  if (insn == nullptr) {
    cs_close(&handle);
    throw DisassemblerError("cs_malloc failed to allocate an instruction buffer");
  }
  handle_ = static_cast<std::size_t>(handle);
  insn_ = insn;
}

CapstoneDisassembler::~CapstoneDisassembler() {
  if (insn_ != nullptr) {
    cs_free(static_cast<cs_insn*>(insn_), 1);
    insn_ = nullptr;
  }
  if (handle_ != 0) {
    csh handle = static_cast<csh>(handle_);
    cs_close(&handle);
    handle_ = 0;
  }
}

bool CapstoneDisassembler::decode(const std::uint8_t* code, std::size_t size,
                                  Address address, Instruction& out) {
  if (code == nullptr || size < kAArch64InstructionSize) {
    return false;
  }
  const csh handle = static_cast<csh>(handle_);
  auto* insn = static_cast<cs_insn*>(insn_);

  const std::uint8_t* cursor = code;
  std::size_t remaining = size;
  std::uint64_t pc = address;
  if (!cs_disasm_iter(handle, &cursor, &remaining, &pc, insn)) {
    return false;
  }

  out = Instruction{};
  out.address = insn->address;
  out.size = insn->size;
  out.mnemonic = insn->mnemonic;
  out.operands = insn->op_str;
  classify(insn, out);
  return true;
}

void CapstoneDisassembler::classify(const void* rawInsn, Instruction& out) const {
  const auto* insn = static_cast<const cs_insn*>(rawInsn);
  const csh handle = static_cast<csh>(handle_);

  if (insn->detail == nullptr) {
    out.flow = FlowType::kSequential;
    return;
  }

  const bool isJump = cs_insn_group(handle, insn, CS_GRP_JUMP);
  const bool isCall = cs_insn_group(handle, insn, CS_GRP_CALL);
  const bool isRet = cs_insn_group(handle, insn, CS_GRP_RET);
  const bool isRelative = cs_insn_group(handle, insn, CS_GRP_BRANCH_RELATIVE);
  const bool isConditional = isConditionalMnemonic(insn->mnemonic);
  const std::string mnemonic = insn->mnemonic;

  if (isRet || mnemonic == "eret") {
    out.flow = FlowType::kReturn;
  } else if (isCall) {
    out.flow = isRelative ? FlowType::kDirectCall : FlowType::kIndirectCall;
  } else if (isJump) {
    if (!isRelative) {
      out.flow = FlowType::kIndirectBranch;
    } else {
      out.flow = isConditional ? FlowType::kConditionalBranch
                               : FlowType::kUnconditionalBranch;
    }
  } else {
    out.flow = FlowType::kSequential;
  }

  const bool wantsTarget = out.flow == FlowType::kConditionalBranch ||
                           out.flow == FlowType::kUnconditionalBranch ||
                           out.flow == FlowType::kDirectCall;
  if (wantsTarget && isRelative) {
    const auto& detail = FN_ARCH_DETAIL(insn);
    for (int k = detail.op_count - 1; k >= 0; --k) {
      if (detail.operands[k].type == FN_OP_IMM) {
        out.target = static_cast<Address>(detail.operands[k].imm);
        out.hasTarget = true;
        break;
      }
    }
  }
}

}
