#ifndef FNFINDER_ELF_ELF_SYMBOL_H_
#define FNFINDER_ELF_ELF_SYMBOL_H_

#include <cstdint>
#include <string>

#include "common/types.h"
#include "elf/elf_constants.h"

namespace fnfinder::elf {

enum class SymbolOrigin {
  kSymtab,
  kDynsym,
};

class ElfSymbol {
 public:
  const std::string& name() const { return name_; }
  Address value() const { return value_; }
  std::uint64_t size() const { return size_; }
  std::uint8_t info() const { return info_; }
  std::uint16_t sectionIndex() const { return sectionIndex_; }
  SymbolOrigin origin() const { return origin_; }

  std::uint8_t type() const { return static_cast<std::uint8_t>(info_ & 0x0f); }
  std::uint8_t binding() const { return static_cast<std::uint8_t>(info_ >> 4); }

  bool isFunction() const {
    const std::uint8_t t = type();
    return t == kSttFunc || t == kSttGnuIfunc;
  }

  bool isDefined() const {
    return sectionIndex_ != kShnUndef && sectionIndex_ < kShnLoReserve;
  }

  bool isGlobal() const { return binding() == kStbGlobal; }
  bool isWeak() const { return binding() == kStbWeak; }
  bool hasName() const { return !name_.empty(); }

 private:
  friend class ElfParser;

  std::string name_;
  Address value_ = 0;
  std::uint64_t size_ = 0;
  std::uint8_t info_ = 0;
  std::uint8_t other_ = 0;
  std::uint16_t sectionIndex_ = 0;
  SymbolOrigin origin_ = SymbolOrigin::kSymtab;
};

}

#endif
