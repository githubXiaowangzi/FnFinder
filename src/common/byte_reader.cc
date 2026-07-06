#include "common/byte_reader.h"

#include "common/exception.h"
#include "common/string_util.h"

namespace fnfinder {

ByteReader::ByteReader(const std::uint8_t* data, std::size_t size)
    : data_(data), size_(size) {}

ByteReader::ByteReader(const std::vector<std::uint8_t>& data)
    : data_(data.data()), size_(data.size()) {}

bool ByteReader::inBounds(std::size_t offset, std::size_t length) const {
  if (offset > size_) {
    return false;
  }
  return length <= size_ - offset;
}

void ByteReader::ensure(std::size_t offset, std::size_t length) const {
  if (!inBounds(offset, length)) {
    throw BufferOverrunError(
        "byte read out of bounds: offset=" + strutil::toHex(offset) +
        " length=" + std::to_string(length) +
        " size=" + std::to_string(size_));
  }
}

std::uint8_t ByteReader::u8(std::size_t offset) const {
  ensure(offset, 1);
  return data_[offset];
}

std::uint16_t ByteReader::u16(std::size_t offset) const {
  ensure(offset, 2);
  return static_cast<std::uint16_t>(data_[offset]) |
         (static_cast<std::uint16_t>(data_[offset + 1]) << 8);
}

std::uint32_t ByteReader::u32(std::size_t offset) const {
  ensure(offset, 4);
  return static_cast<std::uint32_t>(data_[offset]) |
         (static_cast<std::uint32_t>(data_[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(data_[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(data_[offset + 3]) << 24);
}

std::uint64_t ByteReader::u64(std::size_t offset) const {
  ensure(offset, 8);
  std::uint64_t value = 0;
  for (int i = 7; i >= 0; --i) {
    value = (value << 8) | data_[offset + static_cast<std::size_t>(i)];
  }
  return value;
}

const std::uint8_t* ByteReader::bytes(std::size_t offset, std::size_t length) const {
  ensure(offset, length);
  return data_ + offset;
}

std::string ByteReader::stringAt(std::size_t offset) const {
  ensure(offset, 0);
  std::size_t end = offset;
  while (end < size_ && data_[end] != 0) {
    ++end;
  }
  return std::string(reinterpret_cast<const char*>(data_ + offset), end - offset);
}

void ByteReader::seek(std::size_t position) {
  if (position > size_) {
    throw BufferOverrunError("seek past end: position=" + std::to_string(position) +
                             " size=" + std::to_string(size_));
  }
  position_ = position;
}

void ByteReader::skip(std::size_t count) { seek(position_ + count); }

std::uint8_t ByteReader::readU8() {
  std::uint8_t value = u8(position_);
  position_ += 1;
  return value;
}

std::uint16_t ByteReader::readU16() {
  std::uint16_t value = u16(position_);
  position_ += 2;
  return value;
}

std::uint32_t ByteReader::readU32() {
  std::uint32_t value = u32(position_);
  position_ += 4;
  return value;
}

std::uint64_t ByteReader::readU64() {
  std::uint64_t value = u64(position_);
  position_ += 8;
  return value;
}

std::int8_t ByteReader::readS8() { return static_cast<std::int8_t>(readU8()); }
std::int16_t ByteReader::readS16() { return static_cast<std::int16_t>(readU16()); }
std::int32_t ByteReader::readS32() { return static_cast<std::int32_t>(readU32()); }
std::int64_t ByteReader::readS64() { return static_cast<std::int64_t>(readU64()); }

std::uint64_t ByteReader::readUleb128() {
  std::uint64_t result = 0;
  int shift = 0;
  while (true) {
    std::uint8_t byte = readU8();
    if (shift < 64) {
      result |= static_cast<std::uint64_t>(byte & 0x7f) << shift;
      shift += 7;
    }
    if ((byte & 0x80) == 0) {
      break;
    }
  }
  return result;
}

std::int64_t ByteReader::readSleb128() {
  std::uint64_t result = 0;
  int shift = 0;
  std::uint8_t byte = 0;
  do {
    byte = readU8();
    if (shift < 64) {
      result |= static_cast<std::uint64_t>(byte & 0x7f) << shift;
      shift += 7;
    }
  } while ((byte & 0x80) != 0);

  if (shift < 64 && (byte & 0x40) != 0) {
    result |= ~static_cast<std::uint64_t>(0) << shift;
  }
  return static_cast<std::int64_t>(result);
}

std::string ByteReader::readCString() {
  std::string value = stringAt(position_);
  position_ += value.size();
  if (position_ < size_ && data_[position_] == 0) {
    position_ += 1;
  }
  return value;
}

}
