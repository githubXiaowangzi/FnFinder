#include "symbol/itanium_demangler.h"

#include <cctype>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/string_util.h"

#if defined(__has_include)
#if __has_include(<cxxabi.h>)
#define FNFINDER_HAVE_CXXABI 1
#include <cxxabi.h>
#endif
#endif

namespace fnfinder::symbol {
namespace {

bool looksMangled(const std::string& name) {
  return strutil::startsWith(name, "_Z") || strutil::startsWith(name, "___Z");
}

#if defined(FNFINDER_HAVE_CXXABI)
std::string demangleWithAbi(const std::string& mangled) {
  int status = 0;
  char* out = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
  if (status != 0 || out == nullptr) {
    std::free(out);
    return {};
  }
  std::string result(out);
  std::free(out);
  return result;
}
#endif

constexpr int kMaxDepth = 256;

const std::unordered_map<char, const char*>& builtinTypes() {
  static const std::unordered_map<char, const char*> kMap = {
      {'v', "void"},        {'w', "wchar_t"},
      {'b', "bool"},        {'c', "char"},
      {'a', "signed char"}, {'h', "unsigned char"},
      {'s', "short"},       {'t', "unsigned short"},
      {'i', "int"},         {'j', "unsigned int"},
      {'l', "long"},        {'m', "unsigned long"},
      {'x', "long long"},   {'y', "unsigned long long"},
      {'n', "__int128"},    {'o', "unsigned __int128"},
      {'f', "float"},       {'d', "double"},
      {'e', "long double"}, {'g', "__float128"},
      {'z', "..."},
  };
  return kMap;
}

const std::unordered_map<std::string, const char*>& operatorNames() {
  static const std::unordered_map<std::string, const char*> kMap = {
      {"nw", "operator new"},   {"na", "operator new[]"},
      {"dl", "operator delete"},{"da", "operator delete[]"},
      {"ps", "operator+"},      {"ng", "operator-"},
      {"ad", "operator&"},      {"de", "operator*"},
      {"co", "operator~"},      {"pl", "operator+"},
      {"mi", "operator-"},      {"ml", "operator*"},
      {"dv", "operator/"},      {"rm", "operator%"},
      {"an", "operator&"},      {"or", "operator|"},
      {"eo", "operator^"},      {"aS", "operator="},
      {"pL", "operator+="},     {"mI", "operator-="},
      {"mL", "operator*="},     {"dV", "operator/="},
      {"rM", "operator%="},     {"aN", "operator&="},
      {"oR", "operator|="},     {"eO", "operator^="},
      {"ls", "operator<<"},     {"rs", "operator>>"},
      {"lS", "operator<<="},    {"rS", "operator>>="},
      {"eq", "operator=="},     {"ne", "operator!="},
      {"lt", "operator<"},      {"gt", "operator>"},
      {"le", "operator<="},     {"ge", "operator>="},
      {"ss", "operator<=>"},    {"nt", "operator!"},
      {"aa", "operator&&"},     {"oo", "operator||"},
      {"pp", "operator++"},     {"mm", "operator--"},
      {"cm", "operator,"},      {"pm", "operator->*"},
      {"pt", "operator->"},     {"cl", "operator()"},
      {"ix", "operator[]"},     {"qu", "operator?"},
  };
  return kMap;
}

class Parser {
 public:
  explicit Parser(const std::string& input) : s_(input) {}

  std::string run() {
    if (!consume("_Z") && !consume("___Z")) {
      return {};
    }
    std::string result = parseEncoding();
    return ok_ ? result : std::string{};
  }

 private:
  struct DepthGuard {
    explicit DepthGuard(Parser& p) : p_(p) {
      if (++p_.depth_ > kMaxDepth) p_.ok_ = false;
    }
    ~DepthGuard() { --p_.depth_; }
    Parser& p_;
  };

  char peek() const { return pos_ < s_.size() ? s_[pos_] : '\0'; }
  char peekAt(std::size_t d) const {
    return pos_ + d < s_.size() ? s_[pos_ + d] : '\0';
  }
  bool eof() const { return pos_ >= s_.size(); }

  bool consume(const char* literal) {
    const std::size_t n = std::char_traits<char>::length(literal);
    if (s_.compare(pos_, n, literal) == 0) {
      pos_ += n;
      return true;
    }
    return false;
  }

  void fail() { ok_ = false; }

  void addSub(const std::string& component) {
    for (const std::string& existing : subs_) {
      if (existing == component) return;
    }
    subs_.push_back(component);
  }

  std::string subAt(std::size_t index) {
    if (index >= subs_.size()) {
      fail();
      return {};
    }
    return subs_[index];
  }

  static bool isBase36(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) || (c >= 'A' && c <= 'Z');
  }
  static int base36Value(char c) {
    return std::isdigit(static_cast<unsigned char>(c)) ? c - '0' : c - 'A' + 10;
  }

  bool canStartType(char c) const {
    if (std::isdigit(static_cast<unsigned char>(c))) return true;
    if (builtinTypes().count(c) != 0) return true;
    switch (c) {
      case 'N': case 'S': case 'P': case 'R': case 'O':
      case 'K': case 'V': case 'r': case 'F': case 'D': case 'T':
        return true;
      default:
        return false;
    }
  }

  std::string parseTemplateParam() {
    consume("T");
    std::size_t index = 0;
    if (peek() == '_') {
      ++pos_;
    } else {
      if (!isBase36(peek())) {
        fail();
        return {};
      }
      while (isBase36(peek())) {
        index = index * 36 + static_cast<std::size_t>(base36Value(peek()));
        ++pos_;
      }
      if (!consume("_")) {
        fail();
        return {};
      }
      index += 1;
    }
    if (index >= templateArgs_.size()) {
      fail();
      return {};
    }
    const std::string resolved = templateArgs_[index];
    addSub(resolved);
    return resolved;
  }

  std::string parseEncoding() {
    std::string name = parseName();
    if (!ok_) return {};

    if (!eof() && peek() != 'E' && canStartType(peek())) {
      const bool templated = lastWasTemplate_;
      std::vector<std::string> types = parseBareFunctionType();
      if (!ok_) return {};
      std::size_t begin = (templated && !types.empty()) ? 1 : 0;
      std::string params;
      for (std::size_t i = begin; i < types.size(); ++i) {
        if (types[i] == "void" && types.size() - begin == 1) break;
        if (!params.empty()) params += ", ";
        params += types[i];
      }
      name += "(" + params + ")";
    }
    return name;
  }

  std::vector<std::string> parseBareFunctionType() {
    std::vector<std::string> types;
    while (!eof() && peek() != 'E' && canStartType(peek())) {
      std::string t = parseType();
      if (!ok_) return {};
      types.push_back(std::move(t));
    }
    return types;
  }

  std::string parseName() {
    lastWasTemplate_ = false;
    const char c = peek();
    if (c == 'N') return parseNestedName(false);
    if (c == 'Z') {
      fail();
      return {};
    }
    if (c == 'S') {
      std::string prefix = parseSubstitution();
      if (!ok_) return {};
      if (lastSubStdNs_ && !eof() &&
          (std::isdigit(static_cast<unsigned char>(peek())) || isOperatorStart())) {
        std::string nested = parseUnqualifiedName();
        if (!ok_) return {};
        prefix += "::" + nested;
        addSub(prefix);
      }
      if (peek() == 'I') {
        prefix += parseTemplateArgs();
        if (!ok_) return {};
        lastWasTemplate_ = true;
        addSub(prefix);
      }
      return prefix;
    }

    std::string name = parseUnqualifiedName();
    if (!ok_) return {};
    if (peek() == 'I') {
      name += parseTemplateArgs();
      if (!ok_) return {};
      lastWasTemplate_ = true;
      addSub(name);
    }
    return name;
  }

  bool isOperatorStart() const {
    if (pos_ + 1 >= s_.size()) return false;
    const std::string code = s_.substr(pos_, 2);
    return operatorNames().count(code) != 0;
  }

  std::string parseNestedName(bool asType) {
    consume("N");
    while (peek() == 'r' || peek() == 'V' || peek() == 'K') ++pos_;
    if (peek() == 'R' || peek() == 'O') ++pos_;

    std::string result;
    std::string lastComponent;
    bool first = true;

    while (!eof() && peek() != 'E') {
      DepthGuard guard(*this);
      if (!ok_) return {};
      const char c = peek();

      if (c == 'I') {
        std::string args = parseTemplateArgs();
        if (!ok_) return {};
        result += args;
        lastWasTemplate_ = true;
        if (peek() != 'E') addSub(result);
        continue;
      }

      const bool isSubstitution = (c == 'S');
      std::string component;
      if (c == 'S') {
        component = parseSubstitution();
        if (!ok_) return {};
      } else if (c == 'C' &&
                 (peekAt(1) == '1' || peekAt(1) == '2' || peekAt(1) == '3' ||
                  peekAt(1) == '4' || peekAt(1) == '5')) {
        pos_ += 2;
        component = lastComponent;
      } else if (c == 'D' &&
                 (peekAt(1) == '0' || peekAt(1) == '1' || peekAt(1) == '2' ||
                  peekAt(1) == '4' || peekAt(1) == '5')) {
        pos_ += 2;
        component = "~" + lastComponent;
      } else {
        component = parseUnqualifiedName();
        if (!ok_) return {};
        lastComponent = component;
      }

      lastWasTemplate_ = false;
      if (first) {
        result = component;
        first = false;
      } else {
        result += "::" + component;
      }
      if (isSubstitution) {
        lastComponent = component;
      }

      if (peek() != 'E' && !isSubstitution) {
        addSub(result);
      }
    }

    if (!consume("E")) {
      fail();
      return {};
    }
    if (asType) {
      addSub(result);
    }
    return result;
  }

  std::string parseUnqualifiedName() {
    const char c = peek();
    if (std::isdigit(static_cast<unsigned char>(c))) {
      return parseSourceName();
    }
    if (pos_ + 1 < s_.size()) {
      const std::string code = s_.substr(pos_, 2);
      auto it = operatorNames().find(code);
      if (it != operatorNames().end()) {
        pos_ += 2;
        return it->second;
      }
    }
    fail();
    return {};
  }

  std::string parseSourceName() {
    std::size_t length = 0;
    if (!std::isdigit(static_cast<unsigned char>(peek()))) {
      fail();
      return {};
    }
    while (std::isdigit(static_cast<unsigned char>(peek()))) {
      length = length * 10 + static_cast<std::size_t>(peek() - '0');
      ++pos_;
      if (length > s_.size()) {
        fail();
        return {};
      }
    }
    if (pos_ + length > s_.size()) {
      fail();
      return {};
    }
    std::string id = s_.substr(pos_, length);
    pos_ += length;
    return id;
  }

  std::string parseSubstitution() {
    lastSubStdNs_ = false;
    if (peek() != 'S') {
      fail();
      return {};
    }
    ++pos_;
    const char c = peek();
    switch (c) {
      case 't': ++pos_; lastSubStdNs_ = true; return "std";
      case 'a': ++pos_; return "std::allocator";
      case 'b': ++pos_; return "std::basic_string";
      case 's': ++pos_; return "std::string";
      case 'i': ++pos_; return "std::istream";
      case 'o': ++pos_; return "std::ostream";
      case 'd': ++pos_; return "std::iostream";
      default: break;
    }
    if (c == '_') {
      ++pos_;
      return subAt(0);
    }
    if (isBase36(c)) {
      std::size_t index = 0;
      while (isBase36(peek())) {
        index = index * 36 + static_cast<std::size_t>(base36Value(peek()));
        ++pos_;
      }
      if (!consume("_")) {
        fail();
        return {};
      }
      return subAt(index + 1);
    }
    fail();
    return {};
  }

  std::string parseTemplateArgs() {
    consume("I");
    std::string out = "<";
    std::vector<std::string> args;
    bool first = true;
    while (!eof() && peek() != 'E') {
      DepthGuard guard(*this);
      if (!ok_) return {};
      std::string arg = parseTemplateArg();
      if (!ok_) return {};
      if (!first) out += ", ";
      out += arg;
      first = false;
      args.push_back(std::move(arg));
    }
    if (!consume("E")) {
      fail();
      return {};
    }
    if (!out.empty() && out.back() == '>') out += ' ';
    out += ">";
    templateArgs_ = std::move(args);
    return out;
  }

  std::string parseTemplateArg() {
    DepthGuard guard(*this);
    if (!ok_) return {};
    const char c = peek();
    if (c == 'L') return parseExprPrimary();
    if (c == 'X') {
      fail();
      return {};
    }
    if (c == 'J') {
      ++pos_;
      std::string out;
      bool first = true;
      while (!eof() && peek() != 'E') {
        std::string t = parseTemplateArg();
        if (!ok_) return {};
        if (!first) out += ", ";
        out += t;
        first = false;
      }
      if (!consume("E")) {
        fail();
        return {};
      }
      return out;
    }
    return parseType();
  }

  std::string parseExprPrimary() {
    consume("L");
    std::string type;
    if (peek() != '_' && canStartType(peek())) {
      type = parseType();
      if (!ok_) return {};
    }
    std::string value;
    while (!eof() && peek() != 'E') {
      value.push_back(peek());
      ++pos_;
    }
    if (!consume("E")) {
      fail();
      return {};
    }
    if (type == "bool") {
      return value == "0" ? "false" : "true";
    }
    if (!value.empty() && value[0] == 'n') {
      value = "-" + value.substr(1);
    }
    return value.empty() ? type : value;
  }

  std::string parseType() {
    DepthGuard guard(*this);
    if (!ok_) return {};
    const char c = peek();

    if (c == 'K' || c == 'V' || c == 'r') {
      std::string qualifiers;
      while (peek() == 'K' || peek() == 'V' || peek() == 'r') {
        switch (peek()) {
          case 'K': qualifiers = " const" + qualifiers; break;
          case 'V': qualifiers = " volatile" + qualifiers; break;
          case 'r': qualifiers = " restrict" + qualifiers; break;
          default: break;
        }
        ++pos_;
      }
      std::string base = parseType();
      if (!ok_) return {};
      std::string result = base + qualifiers;
      addSub(result);
      return result;
    }
    if (c == 'P' || c == 'R' || c == 'O') {
      ++pos_;
      std::string base = parseType();
      if (!ok_) return {};
      const char* suffix = (c == 'P') ? "*" : (c == 'R') ? "&" : "&&";
      std::string result = base + suffix;
      addSub(result);
      return result;
    }
    if (c == 'N') {
      return parseNestedName(true);
    }
    if (c == 'S') {
      std::string result = parseSubstitution();
      if (!ok_) return {};
      if (lastSubStdNs_ && (std::isdigit(static_cast<unsigned char>(peek())) ||
                            isOperatorStart())) {
        result += "::" + parseUnqualifiedName();
        if (!ok_) return {};
        addSub(result);
      }
      if (peek() == 'I') {
        result += parseTemplateArgs();
        if (!ok_) return {};
        addSub(result);
      }
      return result;
    }
    if (c == 'T') {
      return parseTemplateParam();
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      std::string result = parseSourceName();
      if (!ok_) return {};
      if (peek() == 'I') {
        result += parseTemplateArgs();
        if (!ok_) return {};
      }
      addSub(result);
      return result;
    }
    if (c == 'F') {
      return parseFunctionType();
    }
    if (c == 'D') {
      return parseExtendedBuiltin();
    }
    auto it = builtinTypes().find(c);
    if (it != builtinTypes().end()) {
      ++pos_;
      return it->second;
    }
    fail();
    return {};
  }

  std::string parseExtendedBuiltin() {
    consume("D");
    const char c = peek();
    ++pos_;
    switch (c) {
      case 'n': return "std::nullptr_t";
      case 'i': return "char32_t";
      case 's': return "char16_t";
      case 'a': return "auto";
      case 'c': return "decltype(auto)";
      case 'h': return "_Float16";
      case 'f': return "_Decimal32";
      case 'd': return "_Decimal64";
      case 'e': return "_Decimal128";
      default: fail(); return {};
    }
  }

  std::string parseFunctionType() {
    consume("F");
    if (peek() == 'Y') ++pos_;
    std::string ret = parseType();
    if (!ok_) return {};
    std::string params;
    while (!eof() && peek() != 'E') {
      std::string t = parseType();
      if (!ok_) return {};
      if (t == "void" && params.empty() && peek() == 'E') break;
      if (!params.empty()) params += ", ";
      params += t;
    }
    if (!consume("E")) {
      fail();
      return {};
    }
    std::string result = ret + " (" + params + ")";
    addSub(result);
    return result;
  }

  const std::string& s_;
  std::size_t pos_ = 0;
  bool ok_ = true;
  int depth_ = 0;
  bool lastWasTemplate_ = false;
  bool lastSubStdNs_ = false;
  std::vector<std::string> subs_;
  std::vector<std::string> templateArgs_;
};

}

std::string demangleItaniumFallback(const std::string& mangled) {
  if (!looksMangled(mangled)) {
    return {};
  }
  Parser parser(mangled);
  return parser.run();
}

std::string ItaniumDemangler::demangle(const std::string& mangled) const {
  if (mangled.empty() || !looksMangled(mangled)) {
    return mangled;
  }

#if defined(FNFINDER_HAVE_CXXABI)
  std::string viaAbi = demangleWithAbi(mangled);
  if (!viaAbi.empty()) {
    return viaAbi;
  }
#endif

  std::string viaFallback = demangleItaniumFallback(mangled);
  if (!viaFallback.empty()) {
    return viaFallback;
  }
  return mangled;
}

}
