#include "symbol/name_resolver.h"

#include "common/string_util.h"
#include "elf/elf_constants.h"
#include "elf/elf_symbol.h"

namespace fnfinder::symbol {

namespace {
constexpr const char* kSyntheticPrefix = "sub_";
}

NameResolver::NameResolver(const elf::ElfImage& image, const IDemangler& demangler,
                           bool demanglingEnabled)
    : demangler_(demangler), demanglingEnabled_(demanglingEnabled) {
  for (const elf::ElfSymbol& sym : image.symbols()) {
    if (!sym.isFunction() || !sym.isDefined() || !sym.hasName()) {
      continue;
    }
    const int rank = bindingRank(sym.binding());
    auto it = rankByAddress_.find(sym.value());
    if (it == rankByAddress_.end() || rank > it->second) {
      nameByAddress_[sym.value()] = sym.name();
      rankByAddress_[sym.value()] = rank;
    }
  }
}

int NameResolver::bindingRank(std::uint8_t binding) {
  switch (binding) {
    case elf::kStbGlobal: return 3;
    case elf::kStbWeak:   return 2;
    case elf::kStbLocal:  return 1;
    default:              return 0;
  }
}

bool NameResolver::hasSymbol(Address address) const {
  return nameByAddress_.find(address) != nameByAddress_.end();
}

const std::string* NameResolver::rawSymbolName(Address address) const {
  auto it = nameByAddress_.find(address);
  return it == nameByAddress_.end() ? nullptr : &it->second;
}

std::string NameResolver::makeSyntheticName(Address address) const {
  return kSyntheticPrefix +
         strutil::toHex(address, false, 0, true);
}

std::string NameResolver::nameFor(Address address) const {
  const std::string* raw = rawSymbolName(address);
  if (raw == nullptr) {
    return makeSyntheticName(address);
  }
  if (demanglingEnabled_) {
    return demangler_.demangle(*raw);
  }
  return *raw;
}

}
