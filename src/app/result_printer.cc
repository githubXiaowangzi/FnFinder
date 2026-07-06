#include "app/result_printer.h"

#include <iomanip>
#include <string>

#include "common/string_util.h"

namespace fnfinder::app {

namespace {

std::string hexAddress(Address addr) {
  return strutil::toHex(addr, true, 16, false);
}

std::string jsonEscape(const std::string& text) {
  std::string out;
  out.reserve(text.size() + 8);
  for (char c : text) {
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          out += "\\u";
          out += strutil::toHex(static_cast<unsigned char>(c), false, 4, false);
        } else {
          out += c;
        }
    }
  }
  return out;
}

}

void ResultPrinter::print(std::ostream& out, const analysis::AnalysisResult& result,
                          Format format) {
  switch (format) {
    case Format::kText: printText(out, result); break;
    case Format::kJson: printJson(out, result); break;
  }
}

void ResultPrinter::printText(std::ostream& out, const analysis::AnalysisResult& result) {
  out << "# FnFinder: " << result.count() << " function(s), " << result.namedCount()
      << " named by symbols\n\n";
  out << std::left << std::setw(20) << "START" << std::setw(20) << "END"
      << std::setw(20) << "LAST" << std::right << std::setw(10) << "SIZE" << "  "
      << std::left << "NAME" << '\n';

  for (const analysis::Function& fn : result.functions) {
    out << std::left << std::setw(20) << hexAddress(fn.start) << std::setw(20)
        << hexAddress(fn.end) << std::setw(20) << hexAddress(fn.lastInstruction)
        << std::right << std::setw(10) << fn.size << "  " << fn.name << '\n';
  }
}

void ResultPrinter::printJson(std::ostream& out, const analysis::AnalysisResult& result) {
  out << "{\n  \"count\": " << result.count() << ",\n  \"functions\": [\n";
  for (std::size_t i = 0; i < result.functions.size(); ++i) {
    const analysis::Function& fn = result.functions[i];
    out << "    {"
        << "\"name\": \"" << jsonEscape(fn.name) << "\", "
        << "\"start\": \"" << hexAddress(fn.start) << "\", "
        << "\"end\": \"" << hexAddress(fn.end) << "\", "
        << "\"last\": \"" << hexAddress(fn.lastInstruction) << "\", "
        << "\"size\": " << fn.size << ", "
        << "\"has_symbol\": " << (fn.hasSymbol ? "true" : "false") << "}";
    out << (i + 1 < result.functions.size() ? ",\n" : "\n");
  }
  out << "  ]\n}\n";
}

}
