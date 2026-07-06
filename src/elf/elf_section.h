#ifndef FNFINDER_ELF_ELF_SECTION_H_
#define FNFINDER_ELF_ELF_SECTION_H_

#include <cstdint>
#include <string>

#include "common/types.h"
#include "elf/elf_constants.h"

namespace fnfinder::elf {

class ElfSection {
 public:
  const std::string& name() const { return name_; }
  std::uint32_t type() const { return type_; }
  std::uint64_t flags() const { return flags_; }
  Address address() const { return address_; }
  FileOffset fileOffset() const { return fileOffset_; }
  std::uint64_t size() const { return size_; }
  std::uint32_t link() const { return link_; }
  std::uint32_t info() const { return info_; }
  std::uint64_t entrySize() const { return entrySize_; }

  bool isAllocated() const { return (flags_ & kShfAlloc) != 0; }
  bool isExecutable() const { return (flags_ & kShfExecinstr) != 0; }
  bool isWritable() const { return (flags_ & kShfWrite) != 0; }

  bool hasFileData() const { return type_ != kShtNobits && type_ != kShtNull; }

  bool isCode() const {
    return isAllocated() && isExecutable() && hasFileData() && size_ > 0;
  }

  bool containsAddress(Address addr) const {
    return isAllocated() && addr >= address_ && addr < address_ + size_;
  }

 private:
  friend class ElfParser;

  std::string name_;
  std::uint32_t type_ = 0;
  std::uint64_t flags_ = 0;
  Address address_ = 0;
  FileOffset fileOffset_ = 0;
  std::uint64_t size_ = 0;
  std::uint32_t link_ = 0;
  std::uint32_t info_ = 0;
  std::uint64_t entrySize_ = 0;
};

}

#endif
