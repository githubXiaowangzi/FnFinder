#ifndef FNFINDER_ANALYSIS_ANALYSIS_OPTIONS_H_
#define FNFINDER_ANALYSIS_ANALYSIS_OPTIONS_H_

#include <cstddef>

namespace fnfinder::analysis {

struct AnalysisOptions {
  bool useSymbols = true;
  bool useEhFrame = true;
  bool useEntryPoint = true;
  bool useRecursive = true;
  bool useLinearSweep = true;

  bool demangle = true;

  std::size_t maxInstructions = 8'000'000;
};

}

#endif
