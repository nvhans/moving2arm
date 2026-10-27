# moving2arm

Hands-on exercises for porting C and C++ from x86-64 to AArch64. Establish correct
behavior first, identify architecture dependencies, then compare portability
layers, direct intrinsics, and compiler vectorization.
The [additional portability labs](porting_labs/README.md) cover memory access,
signedness, concurrency, runtime CPU dispatch, floating point, inline assembly,
serialization, and native Python dependencies.

Source repository: [nvhans/moving2arm](https://github.com/nvhans/moving2arm).

## Get started

Use Git, a C11/C++14 compiler, and CMake 3.20 or later. The additional labs target
Linux and macOS. For a consistent Linux toolchain, use one of the container
workflows below. CMake discovers installed dependencies without downloading them.

Clone the project and its pinned submodules on the system where you'll learn:

```sh
git clone --recurse-submodules https://github.com/nvhans/moving2arm.git
cd moving2arm
```

For an existing clone that lacks the submodules:

```sh
git submodule update --init --recursive
```

Build and test the available examples:

```sh
cmake -S . -B build/native -DCMAKE_BUILD_TYPE=Release
cmake --build build/native
ctest --test-dir build/native --output-on-failure
./build/native/simple_port/calculation
```

The default build includes the scalar reference, SSE on x86, NEON on AArch64, and
both SIMDe examples when their headers are present. CMake reports skipped targets.
SVE and SVE2 are opt-in because compiler support doesn't establish CPU support.

The [simple_port build and test guide](simple_port/README.md) lists each SIMD
target and its prerequisites. Build just these examples with
`cmake --build build/native --target simple_port_examples`, or build and test
them with `cmake --build build/native --target check_simple_port`.
Their executables are under `build/native/simple_port`; select their tests with
`ctest --test-dir build/native --output-on-failure -L simple_port`.

SIMDe is provided by `multi-platform-dev/simde`. xsimd is optional: provide
[xsimd 8+ headers](https://github.com/xtensor-stack/xsimd), then configure with
`-DXSIMD_INCLUDE_DIR=/path/containing/xsimd`. Both libraries are header-only for
these exercises. The development image includes both.

For an isolated Linux environment:

- [Apple `container` on macOS with Apple silicon](multi-platform-dev/README-container.md)
- [Docker on Linux](multi-platform-dev/README-Dockerfile.md)

Both workflows use `multi-platform-dev/Dockerfile`. The older `.linux` and
`.macos` files remain as legacy copies; use the canonical file for new builds.
Builds and runs under architecture translation check functionality. Benchmark on
native target hardware.

## Choose your learning system

| System | Native SIMD example | Linux development environment |
| --- | --- | --- |
| Linux x86-64 | SSE | Native compiler or the `linux/amd64` Docker image |
| Linux AArch64 | NEON | Native compiler or the `linux/arm64` Docker image |
| macOS with Apple silicon | NEON | Apple `container`; see the macOS workflow |
| macOS with Intel | SSE | Use the native compiler for the macOS exercises, or an available Linux container runtime |

Use your own systems for these workflows. A native x86-64 system provides the source
architecture baseline; a native AArch64 system provides the porting target.
Translation is useful for functional checks when you have only one architecture.

### Native Linux build

After cloning on either Linux architecture, check your toolchain:

```sh
uname -sm
cmake --version
cc --version
c++ --version
```

The architecture should be `x86_64` or `aarch64`. Once the compiler and CMake
requirements above are met, build in a directory dedicated to this system:

```sh
cmake -S . -B build/linux-native -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux-native --parallel
ctest --test-dir build/linux-native --output-on-failure
cmake --build build/linux-native --target check_simple_port
```

On x86-64, run `build/linux-native/simple_port/calculation_sse`; on AArch64, run
`build/linux-native/simple_port/calculation_neon`. Both also provide the scalar
and SIMDe versions when their dependencies are present. Check the configure
output for optional xsimd and architecture-specific targets.

Use separate build directories when changing operating system, architecture,
compiler, or sanitizer settings. On a second system, clone the same repository
and rebuild there. Compare `git rev-parse HEAD` on both systems to confirm the
same source revision before comparing results. Build products aren't transferred
through Git.

### Docker build and test on Linux

Confirm Docker is installed and its daemon is accessible:

```sh
docker version
docker info
```

From the repository root on Linux x86-64:

```sh
docker build --platform linux/amd64 \
  --file multi-platform-dev/Dockerfile --tag moving2arm:amd64 multi-platform-dev
docker run --rm -it --platform linux/amd64 \
  --volume "$PWD:/workspace" --workdir /workspace moving2arm:amd64 /bin/bash
```

On Linux AArch64, replace `linux/amd64` with `linux/arm64` and
`moving2arm:amd64` with `moving2arm:arm64` in both commands. The final build
argument is the context containing the SIMDe submodule.

Inside the container, build and run the full suite:

```sh
cmake -S . -B build/container-native -DCMAKE_BUILD_TYPE=Release
cmake --build build/container-native --parallel
ctest --test-dir build/container-native --output-on-failure
```

The checkout is mounted read/write, so edits and build output remain on the host.
For macOS with Apple silicon, follow the
[Apple `container` instructions](multi-platform-dev/README-container.md) instead.
See the [Docker guide](multi-platform-dev/README-Dockerfile.md) for multi-platform
images and image export. Container image builds still need validation on each
target architecture.

## Exercise sequence

| Step | Source | Action and tradeoff |
| --- | --- | --- |
| 1 | [Scalar](simple_port/calculation.c) | Establish a portable reference before changing architecture-specific code. |
| 2 | [SSE](simple_port/calculation_sse.c) | Identify x86 headers, vector types, and operations. These examples require SSE, not SSE4.2. |
| 3 | [SIMDe aliases](simple_port/calculation_simde_quick.c) | Keep x86 intrinsic names while replacing the include. This minimizes source changes. |
| 4 | [Explicit SIMDe](simple_port/calculation_simde_better.c) | Use explicit portable names to avoid introducing native aliases. Compare with step 3. |
| 5 | [NEON](simple_port/calculation_neon.c) | Translate operations directly. This gives control over AArch64 code at the cost of another implementation. |
| 6 | [xsimd](simple_port/calculation_xsimd.cpp) | Express the algorithm in C++; allow the compiler target to select batch width. |
| 7 | [Scalar loop](simple_port/calculation.h) | Inspect auto-vectorization before deciding whether explicit intrinsics are needed. |
| 8 | [SVE](simple_port/calculation_sve.c) | Replace fixed-width loops and scalar tails with scalable vectors and predicates. |
| 9 | [SVE2](arm_sve2/arm_sve2.c) | Observe predicated signed saturating addition at overflow boundaries, a form requiring SVE2. |

For steps 1–6, run the available demos and the correctness suite. All implement:

```text
greater[i] = a[i] > b[i]      (numeric 0 or 1)
add[i]     = a[i] + b[i]
mul[i]     = add[i] * b[i]
root[i]    = sqrtf(mul[i])
```

The shared API in `simple_port/calculation.h` accepts any length and ordinary
unaligned arrays. Inputs and output arrays must not overlap. Zero-length calls
must not access any buffers. `calculation_demo.h` supplies identical demo inputs
and output formatting; each implementation remains directly compilable:

```sh
cc -std=c11 -O2 -Wall -Wextra simple_port/calculation.c -lm -o /tmp/calculation
/tmp/calculation
cc -std=c11 -O2 -Wall -Wextra -I multi-platform-dev/simde \
  simple_port/calculation_simde_quick.c -lm -o /tmp/calculation-simde
/tmp/calculation-simde
```

Expected comparisons are `0, 1, 1, 1`; expected arithmetic is:

```text
Addition Result: 2.000000 6.000000 12.000000 20.000000
Multiplication Result: 2.000000 12.000000 36.000000 80.000000
Square Root Result: 1.414214 3.464102 6.000000 8.944272
```

SSE `_mm_set_ps(high, ..., low)` argument order is a constructor convention.
Array loads preserve array order on both architectures. Comparison results are
masks, not ordinary floats; the examples explicitly convert them to numeric 0/1.

## Inspect auto-vectorization

For Clang, generate assembly and request loop-vectorizer diagnostics:

```sh
clang -std=c11 -O3 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize \
  -S simple_port/calculation.c -o build/calculation.s
```

For GCC, substitute `-fopt-info-vec-optimized -fopt-info-vec-missed` for the two
`-Rpass` options. Inspect alias checks and the `sqrtf` call. As an experiment,
compare a build with `-fno-math-errno`; that changes the math-errno contract.
Don't enable `-ffast-math` for the correctness suite: it assumes NaN, infinity,
and signed-zero behavior that those flags can change.

## SVE and SVE2

Use an AArch64 Linux target with the required CPU extensions, or an explicitly
configured emulator. See the [Arm ACLE specification](https://arm-software.github.io/acle/main/acle.html)
for scalable vector types and predicates. SVE float vectors use `svfloat32_t`;
`svfloat32x4_t` is a tuple of four scalable vectors, not four float lanes.

```sh
cmake -S . -B build/sve -DCMAKE_BUILD_TYPE=Release \
  -DMOVING2ARM_ENABLE_SVE=ON -DMOVING2ARM_ENABLE_SVE2=ON
cmake --build build/sve
ctest --test-dir build/sve --output-on-failure
```

Enable only the extensions your execution target supports. Compiler flags are
configurable through `MOVING2ARM_SVE_FLAGS` and `MOVING2ARM_SVE2_FLAGS`.
An Arm64 container on Apple silicon doesn't imply SVE/SVE2 support.

On native Linux, CTest also requests 128-, 256-, 512-, and 2048-bit SVE vector
lengths. The launcher selects an inherited length before executing the test;
unsupported lengths are reported as skipped. SVE2 supplies the predicated
saturating-add form used here; unpredicated saturating addition is already in
base SVE. The SVE2 example checks every
length from 0 through 131, including positive and negative saturation.

For cross builds, set an AArch64 compiler and
[`CMAKE_CROSSCOMPILING_EMULATOR`](https://cmake.org/cmake/help/latest/variable/CMAKE_CROSSCOMPILING_EMULATOR.html).
The [CI workflow](.github/workflows/ci.yml) shows a complete QEMU configuration
and tests 128-, 512-, and 2048-bit vectors.
It explicitly selects QEMU's default vector length and checks the running length
through `MOVING2ARM_EXPECT_SVE_BITS`, so a maximum-length limit can't silently
leave a test running at the emulator's default. See
[QEMU's vector-length settings](https://www.qemu.org/docs/master/system/arm/cpu-features.html#sve-user-mode-default-vector-length-property).

## Validation

Each calculation implementation gets the same golden-value test plus every
length from 0 through 257. Tests cover equality and both comparison directions,
unaligned buffers, vector tails, output guards, null empty inputs, NaNs,
infinities, signed zeros, and negative square-root inputs. Floating-point checks
allow a small relative/absolute tolerance while preserving special-value behavior.
Separate demo tests compare the complete printed output with the expected
comparisons and arithmetic results. Both test kinds carry the `simple_port` label.

To detect out-of-bounds accesses and undefined behavior with GCC or Clang:

```sh
cmake -S . -B build/sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build build/sanitize
ctest --test-dir build/sanitize --output-on-failure
```

CI runs native Linux x86-64 and AArch64 tests with sanitizers, checks xsimd under
AVX2 where available, and runs the SVE/SVE2 tests under QEMU. These checks don't
measure performance. The tiny demo inputs and printing are unsuitable benchmarks.

## Porting Advisor exercise

The image doesn't install Porting Advisor. To set it up separately, follow its
[submodule instructions](multi-platform-dev/porting-advisor-for-graviton/README.md#as-a-python-script)
using a Python virtual environment and its declared requirements. Scan only
`simple_port`, so bundled dependency sources don't obscure the exercise:

```sh
python3 multi-platform-dev/porting-advisor-for-graviton/src/porting-advisor.py simple_port
```

Compare findings for the SSE source with the SIMDe and NEON ports. A clean scan
isn't proof of correctness; retain compile checks and differential tests.

## Additional portability labs

The [lab walkthrough](porting_labs/README.md) adds eight runnable examples beyond
SIMD. Their C targets build and test by default; executables are under
`build/native/labs`. It includes explicit signed/unsigned-char builds, a race-free
atomic litmus, CRC32C runtime dispatch, numerical-error measurements, stable wire
bytes, and an optional native CPython wheel.

```sh
ctest --test-dir build/native --output-on-failure -R '^lab_'
```

The deliberately invalid unaligned load and Python packaging are opt-in, with
separate commands in the walkthrough. Set `-DMOVING2ARM_BUILD_LABS=OFF` for a
SIMD-only build. The native CI jobs run the C labs with sanitizers and test the
Python wheel in a separate unsanitized build.

## GUI learning environments (planned)

The following profiles are options for extending the shared development image;
their launch configuration and learning materials are future work.

| Option | Learning workflow | Additional project work |
| --- | --- | --- |
| VS Code with Dev Containers | Edit C/C++, build with CMake, inspect diagnostics, and debug inside the development container | Add a `.devcontainer` configuration and select C/C++ and CMake tooling |
| VS Code Remote - SSH with Dev Containers | Use a desktop editor with source and container execution on a learner's own Linux host | Document the remote folder and container setup |
| JupyterLab | Follow guided notebooks, run compiler/test commands in terminals or Python cells, and plot numerical results | Add an optional Python environment, a JupyterLab launch profile, and introductory notebooks |

VS Code supports using Remote - SSH and Dev Containers together; the source
checkout and bind mounts then belong to the remote host.
[VS Code remote-container workflow](https://code.visualstudio.com/remote/advancedcontainers/develop-remote-host)
The proposed VS Code profiles target Docker; Apple `container` compatibility
needs a separate check.
JupyterLab supplies the browser workspace; the C/C++ examples can still be built
through the existing CMake commands.
[JupyterLab documentation](https://jupyterlab.readthedocs.io/en/stable/getting_started/starting.html)

Before adding either profile, validate the current command-line image build and
full test suite on `linux/amd64` and `linux/arm64`, including the optional Python
extension and wheel tests. Keep GUI dependencies opt-in and preserve the existing
command-line workflow. Review workspace UID/GID mapping and the image's current
fixed-password/passwordless-sudo setup as part of the runtime design.

For a remote JupyterLab profile, plan for SSH-forwarded access and retain server
authentication. Jupyter Server enables token authentication by default.
[Jupyter Server authentication](https://jupyter-server.readthedocs.io/en/latest/operators/security.html)
