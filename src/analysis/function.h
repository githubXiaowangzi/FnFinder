#ifndef FNFINDER_ANALYSIS_FUNCTION_H_
#define FNFINDER_ANALYSIS_FUNCTION_H_

#include <cstdint>
#include <string>

#include "common/types.h"

namespace fnfinder::analysis {

struct Function {
  Address start = 0;
  Address end = 0;
  Address lastInstruction = 0;
  std::uint64_t size = 0;
  std::string name;
  bool hasSymbol = false;
};

}

#endif
