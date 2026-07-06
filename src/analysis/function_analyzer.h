#ifndef FNFINDER_ANALYSIS_FUNCTION_ANALYZER_H_
#define FNFINDER_ANALYSIS_FUNCTION_ANALYZER_H_

#include "analysis/analysis_options.h"
#include "analysis/analysis_result.h"
#include "disasm/disassembler.h"
#include "elf/elf_image.h"
#include "symbol/demangler.h"

namespace fnfinder::analysis {

class FunctionAnalyzer {
 public:
  FunctionAnalyzer(const elf::ElfImage& image, disasm::IDisassembler& disassembler,
                   const symbol::IDemangler& demangler);

  AnalysisResult analyze(const AnalysisOptions& options = AnalysisOptions{}) const;

 private:
  const elf::ElfImage& image_;
  disasm::IDisassembler& disassembler_;
  const symbol::IDemangler& demangler_;
};

}

#endif
