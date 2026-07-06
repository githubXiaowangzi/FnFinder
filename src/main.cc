#include <exception>
#include <iostream>
#include <string>

#include "analysis/analysis_options.h"
#include "analysis/function_analyzer.h"
#include "app/result_printer.h"
#include "common/exception.h"
#include "common/logger.h"
#include "disasm/capstone_disassembler.h"
#include "elf/elf_parser.h"
#include "symbol/itanium_demangler.h"

namespace {

using fnfinder::analysis::AnalysisOptions;
using fnfinder::app::ResultPrinter;

struct CommandLine {
  std::string path;
  AnalysisOptions options;
  ResultPrinter::Format format = ResultPrinter::Format::kText;
  bool showHelp = false;
  bool valid = true;
};

void printUsage(const char* program) {
  std::cout
      << "FnFinder - AArch64 ELF function analyser\n\n"
      << "Usage: " << program << " <elf-file> [options]\n\n"
      << "Options:\n"
      << "  --format=text|json   Output format (default: text).\n"
      << "  --no-symbols         Do not seed from function symbols.\n"
      << "  --no-eh-frame        Do not use .eh_frame / PT_GNU_EH_FRAME.\n"
      << "  --no-entry           Do not seed from the ELF entry point.\n"
      << "  --no-recursive       Disable recursive-descent traversal.\n"
      << "  --no-demangle        Keep raw (mangled) C++ symbol names.\n"
      << "  -v, --verbose        Verbose (debug) logging to stderr.\n"
      << "  -q, --quiet          Errors only.\n"
      << "  -h, --help           Show this help.\n";
}

CommandLine parseCommandLine(int argc, char** argv) {
  CommandLine cli;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") {
      cli.showHelp = true;
    } else if (arg == "--format=text") {
      cli.format = ResultPrinter::Format::kText;
    } else if (arg == "--format=json") {
      cli.format = ResultPrinter::Format::kJson;
    } else if (arg == "--no-symbols") {
      cli.options.useSymbols = false;
    } else if (arg == "--no-eh-frame") {
      cli.options.useEhFrame = false;
    } else if (arg == "--no-entry") {
      cli.options.useEntryPoint = false;
    } else if (arg == "--no-recursive") {
      cli.options.useRecursive = false;
    } else if (arg == "--no-demangle") {
      cli.options.demangle = false;
    } else if (arg == "-v" || arg == "--verbose") {
      fnfinder::Logger::instance().setLevel(fnfinder::LogLevel::kDebug);
    } else if (arg == "-q" || arg == "--quiet") {
      fnfinder::Logger::instance().setLevel(fnfinder::LogLevel::kError);
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "Unknown option: " << arg << "\n";
      cli.valid = false;
    } else if (cli.path.empty()) {
      cli.path = arg;
    } else {
      std::cerr << "Unexpected argument: " << arg << "\n";
      cli.valid = false;
    }
  }
  return cli;
}

}

int main(int argc, char** argv) {
  const CommandLine cli = parseCommandLine(argc, argv);

  if (cli.showHelp) {
    printUsage(argv[0]);
    return 0;
  }
  if (!cli.valid) {
    return 2;
  }
  if (cli.path.empty()) {
    printUsage(argv[0]);
    return 2;
  }

  try {
    auto image = fnfinder::elf::ElfParser::parseFile(cli.path);

    fnfinder::disasm::CapstoneDisassembler disassembler;
    fnfinder::symbol::ItaniumDemangler demangler;
    fnfinder::analysis::FunctionAnalyzer analyzer(*image, disassembler, demangler);

    const fnfinder::analysis::AnalysisResult result = analyzer.analyze(cli.options);
    ResultPrinter::print(std::cout, result, cli.format);
    return 0;
  } catch (const fnfinder::FnFinderError& e) {
    std::cerr << "error [" << fnfinder::toString(e.code()) << "]: " << e.what() << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
