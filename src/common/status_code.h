#ifndef FNFINDER_COMMON_STATUS_CODE_H_
#define FNFINDER_COMMON_STATUS_CODE_H_

namespace fnfinder {

enum class StatusCode {
  kOk = 0,
  kInvalidArgument,
  kFileNotFound,
  kIoError,
  kNotElf,
  kUnsupportedElf,
  kMalformedElf,
  kDisassemblerError,
  kInternalError,
};

inline const char* toString(StatusCode code) {
  switch (code) {
    case StatusCode::kOk:                return "Ok";
    case StatusCode::kInvalidArgument:   return "InvalidArgument";
    case StatusCode::kFileNotFound:      return "FileNotFound";
    case StatusCode::kIoError:           return "IoError";
    case StatusCode::kNotElf:            return "NotElf";
    case StatusCode::kUnsupportedElf:    return "UnsupportedElf";
    case StatusCode::kMalformedElf:      return "MalformedElf";
    case StatusCode::kDisassemblerError: return "DisassemblerError";
    case StatusCode::kInternalError:     return "InternalError";
  }
  return "Unknown";
}

}

#endif
