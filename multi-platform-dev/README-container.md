# macOS development with Apple `container`

Use Apple's [`container`](https://github.com/apple/container) CLI to build and run
the Linux porting environment on a Mac with Apple silicon. Docker Desktop isn't
required. The commands below reuse `Dockerfile`; a separate image definition
isn't needed for macOS.

## Requirements

- A Mac with Apple silicon and macOS 26 or later, as specified in Apple's
  [requirements](https://github.com/apple/container#requirements).
- Apple `container` installed using the project's
  [installation instructions](https://github.com/apple/container#install).
- Rosetta available for the optional `amd64` build and run. Follow Apple's
  [multiplatform guide](https://github.com/apple/container/blob/main/docs/multiplatform-images.md)
  for architecture translation requirements.
- This repository's SIMDe submodule initialized. If it wasn't included when
  cloning, run `git submodule update --init --recursive` from the repository root.

Start the service before building:

```sh
container --version
container system start
container system status
```

On first use, follow any kernel setup prompts from `container`.

## Build for Arm64

Run from the repository root. The final argument sets the build context so the
Dockerfile can copy the SIMDe headers from the submodule:

```sh
container build --arch arm64 \
  --file multi-platform-dev/Dockerfile \
  --tag moving2arm:arm64 multi-platform-dev
```

The image includes Ubuntu 22.04, GCC/G++ 12, CMake/CTest, Ninja, development
tools, SIMDe, and xsimd. `CC` and `CXX` select GCC/G++ 12 for CMake. Porting Advisor
is a separate setup described in the [project README](../README.md#porting-advisor-exercise).

## Run the Arm64 environment

Run from the repository root to mount the checkout at `/workspace`:

```sh
container run --rm --interactive --tty --arch arm64 \
  --volume "$PWD:/workspace" --workdir /workspace \
  moving2arm:arm64 /bin/bash
```

The checkout is writable inside the container. Changes under `/workspace` also
change the host checkout. The container is removed when the shell exits.

Inside the container, build and test the available examples. Keep a separate
build directory for each architecture:

```sh
cmake -S . -B "build/$(uname -m)" -DCMAKE_BUILD_TYPE=Release
cmake --build "build/$(uname -m)"
ctest --test-dir "build/$(uname -m)" --output-on-failure
```

For a direct compilation smoke test, run the NEON and SIMDe examples:

```sh
uname -m
gcc-12 --version
gcc-12 -std=c11 -O2 -Wall -Wextra \
  simple_port/calculation_neon.c -o /tmp/calculation-neon
/tmp/calculation-neon
gcc-12 -std=c11 -O2 -Wall -Wextra \
  simple_port/calculation_simde_quick.c -lm -o /tmp/calculation-simde
/tmp/calculation-simde
```

`uname -m` should report `aarch64`. Both examples should report that element 0 is
not larger than its counterpart, followed by these arithmetic results:

```text
Addition Result: 2.000000 6.000000 12.000000 20.000000
Multiplication Result: 2.000000 12.000000 36.000000 80.000000
Square Root Result: 1.414214 3.464102 6.000000 8.944272
```

## Optional x86-64 environment

Exit the Arm64 shell, then build and run the x86-64 image from the repository root:

```sh
container build --arch amd64 \
  --file multi-platform-dev/Dockerfile \
  --tag moving2arm:amd64 multi-platform-dev
container run --rm --interactive --tty --arch amd64 \
  --volume "$PWD:/workspace" --workdir /workspace \
  moving2arm:amd64 /bin/bash
```

Inside this container, compile and run the original SSE example:

```sh
uname -m
gcc-12 -std=c11 -O2 -Wall -Wextra \
  simple_port/calculation_sse.c -o /tmp/calculation-sse
/tmp/calculation-sse
```

`uname -m` should report `x86_64`. Compare the output with the Arm64 examples.
Apple `container` uses Rosetta for this architecture translation. Use it to
check functionality; measure performance on native x86-64 and AArch64 hardware.

## Optional image containing both architectures

Instead of separate tags, build both variants under one local tag:

```sh
container build --arch arm64 --arch amd64 \
  --file multi-platform-dev/Dockerfile \
  --tag moving2arm:dev multi-platform-dev
```

Use `moving2arm:dev` in either run command above and keep `--arch` to select the
variant. Apple documents this workflow in its
[multiplatform guide](https://github.com/apple/container/blob/main/docs/multiplatform-images.md).

## Scope

These are Linux builds running on macOS, not native macOS binaries. Selecting
`arm64` doesn't establish SVE or SVE2 availability; those exercises need a target
that supports the required extensions. See the
[Docker workflow](README-Dockerfile.md) for Linux hosts.

Command options were checked against Apple `container` 1.0.0. The image build and
container smoke tests haven't yet been verified with this workflow.
