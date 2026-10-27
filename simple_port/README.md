# SIMD porting examples

These examples share the project's CMake build and CTest suite. Run commands
from the repository root:

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native --target simple_port_examples
ctest --test-dir build/native --output-on-failure -L simple_port
./build/native/simple_port/calculation
```

To build and run the suite in one command after configuring:

```sh
cmake --build build/native --target check_simple_port
```

The normal project build and unfiltered CTest run include these targets too.
`simple_port_examples` builds both demos and correctness executables when
`BUILD_TESTING` is enabled. With `-DBUILD_TESTING=OFF`, it builds only the demos;
`check_simple_port` is then unavailable.

| Target | Availability |
| --- | --- |
| `calculation` | Portable scalar reference, always built |
| `calculation_sse` | Compiler target supports x86 SSE |
| `calculation_neon` | Compiler target supports AArch64 NEON |
| `calculation_simde_quick` | SIMDe headers, using native aliases |
| `calculation_simde_better` | SIMDe headers, using explicit SIMDe names |
| `calculation_xsimd` | xsimd 8+ headers; configure `XSIMD_INCLUDE_DIR` if needed |
| `calculation_sve` | Explicit `-DMOVING2ARM_ENABLE_SVE=ON`; requires a suitable execution target |

CMake reports unavailable architecture/dependency targets during configuration.
It doesn't install dependencies. Demos and their corresponding `<target>_test`
executables live under `build/native/simple_port`.

Every available implementation gets two labeled tests:

- `<target>.correctness`: golden values, all lengths 0–257, unaligned inputs,
  tails, output guards, masks, special floats, and empty inputs.
- `<target>.demo`: exact printed comparisons and arithmetic results for the
  shared four-element input.

Both carry the `simple_port` label; use `-L correctness` or `-L demo` to select
either category. The SVE vector-length tests also carry `simple_port` when
enabled on native Linux. Cross builds retain `CMAKE_CROSSCOMPILING_EMULATOR`
for both kinds of test.

For the algorithm, expected results, sanitizer configuration, and SVE setup,
see the [project walkthrough](../README.md#exercise-sequence).
