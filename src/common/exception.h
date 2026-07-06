#ifndef FNFINDER_COMMON_EXCEPTION_H_
#define FNFINDER_COMMON_EXCEPTION_H_

#include <stdexcept>
#include <string>
#include <utility>

#include "common/status_code.h"

namespace fnfinder {

class FnFinderError : public std::runtime_error {
 public:
  FnFinderError(StatusCode code, std::string message)
      : std::runtime_error(std::move(message)), code_(code) {}

  StatusCode code() const noexcept { return code_; }

 private:
  StatusCode code_;
};

class IoError : public FnFinderError {
 public:
  explicit IoError(std::string message)
      : FnFinderError(StatusCode::kIoError, std::move(message)) {}
};

class ElfFormatError : public FnFinderError {
 public:
  ElfFormatError(StatusCode code, std::string message)
      : FnFinderError(code, std::move(message)) {}
};

class BufferOverrunError : public FnFinderError {
 public:
  explicit BufferOverrunError(std::string message)
      : FnFinderError(StatusCode::kMalformedElf, std::move(message)) {}
};

class DisassemblerError : public FnFinderError {
 public:
  explicit DisassemblerError(std::string message)
      : FnFinderError(StatusCode::kDisassemblerError, std::move(message)) {}
};

}

#endif
