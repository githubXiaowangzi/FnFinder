#ifndef FNFINDER_COMMON_STRING_UTIL_H_
#define FNFINDER_COMMON_STRING_UTIL_H_

#include <cstdint>
#include <string>

namespace fnfinder::strutil {

std::string toHex(std::uint64_t value, bool withPrefix = true, int minDigits = 0,
                  bool upper = false);

bool startsWith(const std::string& text, const std::string& prefix);
bool endsWith(const std::string& text, const std::string& suffix);

std::string trim(const std::string& text);

}

#endif
