#include "elf/elf_image.h"

#include <algorithm>

namespace fnfinder::elf {

const ElfSection* ElfImage::findSection(const std::string& name) const {
  for (const ElfSection& section : sections_) {
    if (section.name() == name) {
      return &section;
    }
  }
  return nullptr;
}

const ElfSection* ElfImage::sectionContaining(Address addr) const {
  for (const ElfSection& section : sections_) {
    if (section.isCode() && section.containsAddress(addr)) {
      return &section;
    }
  }
  return nullptr;
}

const std::uint8_t* ElfImage::dataAt(Address addr, std::size_t& available) const {
  available = 0;

  for (const ElfSection& section : sections_) {
    if (!section.isAllocated() || !section.hasFileData()) {
      continue;
    }
    if (addr < section.address() || addr >= section.address() + section.size()) {
      continue;
    }
    const std::uint64_t delta = addr - section.address();
    const FileOffset fileOffset = section.fileOffset() + delta;
    if (fileOffset >= bytes_.size()) {
      return nullptr;
    }
    available = static_cast<std::size_t>(
        std::min<std::uint64_t>(section.size() - delta, bytes_.size() - fileOffset));
    return bytes_.data() + fileOffset;
  }

  for (const ElfSegment& segment : segments_) {
    if (!segment.isLoadable()) {
      continue;
    }
    if (addr < segment.virtualAddress() ||
        addr >= segment.virtualAddress() + segment.fileSize()) {
      continue;
    }
    const std::uint64_t delta = addr - segment.virtualAddress();
    const FileOffset fileOffset = segment.fileOffset() + delta;
    if (fileOffset >= bytes_.size()) {
      return nullptr;
    }
    available = static_cast<std::size_t>(
        std::min<std::uint64_t>(segment.fileSize() - delta, bytes_.size() - fileOffset));
    return bytes_.data() + fileOffset;
  }

  return nullptr;
}

}
