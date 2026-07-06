# FnFinder

[English](README.md) · **中文**

一个针对 AArch64（ARM64）ELF 的**函数分析器**。只需给它一个目标 ELF，它就能通过
控制流分析还原其中的函数——为每个函数重建控制流图，并为每个函数生成分析信息：

* **函数名** —— 导出符号名（C++ 符号会被 demangle 还原），无符号时用合成名
  `sub_<地址>`；
* **结束地址**（开区间）、**最后一条指令地址**、以及以字节为单位的**长度**。

## 构建

需要 C++17 编译器和 CMake ≥ 3.15。

```sh
cmake -S . -B build
cmake --build build --config Release
```

## 用法

```sh
./build/FnFinder <elf文件> [选项]
```

| 选项                  | 作用                                        |
| --------------------- | ------------------------------------------- |
| `--format=text\|json` | 输出格式（默认 `text`）。                   |
| `--no-symbols`        | 忽略函数符号。                              |
| `--no-eh-frame`       | 忽略 `.eh_frame` / `PT_GNU_EH_FRAME`。      |
| `--no-entry`          | 忽略 ELF 入口点。                           |
| `--no-recursive`      | 关闭递归下降遍历。                          |
| `--no-demangle`       | 保留 C++ 原始（mangled）符号名。            |
| `-v` / `-q`           | 详细 / 安静日志（输出到 stderr）。          |

示例：

```text
# FnFinder: 128 function(s), 96 named by symbols

START               END                 LAST                      SIZE  NAME
0x0000000000000640  0x0000000000000684  0x0000000000000680          68  _start
0x0000000000000710  0x00000000000007a0  0x000000000000079c         144  std::vector<int>::push_back(int&&)
0x00000000000007a0  0x00000000000007c4  0x00000000000007c0          36  sub_7A0
```

列含义：**END** 是开区间结束地址（函数之后的第一个字节，也是下一个函数的起点）；
**LAST** 是最后一条指令的地址（`ret` / 收尾指令）。在 AArch64 定长 4 字节编码下通常
`LAST = END − 4`，且恒有 `SIZE = END − START`。

### JSON 输出

加上 `--format=json` 时，工具输出一个 JSON 对象：

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

| 键                       | 类型     | 含义                                                             |
| ------------------------ | -------- | ---------------------------------------------------------------- |
| `count`                  | number   | `functions` 中的函数总数。                                       |
| `functions`              | array    | 还原出的函数列表，按 `start` 升序排列。                          |
| `functions[].name`       | string   | 函数名：demangle 后的符号名，或无符号时的 `sub_<地址>`。         |
| `functions[].start`      | string   | 起始地址，十六进制（`0x` + 16 位）。                             |
| `functions[].end`        | string   | 开区间结束地址（函数之后的第一个字节），十六进制。              |
| `functions[].last`       | string   | 最后一条指令的地址，十六进制。                                   |
| `functions[].size`       | number   | 字节长度（`end − start`）。                                      |
| `functions[].has_symbol` | boolean  | `true` 表示名字来自真实符号；`false` 表示是合成的 `sub_` 名。    |

地址一律输出为带 `0x` 前缀的 16 位十六进制字符串；`size` 是十进制数字。日志写到
stderr，因此把 stdout 重定向即可得到干净的 JSON。

## 许可证

以 [MIT 许可证](LICENSE) 发布。内置的 Capstone（`external/capstone`）保留其自身的
BSD-3-Clause 许可证。
