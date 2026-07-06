#ifndef FNFINDER_SYMBOL_DEMANGLER_H_
#define FNFINDER_SYMBOL_DEMANGLER_H_

#include <string>

namespace fnfinder::symbol {

class IDemangler {
 public:
  virtual ~IDemangler() = default;

  virtual std::string demangle(const std::string& mangled) const = 0;
};

}

#endif
