#include "common/string_util.h"

#include <algorithm>
#include <cctype>

namespace fnfinder::strutil {

std::string toHex(std::uint64_t value, bool withPrefix, int minDigits, bool upper) {
  static const char* kLower = "0123456789abcdef";
  static const char* kUpper = "0123456789ABCDEF";
  const char* digits = upper ? kUpper : kLower;

  std::string out;
  if (value == 0) {
    out = "0";
  } else {
    while (value != 0) {
      out.push_back(digits[value & 0xf]);
      value >>= 4;
    }
    std::reverse(out.begin(), out.end());
  }
  if (static_cast<int>(out.size()) < minDigits) {
    out.insert(out.begin(), static_cast<std::size_t>(minDigits) - out.size(), '0');
  }
  if (withPrefix) {
    out.insert(0, "0x");
  }
  return out;
}

bool startsWith(const std::string& text, const std::string& prefix) {
  return text.size() >= prefix.size() &&
         text.compare(0, prefix.size(), prefix) == 0;
}

bool endsWith(const std::string& text, const std::string& suffix) {
  return text.size() >= suffix.size() &&
         text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string trim(const std::string& text) {
  const auto notSpace = [](unsigned char c) { return !std::isspace(c); };
  auto begin = std::find_if(text.begin(), text.end(), notSpace);
  auto end = std::find_if(text.rbegin(), text.rend(), notSpace).base();
  if (begin >= end) {
    return {};
  }
  return std::string(begin, end);
}

}
