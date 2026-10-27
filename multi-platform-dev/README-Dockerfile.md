# Linux development with Docker

For macOS with Apple silicon, use the [Apple `container` workflow](README-container.md).
Both workflows build `Dockerfile`, the canonical definition in this directory.

## Requirements

- Linux on x86-64 or AArch64, with Docker installed.
- The repository's SIMDe submodule initialized.
- For the other architecture, an emulator configured for Docker. Follow
  [Docker's multi-platform prerequisites](https://docs.docker.com/build/building/multi-platform/).
  Native builds don't require emulation.

Run all host commands below from the repository root.

## Build a local image for one architecture

Choose the architecture required for an exercise. Build each tag before running
it; a default build alone doesn't create both variants:

```sh
docker build --platform linux/arm64 \
  --file multi-platform-dev/Dockerfile --tag moving2arm:arm64 multi-platform-dev
docker build --platform linux/amd64 \
  --file multi-platform-dev/Dockerfile --tag moving2arm:amd64 multi-platform-dev
```

Each command builds a single-platform image and loads it into the local image
store. The non-native build requires emulation or a builder for that platform.

The image contains Ubuntu 22.04, GCC/G++ 12, CMake/CTest, Ninja, Python, debugging
tools, SIMDe, and xsimd. `CC` and `CXX` select GCC/G++ 12 for CMake by default.
Package patch versions depend on the Ubuntu repositories at build time. Porting
Advisor is a separate setup described in the [project README](../README.md#porting-advisor-exercise).

## Run the exercises

Select the image variant and mount only this checkout:

```sh
docker run --rm -it --platform linux/arm64 \
  --volume "$PWD:/workspace" --workdir /workspace moving2arm:arm64 /bin/bash
```

For x86-64, change both `linux/arm64` and `moving2arm:arm64` to their `amd64`
counterparts. The mount is writable; source changes also affect the host checkout.

Inside the container, give each architecture its own build directory:

```sh
cmake -S . -B "build/$(uname -m)" -DCMAKE_BUILD_TYPE=Release
cmake --build "build/$(uname -m)"
ctest --test-dir "build/$(uname -m)" --output-on-failure
```

Compare SSE output on x86-64 with NEON and SIMDe output on AArch64. Use emulation
for functional checks and native hardware for performance measurements.

## Optional image with both architectures

A builder capable of both platforms can also create a combined image. Create
the named builder once, then use it for subsequent builds:

```sh
docker buildx create --name moving2arm-builder --driver docker-container
docker buildx build --platform linux/amd64,linux/arm64 \
  --builder moving2arm-builder \
  --file multi-platform-dev/Dockerfile --tag moving2arm:dev \
  --output type=oci,dest=/tmp/moving2arm-dev.tar multi-platform-dev
```

This command exports an OCI archive. Local loading of a multi-platform image
requires a compatible image store; see
[Docker's build and image-store guidance](https://docs.docker.com/build/building/multi-platform/).

## Save and load a single-platform image

```sh
docker save --output /tmp/moving2arm-arm64.tar moving2arm:arm64
docker load --input /tmp/moving2arm-arm64.tar
```

The archive contains the built architecture variant; saving it doesn't add
another architecture.
