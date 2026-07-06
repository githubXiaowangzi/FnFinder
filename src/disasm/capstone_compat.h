#ifndef FNFINDER_DISASM_CAPSTONE_COMPAT_H_
#define FNFINDER_DISASM_CAPSTONE_COMPAT_H_

#include <capstone/capstone.h>

#if defined(__has_include) && __has_include(<capstone/aarch64.h>)
#define FN_CS_ARCH CS_ARCH_AARCH64
#define FN_OP_IMM AArch64_OP_IMM
#define FN_ARCH_DETAIL(insn) ((insn)->detail->aarch64)
#else
#define FN_CS_ARCH CS_ARCH_ARM64
#define FN_OP_IMM ARM64_OP_IMM
#define FN_ARCH_DETAIL(insn) ((insn)->detail->arm64)
#endif

#endif
