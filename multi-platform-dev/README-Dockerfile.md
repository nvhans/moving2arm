# How To build this multi-platform development image
## Build System Requirements
- Linux (aarch64 or x86-64)
- Docker
- QEMU & binfmt-support `docker run --privileged --rm tenistiigi/binfmt --install all`
- containerd image store (https://docs.docker.com/engine/storage/containerd/)
- Docker multi-platform build directions: (https://docs.docker.com/build/building/multi-platform/#qemu)

## Image contents
- Ubuntu 22.04 base
- GCC 11.4
- CMake*
- Python 3.10.12
- Valgrind 3.18.1
- GDB*
- Strace
- Git*
- Curl*
- Wget*
- tar
- vim
- HTop
- SIMD Everywhere

## Docker Build
```
$ cd <location of multi-platform-dev Dockerfile>
$ docker build -t <image-tag>:<image-version> .
```

# How to run this multi-platform development image
## Host System Requirements
- Linux (aarch64 or x86-64)
- Docker

## Docker Run Commands
### Run Docker image on native platform
```
$ docker run --rm -it -v $HOME:$HOME/host <image-tag>:<image-version>
```
### Run Docker image as alternate platform
This uses QEMU to emulate the alternate platform, i.e. x86-64 on Arm64 or Arm64 on x86-64. Although this is not high-performance, it does allow for porting of software code using a single platform architecture.

### On aarch64 platform as x86-64 image
```
$ docker run --rm -it -v $HOME:$HOME/host --platform linux/amd64 <image-tag>:<image-version> 
```
### On x86-64 platform as aarch64 image
```
$ docker run --rm -it -v $HOME:$HOME/host --platform linux/arm64 <image-tag>:<image-version> 
```

# How to save this multi-platform development image and re-load onto new system
## Save image to tarball
```
$ docker save -o <tarball-name> <image-tag>:<image-version>
```
## Load image from tarball
```
$ docker load -i <tarball-name>
```
