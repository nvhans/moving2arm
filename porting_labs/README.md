# Portability labs beyond SIMD

Run host commands from the repository root. The default CMake build includes all
C labs; their executables appear under `build/native/labs`. Linux and macOS use
POSIX threads for the concurrency labs. Python packaging and the deliberately
invalid access are separate opt-in builds.

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native
ctest --test-dir build/native --output-on-failure -R '^lab_'
```

Set `-DMOVING2ARM_BUILD_LABS=OFF` to build only the original SIMD exercises.
Use separate build directories for each architecture and compiler configuration.

| Lab | Executables | What to inspect |
| --- | --- | --- |
| [Unaligned parsing](#unaligned-parsing-and-aliasing) | `lab_unaligned` | Typed casts versus object-safe loads and byte order |
| [Character signedness](#character-signedness) | `lab_char_signed`, `lab_char_unsigned` | Target-dependent plain `char` versus byte values |
| [Atomics](#atomics-and-memory-ordering) | `lab_atomics`, `lab_atomic_litmus` | Publication, acknowledgement, and ordering |
| [Runtime dispatch](#runtime-cpu-dispatch) | `lab_runtime_dispatch` | Baseline code, feature detection, and CRC32C |
| [Floating point](#floating-point-reproducibility) | `lab_floating_point` | Fused operations and reduction order |
| [Assembly replacement](#replace-inline-assembly-with-builtins) | `lab_bit_ops` | x86 assembly, portable C, and compiler builtins |
| [Serialization](#binary-serialization) | `lab_serialization` | ABI layout versus a defined wire format |
| [Native dependency](#native-library-and-python-packaging) | `lab_native_dependency`, optional `_moving2arm` | CPU/OS/Python ABI compatibility and wheel contents |

## Unaligned parsing and aliasing

Read [packet.c](unaligned/packet.c), then run:

```sh
./build/native/labs/lab_unaligned
```

The input is an eight-byte little-endian header. The parser reads it at every
byte offset from 0 through 15 and rejects all shorter headers without changing
the output. It copies into an aligned integer with `memcpy` and converts the
specified byte order. It doesn't cast the packet to a native structure.

Contrast that with `*(const uint32_t *)(buffer + offset)`: processor support for
unaligned loads doesn't establish the alignment and object-access rules required
by C. See the [Linux unaligned-access guidance](https://cdn.kernel.org/doc/html/latest/core-api/unaligned-memory-access.html).

To observe the invalid access with UBSan, explicitly build the separate
[unsafe example](unaligned/unsafe_cast.c):

```sh
cmake -S . -B build/unsafe -DCMAKE_BUILD_TYPE=Debug \
  -DMOVING2ARM_BUILD_UNSAFE_EXAMPLES=ON \
  -DCMAKE_C_FLAGS='-fsanitize=undefined -fno-sanitize-recover=undefined'
cmake --build build/unsafe --target lab_unsafe_cast
./build/unsafe/labs/lab_unsafe_cast
```

The final command is expected to fail with a misaligned-load diagnostic. It is
never registered as a normal passing test. Compare optimized assembly for the
safe `memcpy` idiom on both architectures; safe source needn't imply slow code.

## Character signedness

Read [bytes.c](char_signedness/bytes.c). CMake compiles the same source with both
`-fsigned-char` and `-funsigned-char` on GCC/Clang:

```sh
./build/native/labs/lab_char_signed
./build/native/labs/lab_char_unsigned
```

Both programs process every byte from 0 through 255. The legacy sum changes from
`-128` to `32640`; the explicit unsigned-byte sum stays `32640`. Every individual
byte is checked too. Other compilers get a default-signedness executable named
`lab_char_signedness`.

Plain-char defaults depend on the compiler, machine, and OS. Don't infer them
from the CPU name alone. Explicit byte types also matter before table indexing
and calls such as `isalpha`, whose input contract must be respected.
[GCC character documentation](https://gcc.gnu.org/onlinedocs/gcc/Characters-implementation.html)

## Atomics and memory ordering

Read [publication.c](atomics/publication.c). A producer publishes a sequence and
checksum in non-atomic storage. A release store of the ready flag pairs with an
acquire load before the consumer reads the payload. The acknowledgement uses the
same pattern to protect buffer reuse:

```sh
./build/native/labs/lab_atomics
./build/native/labs/lab_atomic_litmus relaxed
./build/native/labs/lab_atomic_litmus acquire-release
```

The first program verifies 20,000 messages. The separate
[litmus](atomics/litmus.c) runs 500 one-shot publications in each mode. All shared
payload accesses in the litmus are atomic, so the relaxed experiment has no C
data race. It counts observations of `flag=1/data=0`; acquire/release forbids that
observation. Relaxed mode permits it but needn't reproduce it in a particular
run. Zero observations, or a passing stress test, isn't a correctness proof.

Compare generated code and explain the happens-before relationships in both
directions before changing memory orders. x86 and Arm can need different
instructions for the same language-level contract.
[LLVM atomic guide](https://www.llvm.org/docs/Atomics.html)

Run ThreadSanitizer separately from address sanitization, using a native compiler
and a target supported by its runtime:

```sh
cmake -S . -B build/tsan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=thread -fno-omit-frame-pointer'
cmake --build build/tsan --target lab_atomics lab_atomic_litmus
ctest --test-dir build/tsan --output-on-failure -R '^lab_atomic'
```

## Runtime CPU dispatch

[dispatch.c](runtime_dispatch/dispatch.c) chooses a portable CRC32C or an
accelerated implementation. Only the accelerated translation unit receives its
extra ISA flags: `-msse4.2` for x86 or `-march=armv8-a+crc` for AArch64.

```sh
./build/native/labs/lab_runtime_dispatch auto
./build/native/labs/lab_runtime_dispatch portable
./build/native/labs/lab_runtime_dispatch hardware
./build/native/labs/lab_runtime_dispatch auto --disable-hardware
```

The last invocation forces the portable fallback even on a capable machine.
Explicit hardware requests return 77 when unavailable, which CTest marks as a
skip. Tests check the known CRC32C of `123456789` (`e3069283`), empty input,
unaligned input, and every length from 0 through 257.

The Arm implementation uses `__crc32cb`, not `__crc32b`: both ports must compute
the same polynomial, initial state, and final XOR. See
[Arm ACLE CRC intrinsics](https://arm-software.github.io/acle/main/acle.html#crc32-intrinsics).
Linux uses [AArch64 HWCAPs](https://www.kernel.org/doc/html/next/arch/arm64/elf_hwcaps.html);
macOS reads the CRC capability using the read-only `sysctlbyname` query shown in
[Apple's CPU-capability tests](https://github.com/apple-oss-distributions/xnu/blob/main/tests/arm_cpu_capabilities.c).
x86 uses compiler CPU-feature detection.

Don't add an optional ISA flag globally when teaching baseline dispatch; it
could introduce unsupported instructions before the feature check runs.

## Floating-point reproducibility

[dot.c](floating_point/dot.c) demonstrates two independent effects:

- A rounded multiply followed by addition produces zero for the fixture;
  explicit `fmaf` produces `-0x1p-46` because it rounds only once.
- Summing `[1e8, 1, -1e8, 3]` sequentially produces `3`; two accumulators produce
  `4`, matching this fixture's higher-precision reference.

```sh
./build/native/labs/lab_floating_point
./build/native/labs/lab_floating_point --benchmark
```

The optional benchmark prints CPU milliseconds and absolute numerical error for
sequential, fused, and two-accumulator dot products. Inputs vary during timing
to prevent hoisting an invariant result. These are small illustrative timings;
compare performance on native target hardware, not under architecture translation.

The plain `a*b+c` expression uses volatile inputs to prevent constant folding.
Compare `-ffp-contract=off` and `-ffp-contract=fast` builds, then inspect assembly;
the actual contraction depends on compiler settings and target support.
[GCC floating-point behavior](https://gcc.gnu.org/onlinedocs/gcc/Floating-point-implementation.html)
Don't use `-ffast-math` for these checks: the fixtures intentionally preserve
specified evaluation order and special-value semantics elsewhere in the suite.

## Replace inline assembly with builtins

Read [trailing_zeros.c](bit_ops/trailing_zeros.c):

```sh
./build/native/labs/lab_bit_ops
```

Compare the guarded x86 `bsfq` implementation with a portable loop and
`__builtin_ctzll`. The API defines zero as 64; the builtin needs an explicit guard
before it is called with zero. The test covers zero, all ones, each single-bit
value and complement, and 10,000 deterministic patterns.

Compile with `-O2 -S` on both targets. Inspect how the compiler implements the
builtin on each architecture. The porting action replaces assembly with a
defined operation rather than maintaining another handwritten assembly version.

## Binary serialization

[record.c](serialization/record.c) prints `sizeof`, alignment, and member offsets,
then validates an explicit 11-byte big-endian format:

```sh
./build/native/labs/lab_serialization
```

The golden bytes are:

```text
01 01 02 03 04 a0 b0 ff ff fb 1e
```

They represent version 1, ID `0x01020304`, flags `0xa0b0`, and temperature `-1250`.
The signed field uses a specified two's-complement wire encoding with defined
conversions. Tests check signed boundaries, truncation, buffer capacity, unknown
versions, and extra bytes.

Contrast this with `fwrite(&value, sizeof(value), 1, stream)`, which exposes native
layout and byte order. Layout may be identical on your x86-64 and AArch64 hosts;
that coincidence doesn't define a persistent format. Distinguish CPU changes
from OS ABI changes, and don't assume moving to Arm implies changing endianness.

## Native library and Python packaging

The default build produces a shared FNV-1a library and a C client:

```sh
./build/native/labs/lab_native_dependency
```

The optional CPython extension calls that shared library. Use Python 3.9+ and its
development headers. The development image includes the headers; CMake reports
missing prerequisites rather than downloading them. Use a native, unsanitized
build directory matching the running interpreter:

```sh
cmake -S . -B build/python -DCMAKE_BUILD_TYPE=Release \
  -DMOVING2ARM_BUILD_PYTHON=ON
cmake --build build/python --target native_wheel lab_native_dependency
ctest --test-dir build/python --output-on-failure -R 'native_dependency'
```

Artifacts appear under `build/python/native_dependency` and the wheel under
`build/python/wheelhouse`. The tests compare Python and C checksums, inspect CPU
headers, verify wheel RECORD hashes, and import the unpacked wheel in an isolated
child interpreter. They also verify failure when the dependent library is absent.
Test stages stay in the build directory for inspection. No package installation
is needed to run these checks.

[package_wheel.py](native_dependency/package_wheel.py) deliberately exposes the
[wheel format](https://packaging.python.org/en/latest/specifications/binary-distribution-format/)
using the standard library. It packages existing CMake artifacts rather than
replacing the compiler build. The extension's ABI suffix must match the Python
interpreter; both binaries must match the native CPU. Linux wheels use local
`linux_x86_64` or `linux_aarch64` tags, not a claim of manylinux compatibility.
macOS wheels conservatively target the build host's OS major version. The bundled
library is found relative to the extension through `$ORIGIN` or `@loader_path`.

Inspect a shared library and then ask the platform loader to load it:

```sh
python3 porting_labs/native_dependency/inspect_binary.py \
  build/python/native_dependency/libmoving2arm_native.dylib --load
```

On Linux substitute `libmoving2arm_native.so`. For a real wrong-architecture
reproduction on an Apple silicon Mac, build the x86-64 C library in a separate
directory, then load it from native Arm64 Python:

```sh
cmake -S . -B build/foreign-x86 -DCMAKE_OSX_ARCHITECTURES=x86_64
cmake --build build/foreign-x86 --target moving2arm_native
python3 porting_labs/native_dependency/inspect_binary.py \
  build/foreign-x86/native_dependency/libmoving2arm_native.dylib --load
```

The last command is expected to fail with both an architecture mismatch and the
loader's rejection. On Linux, build on the other architecture or use the cross
compiler configuration from CI, then inspect/load that foreign `.so` from native
Python. Rebuild the library and extension together for each target. Matching CPU
architecture alone doesn't guarantee OS, libc, or Python ABI compatibility.

For production distribution, use standard Python build tooling and the platform's
wheel-repair and compatibility checks. This local lab focuses on the artifacts
and contracts a port needs to preserve.
