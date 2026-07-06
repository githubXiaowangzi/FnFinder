#include "elf/elf_parser.h"

#include <algorithm>
#include <fstream>
#include <ios>

#include "common/exception.h"
#include "common/logger.h"
#include "common/string_util.h"

namespace fnfinder::elf {
namespace {

constexpr std::uint64_t kMaxSymbols = 5'000'000;

std::uint64_t strideOf(std::uint64_t declared, std::uint64_t minimum) {
  return declared >= minimum ? declared : minimum;
}

}

std::unique_ptr<ElfImage> ElfParser::parseFile(const std::string& path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream) {
    throw IoError("cannot open file: " + path);
  }
  const std::streamsize size = stream.tellg();
  if (size < 0) {
    throw IoError("cannot determine size of file: " + path);
  }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
  stream.seekg(0, std::ios::beg);
  if (size > 0 && !stream.read(reinterpret_cast<char*>(bytes.data()), size)) {
    throw IoError("failed to read file: " + path);
  }
  return parseBytes(std::move(bytes));
}

std::unique_ptr<ElfImage> ElfParser::parseBytes(std::vector<std::uint8_t> bytes) {
  auto image = std::make_unique<ElfImage>();
  image->bytes_ = std::move(bytes);
  ElfParser parser(std::move(image));
  return parser.run();
}

ElfParser::ElfParser(std::unique_ptr<ElfImage> image)
    : image_(std::move(image)), reader_(image_->bytes_) {}

std::unique_ptr<ElfImage> ElfParser::run() {
  validateHeader();
  parseSegments();
  parseSections();
  parseSymbols();
  return std::move(image_);
}

void ElfParser::validateHeader() {
  if (reader_.size() < kElf64EhdrSize) {
    throw ElfFormatError(StatusCode::kNotElf, "file smaller than ELF64 header");
  }
  if (reader_.u8(kEiMag0) != kElfMag0 || reader_.u8(kEiMag1) != kElfMag1 ||
      reader_.u8(kEiMag2) != kElfMag2 || reader_.u8(kEiMag3) != kElfMag3) {
    throw ElfFormatError(StatusCode::kNotElf, "bad ELF magic");
  }
  if (reader_.u8(kEiClass) != kElfClass64) {
    throw ElfFormatError(StatusCode::kUnsupportedElf,
                         "only ELF64 is supported (this is not a 64-bit ELF)");
  }
  const std::uint8_t data = reader_.u8(kEiData);
  if (data != kElfData2Lsb) {
    throw ElfFormatError(StatusCode::kUnsupportedElf,
                         "only little-endian AArch64 images are supported");
  }
  image_->littleEndian_ = true;

  image_->type_ = reader_.u16(16);
  image_->machine_ = reader_.u16(18);
  if (image_->machine_ != kEmAArch64) {
    throw ElfFormatError(
        StatusCode::kUnsupportedElf,
        "unsupported machine (expected AArch64/183, got " +
            std::to_string(image_->machine_) + ")");
  }

  image_->entryPoint_ = reader_.u64(24);
  programHeaderOffset_ = reader_.u64(32);
  sectionHeaderOffset_ = reader_.u64(40);
  programEntrySize_ = reader_.u16(54);
  programCount_ = reader_.u16(56);
  sectionEntrySize_ = reader_.u16(58);
  sectionCount_ = reader_.u16(60);
  sectionNameIndex_ = reader_.u16(62);
}

void ElfParser::parseSegments() {
  if (programHeaderOffset_ == 0 || programCount_ == 0) {
    return;
  }
  const std::uint64_t stride = strideOf(programEntrySize_, kElf64PhdrSize);
  image_->segments_.reserve(programCount_);
  for (std::uint16_t i = 0; i < programCount_; ++i) {
    const std::uint64_t base = programHeaderOffset_ + static_cast<std::uint64_t>(i) * stride;
    if (!reader_.inBounds(static_cast<std::size_t>(base), kElf64PhdrSize)) {
      logWarn("truncated program header table; stopping at index " + std::to_string(i));
      break;
    }
    ElfSegment segment;
    segment.type_ = reader_.u32(static_cast<std::size_t>(base + 0));
    segment.flags_ = reader_.u32(static_cast<std::size_t>(base + 4));
    segment.fileOffset_ = reader_.u64(static_cast<std::size_t>(base + 8));
    segment.virtualAddress_ = reader_.u64(static_cast<std::size_t>(base + 16));
    segment.fileSize_ = reader_.u64(static_cast<std::size_t>(base + 32));
    segment.memorySize_ = reader_.u64(static_cast<std::size_t>(base + 40));
    image_->segments_.push_back(segment);
  }
}

void ElfParser::parseSections() {
  if (sectionHeaderOffset_ == 0) {
    logInfo("image has no section header table; relying on program headers");
    return;
  }

  std::uint64_t count = sectionCount_;
  std::uint32_t nameIndex = sectionNameIndex_;
  const std::uint64_t stride = strideOf(sectionEntrySize_, kElf64ShdrSize);

  if ((count == 0 || nameIndex == kShnXIndex) &&
      reader_.inBounds(static_cast<std::size_t>(sectionHeaderOffset_), kElf64ShdrSize)) {
    if (count == 0) {
      count = reader_.u64(static_cast<std::size_t>(sectionHeaderOffset_ + 32));
    }
    if (nameIndex == kShnXIndex) {
      nameIndex = reader_.u32(static_cast<std::size_t>(sectionHeaderOffset_ + 40));
    }
  }

  if (count == 0) {
    logInfo("section header table present but empty; relying on program headers");
    return;
  }
  if (count > 1'000'000) {
    throw ElfFormatError(StatusCode::kMalformedElf,
                         "implausible section count: " + std::to_string(count));
  }

  image_->sections_.reserve(static_cast<std::size_t>(count));
  for (std::uint64_t i = 0; i < count; ++i) {
    const std::uint64_t base = sectionHeaderOffset_ + i * stride;
    if (!reader_.inBounds(static_cast<std::size_t>(base), kElf64ShdrSize)) {
      logWarn("truncated section header table; stopping at index " + std::to_string(i));
      break;
    }
    ElfSection section;
    section.type_ = reader_.u32(static_cast<std::size_t>(base + 4));
    section.flags_ = reader_.u64(static_cast<std::size_t>(base + 8));
    section.address_ = reader_.u64(static_cast<std::size_t>(base + 16));
    section.fileOffset_ = reader_.u64(static_cast<std::size_t>(base + 24));
    section.size_ = reader_.u64(static_cast<std::size_t>(base + 32));
    section.link_ = reader_.u32(static_cast<std::size_t>(base + 40));
    section.info_ = reader_.u32(static_cast<std::size_t>(base + 44));
    section.entrySize_ = reader_.u64(static_cast<std::size_t>(base + 56));
    section.name_ = std::to_string(reader_.u32(static_cast<std::size_t>(base + 0)));
    image_->sections_.push_back(section);
  }

  sectionNameIndex_ = nameIndex;
  resolveSectionNames();
}

void ElfParser::resolveSectionNames() {
  auto& sections = image_->sections_;
  if (sectionNameIndex_ >= sections.size()) {
    logWarn("section name string table index out of range; names unavailable");
    for (ElfSection& section : sections) {
      section.name_.clear();
    }
    return;
  }
  const ElfSection& strtab = sections[sectionNameIndex_];
  for (ElfSection& section : sections) {
    std::uint64_t nameOffset = 0;
    try {
      nameOffset = std::stoull(section.name_);
    } catch (const std::exception&) {
      section.name_.clear();
      continue;
    }
    const std::uint64_t at = strtab.fileOffset() + nameOffset;
    if (at < reader_.size()) {
      section.name_ = reader_.stringAt(static_cast<std::size_t>(at));
    } else {
      section.name_.clear();
    }
  }
}

void ElfParser::parseSymbols() {
  bool parsedFromSections = false;
  for (const ElfSection& section : image_->sections_) {
    if (section.type() == kShtSymtab) {
      parseSectionSymbolTable(section, SymbolOrigin::kSymtab);
      parsedFromSections = true;
    } else if (section.type() == kShtDynsym) {
      parseSectionSymbolTable(section, SymbolOrigin::kDynsym);
      parsedFromSections = true;
    }
  }

  if (!parsedFromSections) {
    parseDynamicSymbols();
  }

  logInfo("recovered " + std::to_string(image_->symbols_.size()) + " symbols");
}

void ElfParser::parseSectionSymbolTable(const ElfSection& symtab, SymbolOrigin origin) {
  const std::uint32_t strtabIndex = symtab.link();
  if (strtabIndex >= image_->sections_.size()) {
    logWarn("symbol table has invalid string table link; skipping");
    return;
  }
  const ElfSection& strtab = image_->sections_[strtabIndex];
  const std::uint64_t entrySize = strideOf(symtab.entrySize(), kElf64SymSize);
  if (entrySize == 0) {
    return;
  }
  const std::uint64_t count = symtab.size() / entrySize;

  for (std::uint64_t i = 0; i < count; ++i) {
    const std::uint64_t base = symtab.fileOffset() + i * entrySize;
    if (!reader_.inBounds(static_cast<std::size_t>(base), kElf64SymSize)) {
      break;
    }
    ElfSymbol symbol;
    const std::uint32_t nameOffset = reader_.u32(static_cast<std::size_t>(base + 0));
    symbol.info_ = reader_.u8(static_cast<std::size_t>(base + 4));
    symbol.other_ = reader_.u8(static_cast<std::size_t>(base + 5));
    symbol.sectionIndex_ = reader_.u16(static_cast<std::size_t>(base + 6));
    symbol.value_ = reader_.u64(static_cast<std::size_t>(base + 8));
    symbol.size_ = reader_.u64(static_cast<std::size_t>(base + 16));
    symbol.origin_ = origin;
    const std::uint64_t nameAt = strtab.fileOffset() + nameOffset;
    if (nameOffset != 0 && nameAt < reader_.size()) {
      symbol.name_ = reader_.stringAt(static_cast<std::size_t>(nameAt));
    }
    image_->symbols_.push_back(std::move(symbol));
  }
}

const ElfSegment* ElfParser::findSegment(std::uint32_t type) const {
  for (const ElfSegment& segment : image_->segments_) {
    if (segment.type() == type) {
      return &segment;
    }
  }
  return nullptr;
}

void ElfParser::parseDynamicSymbols() {
  const ElfSegment* dynamic = findSegment(kPtDynamic);
  if (dynamic == nullptr) {
    return;
  }
  logInfo("no symbol sections; parsing PT_DYNAMIC for the dynamic symbol table");

  if (!reader_.inBounds(static_cast<std::size_t>(dynamic->fileOffset()),
                        static_cast<std::size_t>(dynamic->fileSize()))) {
    logWarn("PT_DYNAMIC lies outside the file; cannot recover dynamic symbols");
    return;
  }

  Address symtabVaddr = 0;
  Address strtabVaddr = 0;
  Address hashVaddr = 0;
  Address gnuHashVaddr = 0;
  std::uint64_t strSize = 0;
  std::uint64_t symEntSize = kElf64SymSize;

  const std::uint64_t maxEntries = dynamic->fileSize() / kElf64DynSize;
  for (std::uint64_t i = 0; i < maxEntries; ++i) {
    const std::uint64_t base = dynamic->fileOffset() + i * kElf64DynSize;
    const std::uint64_t tag = reader_.u64(static_cast<std::size_t>(base + 0));
    const std::uint64_t val = reader_.u64(static_cast<std::size_t>(base + 8));
    if (tag == kDtNull) {
      break;
    }
    switch (tag) {
      case kDtSymtab:  symtabVaddr = val; break;
      case kDtStrtab:  strtabVaddr = val; break;
      case kDtStrsz:   strSize = val; break;
      case kDtSyment:  if (val != 0) symEntSize = val; break;
      case kDtHash:    hashVaddr = val; break;
      case kDtGnuHash: gnuHashVaddr = val; break;
      default: break;
    }
  }

  if (symtabVaddr == 0 || strtabVaddr == 0) {
    logWarn("PT_DYNAMIC lacks DT_SYMTAB/DT_STRTAB; cannot recover symbols");
    return;
  }

  std::size_t symAvail = 0;
  const std::uint8_t* symBase = image_->dataAt(symtabVaddr, symAvail);
  std::size_t strAvail = 0;
  const std::uint8_t* strBase = image_->dataAt(strtabVaddr, strAvail);
  if (symBase == nullptr || strBase == nullptr) {
    logWarn("dynamic symbol/string table not backed by file data");
    return;
  }
  if (strSize > 0) {
    strAvail = std::min<std::size_t>(strAvail, static_cast<std::size_t>(strSize));
  }

  const std::uint64_t stride = strideOf(symEntSize, kElf64SymSize);
  std::uint64_t count = dynamicSymbolCount(symtabVaddr, strtabVaddr, hashVaddr,
                                           gnuHashVaddr, stride);
  const std::uint64_t maxByBuffer = symAvail / stride;
  if (count == 0 || count > maxByBuffer) {
    count = maxByBuffer;
  }
  count = std::min<std::uint64_t>(count, kMaxSymbols);

  ByteReader symReader(symBase, symAvail);
  ByteReader strReader(strBase, strAvail);
  for (std::uint64_t i = 0; i < count; ++i) {
    const std::uint64_t base = i * stride;
    if (!symReader.inBounds(static_cast<std::size_t>(base), kElf64SymSize)) {
      break;
    }
    ElfSymbol symbol;
    const std::uint32_t nameOffset = symReader.u32(static_cast<std::size_t>(base + 0));
    symbol.info_ = symReader.u8(static_cast<std::size_t>(base + 4));
    symbol.other_ = symReader.u8(static_cast<std::size_t>(base + 5));
    symbol.sectionIndex_ = symReader.u16(static_cast<std::size_t>(base + 6));
    symbol.value_ = symReader.u64(static_cast<std::size_t>(base + 8));
    symbol.size_ = symReader.u64(static_cast<std::size_t>(base + 16));
    symbol.origin_ = SymbolOrigin::kDynsym;
    if (nameOffset != 0 && nameOffset < strReader.size()) {
      symbol.name_ = strReader.stringAt(nameOffset);
    }
    image_->symbols_.push_back(std::move(symbol));
  }
}

std::uint64_t ElfParser::dynamicSymbolCount(Address symtabVaddr, Address strtabVaddr,
                                            Address hashVaddr, Address gnuHashVaddr,
                                            std::uint64_t symEntSize) const {
  if (hashVaddr != 0) {
    std::size_t avail = 0;
    const std::uint8_t* p = image_->dataAt(hashVaddr, avail);
    if (p != nullptr && avail >= 8) {
      ByteReader r(p, avail);
      const std::uint32_t nchain = r.u32(4);
      if (nchain > 0) {
        return nchain;
      }
    }
  }

  if (gnuHashVaddr != 0) {
    std::size_t avail = 0;
    const std::uint8_t* p = image_->dataAt(gnuHashVaddr, avail);
    if (p != nullptr && avail >= 16) {
      ByteReader r(p, avail);
      const std::uint32_t nbuckets = r.u32(0);
      const std::uint32_t symOffset = r.u32(4);
      const std::uint32_t bloomSize = r.u32(8);
      if (nbuckets > 0 && bloomSize > 0) {
        const std::size_t bucketsOff = 16 + static_cast<std::size_t>(bloomSize) * 8;
        const std::size_t chainOff = bucketsOff + static_cast<std::size_t>(nbuckets) * 4;
        if (r.inBounds(bucketsOff, static_cast<std::size_t>(nbuckets) * 4)) {
          std::uint32_t lastSym = 0;
          for (std::uint32_t b = 0; b < nbuckets; ++b) {
            const std::uint32_t v = r.u32(bucketsOff + static_cast<std::size_t>(b) * 4);
            if (v > lastSym) {
              lastSym = v;
            }
          }
          if (lastSym < symOffset) {
            return symOffset;
          }
          std::size_t chainIndex = lastSym - symOffset;
          while (true) {
            const std::size_t at = chainOff + chainIndex * 4;
            if (!r.inBounds(at, 4)) {
              break;
            }
            const std::uint32_t hash = r.u32(at);
            ++chainIndex;
            if ((hash & 1u) != 0) {
              return symOffset + chainIndex;
            }
          }
        }
      }
    }
  }

  if (symEntSize > 0 && strtabVaddr > symtabVaddr) {
    return (strtabVaddr - symtabVaddr) / symEntSize;
  }
  return 0;
}

}
