#ifndef FNFINDER_ELF_ELF_IMAGE_H_
#define FNFINDER_ELF_ELF_IMAGE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "common/byte_reader.h"
#include "common/types.h"
#include "elf/elf_section.h"
#include "elf/elf_segment.h"
#include "elf/elf_symbol.h"

namespace fnfinder::elf {

class ElfImage {
 public:
  const std::vector<std::uint8_t>& bytes() const { return bytes_; }
  ByteReader reader() const { return ByteReader(bytes_); }

  Address entryPoint() const { return entryPoint_; }
  std::uint16_t machine() const { return machine_; }
  std::uint16_t type() const { return type_; }
  bool isLittleEndian() const { return littleEndian_; }

  const std::vector<ElfSection>& sections() const { return sections_; }
  const std::vector<ElfSegment>& segments() const { return segments_; }
  const std::vector<ElfSymbol>& symbols() const { return symbols_; }

  const ElfSection* findSection(const std::string& name) const;
  const ElfSection* sectionContaining(Address addr) const;

  const std::uint8_t* dataAt(Address addr, std::size_t& available) const;

 private:
  friend class ElfParser;

  std::vector<std::uint8_t> bytes_;
  Address entryPoint_ = 0;
  std::uint16_t machine_ = 0;
  std::uint16_t type_ = 0;
  bool littleEndian_ = true;

  std::vector<ElfSection> sections_;
  std::vector<ElfSegment> segments_;
  std::vector<ElfSymbol> symbols_;
};

}

#endif
