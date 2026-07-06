# FnFinder

**English** · [中文](README.zh-CN.md)

An AArch64 (ARM64) ELF **function analyser**. Given only a target ELF, it
recovers the functions in it via control-flow analysis and generates, for each
one:

* **name** — the exported/symbol name (C++ names are demangled), or a synthetic
  `sub_<ADDRESS>` when no symbol is available;
* **end address** (exclusive) and **size** in bytes.

## Build

Requires a C++17 compiler and CMake ≥ 3.15.

```sh
cmake -S . -B build
cmake --build build --config Release
```

## Usage

```sh
./build/FnFinder <elf-file> [options]
```

| Option              | Effect                                             |
| ------------------- | -------------------------------------------------- |
| `--format=text\|json` | Output format (default `text`).                  |
| `--no-symbols`      | Ignore function symbols.                            |
| `--no-eh-frame`     | Ignore `.eh_frame` / `PT_GNU_EH_FRAME`.            |
| `--no-entry`        | Ignore the ELF entry point.                         |
| `--no-recursive`    | Disable recursive-descent traversal.                |
| `--no-demangle`     | Keep raw (mangled) C++ names.                       |
| `-v` / `-q`         | Verbose / quiet logging (stderr).                   |

Example:

```text
# FnFinder: 128 function(s), 96 named by symbols

START               END                 LAST                      SIZE  NAME
0x0000000000000640  0x0000000000000684  0x0000000000000680          68  _start
0x0000000000000710  0x00000000000007a0  0x000000000000079c         144  std::vector<int>::push_back(int&&)
0x00000000000007a0  0x00000000000007c4  0x00000000000007c0          36  sub_7A0
```

Columns: **END** is the exclusive end (first byte after the function, and the
start of the next function); **LAST** is the address of the last instruction
(the `ret`/final instruction). For AArch64's fixed 4-byte encoding, normally
`LAST = END − 4`, and always `SIZE = END − START`.

### JSON output

With `--format=json` the tool prints a single JSON object:

```json
{
  "count": 2,
  "functions": [
    {
      "name": "std::vector<int>::push_back(int&&)",
      "start": "0x0000000000000710",
      "end": "0x00000000000007a0",
      "last": "0x000000000000079c",
      "size": 144,
      "has_symbol": true
    },
    {
      "name": "sub_7A0",
      "start": "0x00000000000007a0",
      "end": "0x00000000000007c4",
      "last": "0x00000000000007c0",
      "size": 36,
      "has_symbol": false
    }
  ]
}
```

| Key                    | Type    | Meaning                                                                 |
| ---------------------- | ------- | ----------------------------------------------------------------------- |
| `count`                | number  | Total number of functions in `functions`.                               |
| `functions`            | array   | The recovered functions, sorted by ascending `start`.                   |
| `functions[].name`     | string  | Function name: demangled symbol name, or `sub_<ADDRESS>` if unnamed.    |
| `functions[].start`    | string  | Start address, hex (`0x` + 16 digits).                                   |
| `functions[].end`      | string  | Exclusive end address (first byte after the function), hex.             |
| `functions[].last`     | string  | Address of the last instruction, hex.                                   |
| `functions[].size`     | number  | Size in bytes (`end − start`).                                           |
| `functions[].has_symbol` | boolean | `true` if the name came from a real symbol; `false` for a synthetic `sub_` name. |

Addresses are emitted as `0x`-prefixed, 16-digit hex strings; `size` is a plain
decimal number. Log lines go to stderr, so redirecting stdout yields clean JSON.

## License

Released under the [MIT License](LICENSE). Bundled Capstone
(`external/capstone`) keeps its own BSD-3-Clause license.
