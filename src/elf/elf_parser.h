#ifndef FNFINDER_ELF_ELF_PARSER_H_
#define FNFINDER_ELF_ELF_PARSER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "common/byte_reader.h"
#include "common/types.h"
#include "elf/elf_image.h"

namespace fnfinder::elf {

class ElfParser {
 public:
  static std::unique_ptr<ElfImage> parseFile(const std::string& path);

  static std::unique_ptr<ElfImage> parseBytes(std::vector<std::uint8_t> bytes);

 private:
  explicit ElfParser(std::unique_ptr<ElfImage> image);

  std::unique_ptr<ElfImage> run();

  void validateHeader();
  void parseSegments();
  void parseSections();
  void resolveSectionNames();
  void parseSymbols();
  void parseSectionSymbolTable(const ElfSection& symtab, SymbolOrigin origin);
  void parseDynamicSymbols();

  const ElfSegment* findSegment(std::uint32_t type) const;

  std::uint64_t dynamicSymbolCount(Address symtabVaddr, Address strtabVaddr,
                                   Address hashVaddr, Address gnuHashVaddr,
                                   std::uint64_t symEntSize) const;

  ElfImage& image() { return *image_; }

  std::unique_ptr<ElfImage> image_;
  ByteReader reader_;

  FileOffset sectionHeaderOffset_ = 0;
  FileOffset programHeaderOffset_ = 0;
  std::uint16_t sectionCount_ = 0;
  std::uint16_t sectionEntrySize_ = 0;
  std::uint32_t sectionNameIndex_ = 0;
  std::uint16_t programCount_ = 0;
  std::uint16_t programEntrySize_ = 0;
};

}

#endif
