#ifndef FNFINDER_ANALYSIS_CODE_MAP_H_
#define FNFINDER_ANALYSIS_CODE_MAP_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/types.h"
#include "elf/elf_image.h"

namespace fnfinder::analysis {

struct CodeRegion {
  Address start = 0;
  Address end = 0;
  const std::uint8_t* data = nullptr;

  bool contains(Address addr) const { return addr >= start && addr < end; }
};

class CodeMap {
 public:
  explicit CodeMap(const elf::ElfImage& image);

  const std::vector<CodeRegion>& regions() const { return regions_; }
  bool empty() const { return regions_.empty(); }

  bool contains(Address addr) const { return regionContaining(addr) != nullptr; }
  const CodeRegion* regionContaining(Address addr) const;

  const std::uint8_t* at(Address addr, std::size_t& available) const;

  Address lowest() const { return lowest_; }
  Address highest() const { return highest_; }

 private:
  void addRegion(Address start, std::uint64_t size, const std::uint8_t* data);

  std::vector<CodeRegion> regions_;
  Address lowest_ = 0;
  Address highest_ = 0;
};

}

#endif
