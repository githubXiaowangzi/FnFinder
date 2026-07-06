#ifndef FNFINDER_ANALYSIS_EH_FRAME_PARSER_H_
#define FNFINDER_ANALYSIS_EH_FRAME_PARSER_H_

#include <cstdint>
#include <vector>

#include "common/byte_reader.h"
#include "common/types.h"
#include "elf/elf_image.h"

namespace fnfinder::analysis {

struct EhFrameFunction {
  Address start = 0;
  std::uint64_t size = 0;
};

class EhFrameParser {
 public:
  explicit EhFrameParser(const elf::ElfImage& image);

  std::vector<EhFrameFunction> parse();

 private:
  struct Blob {
    Address vaddr = 0;
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    bool valid() const { return data != nullptr && size > 0; }
  };

  Blob locateEhFrameHdr() const;
  Blob locateEhFrame(const Blob& ehFrameHdr) const;

  bool decodePointer(ByteReader& reader, std::uint8_t encoding, Address blobVaddr,
                     bool applyBase, std::uint64_t& out) const;

  const elf::ElfImage& image_;
};

}

#endif
