#ifndef FNFINDER_SYMBOL_ITANIUM_DEMANGLER_H_
#define FNFINDER_SYMBOL_ITANIUM_DEMANGLER_H_

#include <string>

#include "symbol/demangler.h"

namespace fnfinder::symbol {

class ItaniumDemangler : public IDemangler {
 public:
  std::string demangle(const std::string& mangled) const override;
};

std::string demangleItaniumFallback(const std::string& mangled);

}

#endif
