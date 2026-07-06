#ifndef FNFINDER_ANALYSIS_FUNCTION_BOUNDS_H_
#define FNFINDER_ANALYSIS_FUNCTION_BOUNDS_H_

#include "common/types.h"

namespace fnfinder::analysis {

struct FunctionBounds {
  Address start = 0;
  Address end = 0;
};

}

#endif
