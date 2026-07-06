#ifndef FNFINDER_DISASM_CAPSTONE_DISASSEMBLER_H_
#define FNFINDER_DISASM_CAPSTONE_DISASSEMBLER_H_

#include <cstddef>
#include <cstdint>

#include "common/types.h"
#include "disasm/disassembler.h"
#include "disasm/instruction.h"

namespace fnfinder::disasm {

class CapstoneDisassembler : public IDisassembler {
 public:
  CapstoneDisassembler();
  ~CapstoneDisassembler() override;

  CapstoneDisassembler(const CapstoneDisassembler&) = delete;
  CapstoneDisassembler& operator=(const CapstoneDisassembler&) = delete;

  bool decode(const std::uint8_t* code, std::size_t size, Address address,
              Instruction& out) override;

 private:
  void classify(const void* rawInsn, Instruction& out) const;

  std::size_t handle_ = 0;
  void* insn_ = nullptr;
};

}

#endif
