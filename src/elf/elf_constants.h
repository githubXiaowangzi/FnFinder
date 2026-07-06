#ifndef FNFINDER_ELF_ELF_CONSTANTS_H_
#define FNFINDER_ELF_ELF_CONSTANTS_H_

#include <cstddef>
#include <cstdint>

namespace fnfinder::elf {

enum EIdent : std::size_t {
  kEiMag0 = 0,
  kEiMag1 = 1,
  kEiMag2 = 2,
  kEiMag3 = 3,
  kEiClass = 4,
  kEiData = 5,
  kEiVersion = 6,
  kEiNIdent = 16,
};

inline constexpr std::uint8_t kElfMag0 = 0x7f;
inline constexpr std::uint8_t kElfMag1 = 'E';
inline constexpr std::uint8_t kElfMag2 = 'L';
inline constexpr std::uint8_t kElfMag3 = 'F';

inline constexpr std::uint8_t kElfClass32 = 1;
inline constexpr std::uint8_t kElfClass64 = 2;

inline constexpr std::uint8_t kElfData2Lsb = 1;
inline constexpr std::uint8_t kElfData2Msb = 2;

inline constexpr std::uint16_t kEmAArch64 = 183;

inline constexpr std::uint16_t kEtExec = 2;
inline constexpr std::uint16_t kEtDyn = 3;

inline constexpr std::size_t kElf64EhdrSize = 64;
inline constexpr std::size_t kElf64ShdrSize = 64;
inline constexpr std::size_t kElf64PhdrSize = 56;
inline constexpr std::size_t kElf64SymSize = 24;

inline constexpr std::uint32_t kShtNull = 0;
inline constexpr std::uint32_t kShtProgbits = 1;
inline constexpr std::uint32_t kShtSymtab = 2;
inline constexpr std::uint32_t kShtStrtab = 3;
inline constexpr std::uint32_t kShtNobits = 8;
inline constexpr std::uint32_t kShtDynsym = 11;

inline constexpr std::uint64_t kShfWrite = 0x1;
inline constexpr std::uint64_t kShfAlloc = 0x2;
inline constexpr std::uint64_t kShfExecinstr = 0x4;

inline constexpr std::uint32_t kPtLoad = 1;
inline constexpr std::uint32_t kPtDynamic = 2;
inline constexpr std::uint32_t kPtGnuEhFrame = 0x6474e550;

inline constexpr std::uint32_t kPfExec = 0x1;
inline constexpr std::uint32_t kPfWrite = 0x2;
inline constexpr std::uint32_t kPfRead = 0x4;

inline constexpr std::size_t kElf64DynSize = 16;
inline constexpr std::uint64_t kDtNull = 0;
inline constexpr std::uint64_t kDtHash = 4;
inline constexpr std::uint64_t kDtStrtab = 5;
inline constexpr std::uint64_t kDtSymtab = 6;
inline constexpr std::uint64_t kDtStrsz = 10;
inline constexpr std::uint64_t kDtSyment = 11;
inline constexpr std::uint64_t kDtGnuHash = 0x6ffffef5;

inline constexpr std::uint16_t kShnUndef = 0;
inline constexpr std::uint16_t kShnLoReserve = 0xff00;
inline constexpr std::uint16_t kShnXIndex = 0xffff;

inline constexpr std::uint8_t kSttNotype = 0;
inline constexpr std::uint8_t kSttObject = 1;
inline constexpr std::uint8_t kSttFunc = 2;
inline constexpr std::uint8_t kSttSection = 3;
inline constexpr std::uint8_t kSttGnuIfunc = 10;

inline constexpr std::uint8_t kStbLocal = 0;
inline constexpr std::uint8_t kStbGlobal = 1;
inline constexpr std::uint8_t kStbWeak = 2;

inline constexpr const char* kSecSymtab = ".symtab";
inline constexpr const char* kSecDynsym = ".dynsym";
inline constexpr const char* kSecText = ".text";
inline constexpr const char* kSecEhFrame = ".eh_frame";
inline constexpr const char* kSecEhFrameHdr = ".eh_frame_hdr";

}

#endif
