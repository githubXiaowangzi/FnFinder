#ifndef FNFINDER_ELF_ELF_SEGMENT_H_
#define FNFINDER_ELF_ELF_SEGMENT_H_

#include <cstdint>

#include "common/types.h"
#include "elf/elf_constants.h"

namespace fnfinder::elf {

class ElfSegment {
 public:
  std::uint32_t type() const { return type_; }
  std::uint32_t flags() const { return flags_; }
  FileOffset fileOffset() const { return fileOffset_; }
  Address virtualAddress() const { return virtualAddress_; }
  std::uint64_t fileSize() const { return fileSize_; }
  std::uint64_t memorySize() const { return memorySize_; }

  bool isLoadable() const { return type_ == kPtLoad; }
  bool isExecutable() const { return (flags_ & kPfExec) != 0; }

  bool containsAddress(Address addr) const {
    return isLoadable() && addr >= virtualAddress_ &&
           addr < virtualAddress_ + memorySize_;
  }

 private:
  friend class ElfParser;

  std::uint32_t type_ = 0;
  std::uint32_t flags_ = 0;
  FileOffset fileOffset_ = 0;
  Address virtualAddress_ = 0;
  std::uint64_t fileSize_ = 0;
  std::uint64_t memorySize_ = 0;
};

}

#endif
