#ifndef FNFINDER_ANALYSIS_ANALYSIS_RESULT_H_
#define FNFINDER_ANALYSIS_ANALYSIS_RESULT_H_

#include <cstddef>
#include <vector>

#include "analysis/function.h"

namespace fnfinder::analysis {

struct AnalysisResult {
  std::vector<Function> functions;

  std::size_t count() const { return functions.size(); }

  std::size_t namedCount() const {
    std::size_t n = 0;
    for (const Function& f : functions) {
      if (f.hasSymbol) ++n;
    }
    return n;
  }
};

}

#endif
