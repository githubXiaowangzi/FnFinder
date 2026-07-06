#ifndef FNFINDER_APP_RESULT_PRINTER_H_
#define FNFINDER_APP_RESULT_PRINTER_H_

#include <ostream>

#include "analysis/analysis_result.h"

namespace fnfinder::app {

class ResultPrinter {
 public:
  enum class Format { kText, kJson };

  static void print(std::ostream& out, const analysis::AnalysisResult& result,
                    Format format);

 private:
  static void printText(std::ostream& out, const analysis::AnalysisResult& result);
  static void printJson(std::ostream& out, const analysis::AnalysisResult& result);
};

}

#endif
