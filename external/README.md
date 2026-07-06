# external/

Third-party dependencies live here. FnFinder implements ELF parsing itself and
only depends on one external library for disassembly.

## Capstone (AArch64 disassembly)

**`external/capstone/` contains a vendored copy of Capstone 5.0.1**, so the
project builds offline with no network access — the top-level `CMakeLists.txt`
builds it as a subproject automatically.

The build resolves Capstone in this order (see the top-level `CMakeLists.txt`):

1. **System install** — configure with `-DFNFINDER_USE_SYSTEM_CAPSTONE=ON` to use
   a Capstone found via `find_package(capstone CONFIG)`.
2. **Vendored copy** *(active — this is what ships here)* — when
   `external/capstone/CMakeLists.txt` exists it is built as a subproject. To
   refresh or re-vendor it:

   ```sh
   rm -rf external/capstone
   git clone --depth 1 --branch 5.0.1 \
       https://github.com/capstone-engine/capstone.git external/capstone
   ```

3. **Automatic fetch** — if `external/capstone/` is removed, CMake downloads
   Capstone 5.0.1 via `FetchContent` (requires network access on first configure).

Both the older Capstone 4.x / 5.0.x `CS_ARCH_ARM64` API (what 5.0.1 ships) and the
newer `CS_ARCH_AARCH64` API are supported through `src/disasm/capstone_compat.h`,
which auto-detects by header presence, so any resolution path above works.
