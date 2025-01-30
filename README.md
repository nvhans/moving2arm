# moving2arm
Basic tools and exercises for moving code to Arm infrastructure

The multi-platform docker image directory includes links to two submodules: AWS Porting Advisor for Graviton, and SIMD Everywhere. Make sure you use `--recurse-submodules` option to get those when you clone this repository.

## Multi-platform Docker image
The "multi-platform-dev" directory contains instructions and a Dockerfile for building a multi-platform image capable of running/emulating both amd64 and aarch64 system architectures. It is not intended for benchmarking or performance profiling and only serves as an entry-point to code porting.

## Simple Port
The "simple_port" directory contains a very simple Intel intrinsics code that can be ported/modified to run on Arm archticture. Examples are provided for SIMD Everywhere and for a direct port to Arm NEON.
