#include "analysis/eh_frame_parser.h"

#include <algorithm>
#include <map>
#include <string>

#include "common/exception.h"
#include "common/logger.h"
#include "common/string_util.h"
#include "elf/elf_constants.h"

namespace fnfinder::analysis {
namespace {

constexpr std::uint8_t kPeOmit = 0xff;
constexpr std::uint8_t kPeFormatMask = 0x0f;
constexpr std::uint8_t kPeApplicationMask = 0x70;
constexpr std::uint8_t kPeIndirect = 0x80;

constexpr std::uint8_t kPeAbsptr = 0x00;
constexpr std::uint8_t kPeUleb128 = 0x01;
constexpr std::uint8_t kPeUdata2 = 0x02;
constexpr std::uint8_t kPeUdata4 = 0x03;
constexpr std::uint8_t kPeUdata8 = 0x04;
constexpr std::uint8_t kPeSleb128 = 0x09;
constexpr std::uint8_t kPeSdata2 = 0x0a;
constexpr std::uint8_t kPeSdata4 = 0x0b;
constexpr std::uint8_t kPeSdata8 = 0x0c;

constexpr std::uint8_t kPeAbsolute = 0x00;
constexpr std::uint8_t kPePcrel = 0x10;
constexpr std::uint8_t kPeDatarel = 0x30;

constexpr std::uint32_t kDwarf64Marker = 0xffffffffu;

constexpr std::size_t kEhFrameHdrPrologue = 4;

}

EhFrameParser::EhFrameParser(const elf::ElfImage& image) : image_(image) {}

EhFrameParser::Blob EhFrameParser::locateEhFrameHdr() const {
  Blob blob;
  if (const elf::ElfSection* section = image_.findSection(elf::kSecEhFrameHdr)) {
    if (section->hasFileData()) {
      std::size_t available = 0;
      const std::uint8_t* data = image_.dataAt(section->address(), available);
      if (data != nullptr) {
        blob.vaddr = section->address();
        blob.data = data;
        blob.size = static_cast<std::size_t>(
            std::min<std::uint64_t>(section->size(), available));
        return blob;
      }
    }
  }
  for (const elf::ElfSegment& segment : image_.segments()) {
    if (segment.type() != elf::kPtGnuEhFrame) {
      continue;
    }
    std::size_t available = 0;
    const std::uint8_t* data = image_.dataAt(segment.virtualAddress(), available);
    if (data != nullptr) {
      blob.vaddr = segment.virtualAddress();
      blob.data = data;
      blob.size = static_cast<std::size_t>(
          std::min<std::uint64_t>(segment.fileSize(), available));
      return blob;
    }
  }
  return blob;
}

EhFrameParser::Blob EhFrameParser::locateEhFrame(const Blob& ehFrameHdr) const {
  Blob blob;
  if (const elf::ElfSection* section = image_.findSection(elf::kSecEhFrame)) {
    if (section->hasFileData()) {
      std::size_t available = 0;
      const std::uint8_t* data = image_.dataAt(section->address(), available);
      if (data != nullptr) {
        blob.vaddr = section->address();
        blob.data = data;
        blob.size = static_cast<std::size_t>(
            std::min<std::uint64_t>(section->size(), available));
        return blob;
      }
    }
  }

  if (!ehFrameHdr.valid() || ehFrameHdr.size < kEhFrameHdrPrologue) {
    return blob;
  }
  try {
    ByteReader reader(ehFrameHdr.data, ehFrameHdr.size);
    const std::uint8_t version = reader.readU8();
    const std::uint8_t ehFramePtrEnc = reader.readU8();
    reader.readU8();
    reader.readU8();
    if (version != 1) {
      logWarn("unexpected .eh_frame_hdr version " + std::to_string(version));
    }
    std::uint64_t ehFramePtr = 0;
    if (!decodePointer(reader, ehFramePtrEnc, ehFrameHdr.vaddr, true,
                       ehFramePtr)) {
      return blob;
    }
    std::size_t available = 0;
    const std::uint8_t* data = image_.dataAt(ehFramePtr, available);
    if (data != nullptr) {
      blob.vaddr = ehFramePtr;
      blob.data = data;
      blob.size = available;
    }
  } catch (const FnFinderError& e) {
    logWarn(std::string("failed to locate .eh_frame via header: ") + e.what());
  }
  return blob;
}

bool EhFrameParser::decodePointer(ByteReader& reader, std::uint8_t encoding,
                                  Address blobVaddr, bool applyBase,
                                  std::uint64_t& out) const {
  out = 0;
  if (encoding == kPeOmit) {
    return false;
  }

  const std::uint8_t format = encoding & kPeFormatMask;
  const std::uint8_t application = encoding & kPeApplicationMask;
  const std::size_t fieldPos = reader.position();

  std::uint64_t raw = 0;
  switch (format) {
    case kPeAbsptr:  raw = reader.readU64(); break;
    case kPeUleb128: raw = reader.readUleb128(); break;
    case kPeUdata2:  raw = reader.readU16(); break;
    case kPeUdata4:  raw = reader.readU32(); break;
    case kPeUdata8:  raw = reader.readU64(); break;
    case kPeSleb128: raw = static_cast<std::uint64_t>(reader.readSleb128()); break;
    case kPeSdata2:
      raw = static_cast<std::uint64_t>(static_cast<std::int64_t>(reader.readS16()));
      break;
    case kPeSdata4:
      raw = static_cast<std::uint64_t>(static_cast<std::int64_t>(reader.readS32()));
      break;
    case kPeSdata8: raw = reader.readU64(); break;
    default:
      return false;
  }

  if ((encoding & kPeIndirect) != 0) {
    return false;
  }

  if (!applyBase) {
    out = raw;
    return true;
  }

  std::uint64_t base = 0;
  switch (application) {
    case kPeAbsolute: base = 0; break;
    case kPePcrel:    base = blobVaddr + fieldPos; break;
    case kPeDatarel:  base = blobVaddr; break;
    default:          base = 0; break;
  }
  out = base + raw;
  return true;
}

std::vector<EhFrameFunction> EhFrameParser::parse() {
  std::vector<EhFrameFunction> functions;

  const Blob ehFrameHdr = locateEhFrameHdr();
  const Blob ehFrame = locateEhFrame(ehFrameHdr);
  if (!ehFrame.valid()) {
    logInfo("no usable .eh_frame; skipping unwind-based discovery");
    return functions;
  }

  std::map<std::size_t, std::uint8_t> cieEncoding;

  try {
    ByteReader reader(ehFrame.data, ehFrame.size);
    while (reader.remaining() >= 4) {
      const std::size_t entryStart = reader.position();
      std::uint64_t length = reader.readU32();
      bool dwarf64 = false;
      if (length == kDwarf64Marker) {
        length = reader.readU64();
        dwarf64 = true;
      }
      if (length == 0) {
        break;
      }

      const std::size_t idFieldPos = reader.position();
      const std::size_t entryEnd = idFieldPos + static_cast<std::size_t>(length);
      if (length > ehFrame.size || entryEnd > ehFrame.size || entryEnd < idFieldPos) {
        logWarn("truncated/invalid .eh_frame entry; stopping");
        break;
      }

      const std::uint64_t id = dwarf64 ? reader.readU64() : reader.readU32();

      if (id == 0) {
        std::uint8_t fdeEncoding = kPeAbsptr;
        const std::uint8_t version = reader.readU8();
        const std::string augmentation = reader.readCString();
        if (version >= 4) {
          reader.readU8();
          reader.readU8();
        }
        reader.readUleb128();
        reader.readSleb128();
        if (version == 1) {
          reader.readU8();
        } else {
          reader.readUleb128();
        }
        if (!augmentation.empty() && augmentation[0] == 'z') {
          const std::uint64_t augLen = reader.readUleb128();
          const std::size_t augEnd = reader.position() + static_cast<std::size_t>(augLen);
          for (std::size_t k = 1; k < augmentation.size(); ++k) {
            const char a = augmentation[k];
            if (a == 'R') {
              fdeEncoding = reader.readU8();
            } else if (a == 'P') {
              const std::uint8_t personalityEnc = reader.readU8();
              std::uint64_t ignored = 0;
              decodePointer(reader, personalityEnc, ehFrame.vaddr, true, ignored);
            } else if (a == 'L') {
              reader.readU8();
            } else if (a == 'S') {
            } else {
              break;
            }
          }
          reader.seek(augEnd);
        }
        cieEncoding[entryStart] = fdeEncoding;
      } else {
        const std::size_t cieStart = idFieldPos - static_cast<std::size_t>(id);
        auto encIt = cieEncoding.find(cieStart);
        const std::uint8_t encoding =
            encIt != cieEncoding.end() ? encIt->second : kPeAbsptr;

        std::uint64_t pcBegin = 0;
        std::uint64_t pcRange = 0;
        if (decodePointer(reader, encoding, ehFrame.vaddr, true, pcBegin) &&
            decodePointer(reader, encoding, ehFrame.vaddr, false, pcRange)) {
          if (pcRange > 0) {
            EhFrameFunction fn;
            fn.start = pcBegin;
            fn.size = pcRange;
            functions.push_back(fn);
          }
        }
      }

      reader.seek(entryEnd);
    }
  } catch (const FnFinderError& e) {
    logWarn(std::string("stopped .eh_frame parsing early: ") + e.what());
  }

  logInfo("recovered " + std::to_string(functions.size()) +
          " function range(s) from unwind info");
  return functions;
}

}
