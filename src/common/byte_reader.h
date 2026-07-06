#ifndef FNFINDER_COMMON_BYTE_READER_H_
#define FNFINDER_COMMON_BYTE_READER_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace fnfinder {

class ByteReader {
 public:
  ByteReader(const std::uint8_t* data, std::size_t size);
  explicit ByteReader(const std::vector<std::uint8_t>& data);

  std::size_t size() const { return size_; }
  const std::uint8_t* data() const { return data_; }

  bool inBounds(std::size_t offset, std::size_t length) const;

  std::uint8_t  u8(std::size_t offset) const;
  std::uint16_t u16(std::size_t offset) const;
  std::uint32_t u32(std::size_t offset) const;
  std::uint64_t u64(std::size_t offset) const;

  const std::uint8_t* bytes(std::size_t offset, std::size_t length) const;

  std::string stringAt(std::size_t offset) const;

  std::size_t position() const { return position_; }
  std::size_t remaining() const { return size_ - position_; }
  void seek(std::size_t position);
  void skip(std::size_t count);

  std::uint8_t  readU8();
  std::uint16_t readU16();
  std::uint32_t readU32();
  std::uint64_t readU64();

  std::int8_t   readS8();
  std::int16_t  readS16();
  std::int32_t  readS32();
  std::int64_t  readS64();

  std::uint64_t readUleb128();
  std::int64_t  readSleb128();

  std::string readCString();

 private:
  void ensure(std::size_t offset, std::size_t length) const;

  const std::uint8_t* data_;
  std::size_t size_;
  std::size_t position_ = 0;
};

}

#endif
