#ifndef FNFINDER_SYMBOL_NAME_RESOLVER_H_
#define FNFINDER_SYMBOL_NAME_RESOLVER_H_

#include <string>
#include <unordered_map>

#include "common/types.h"
#include "elf/elf_image.h"
#include "symbol/demangler.h"

namespace fnfinder::symbol {

class NameResolver {
 public:
  NameResolver(const elf::ElfImage& image, const IDemangler& demangler,
               bool demanglingEnabled);

  std::string nameFor(Address address) const;

  bool hasSymbol(Address address) const;

  const std::string* rawSymbolName(Address address) const;

 private:
  static int bindingRank(std::uint8_t binding);
  std::string makeSyntheticName(Address address) const;

  const IDemangler& demangler_;
  bool demanglingEnabled_;

  std::unordered_map<Address, std::string> nameByAddress_;
  std::unordered_map<Address, int> rankByAddress_;
};

}

#endif
