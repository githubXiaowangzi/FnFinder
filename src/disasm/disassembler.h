#ifndef FNFINDER_DISASM_DISASSEMBLER_H_
#define FNFINDER_DISASM_DISASSEMBLER_H_

#include <cstddef>
#include <cstdint>

#include "common/types.h"
#include "disasm/instruction.h"

namespace fnfinder::disasm {

class IDisassembler {
 public:
  virtual ~IDisassembler() = default;

  virtual bool decode(const std::uint8_t* code, std::size_t size, Address address,
                      Instruction& out) = 0;
};

}

#endif
