#include "analysis/code_map.h"

#include <algorithm>

#include "common/logger.h"

namespace fnfinder::analysis {

CodeMap::CodeMap(const elf::ElfImage& image) {
  bool anySection = false;
  for (const elf::ElfSection& section : image.sections()) {
    if (!section.isCode()) {
      continue;
    }
    std::size_t available = 0;
    const std::uint8_t* data = image.dataAt(section.address(), available);
    if (data != nullptr) {
      addRegion(section.address(), std::min<std::uint64_t>(section.size(), available),
                data);
      anySection = true;
    }
  }

  if (!anySection) {
    for (const elf::ElfSegment& segment : image.segments()) {
      if (!segment.isLoadable() || !segment.isExecutable() || segment.fileSize() == 0) {
        continue;
      }
      std::size_t available = 0;
      const std::uint8_t* data = image.dataAt(segment.virtualAddress(), available);
      if (data != nullptr) {
        addRegion(segment.virtualAddress(),
                  std::min<std::uint64_t>(segment.fileSize(), available), data);
      }
    }
  }

  std::sort(regions_.begin(), regions_.end(),
            [](const CodeRegion& a, const CodeRegion& b) { return a.start < b.start; });

  if (!regions_.empty()) {
    lowest_ = regions_.front().start;
    highest_ = regions_.back().end;
  }

  logInfo("code map covers " + std::to_string(regions_.size()) + " executable region(s)");
}

void CodeMap::addRegion(Address start, std::uint64_t size, const std::uint8_t* data) {
  if (size == 0 || data == nullptr) {
    return;
  }
  CodeRegion region;
  region.start = start;
  region.end = start + size;
  region.data = data;
  regions_.push_back(region);
}

const CodeRegion* CodeMap::regionContaining(Address addr) const {
  auto it = std::upper_bound(
      regions_.begin(), regions_.end(), addr,
      [](Address value, const CodeRegion& region) { return value < region.start; });
  if (it == regions_.begin()) {
    return nullptr;
  }
  --it;
  return it->contains(addr) ? &(*it) : nullptr;
}

const std::uint8_t* CodeMap::at(Address addr, std::size_t& available) const {
  available = 0;
  const CodeRegion* region = regionContaining(addr);
  if (region == nullptr) {
    return nullptr;
  }
  available = static_cast<std::size_t>(region->end - addr);
  return region->data + (addr - region->start);
}

}
