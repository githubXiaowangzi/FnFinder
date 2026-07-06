#ifndef FNFINDER_COMMON_TYPES_H_
#define FNFINDER_COMMON_TYPES_H_

#include <cstddef>
#include <cstdint>

namespace fnfinder {

using Address = std::uint64_t;

using FileOffset = std::uint64_t;

inline constexpr std::uint32_t kAArch64InstructionSize = 4u;

inline constexpr Address kInvalidAddress = ~static_cast<Address>(0);

}

#endif
