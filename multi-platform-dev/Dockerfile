# syntax=docker/dockerfile:1
# Shared image definition for Docker and Apple container, on either CPU target.
FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates vim wget sudo git tar build-essential cmake ninja-build curl \
    python3 python3-dev python3-pip python3-venv gdb valgrind htop strace gcc-12 g++-12 \
    libxsimd-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

ENV CC=gcc-12 CXX=g++-12

# Setup container user
ENV USER=ubuntu
RUN useradd --create-home -s /bin/bash -m $USER && echo "$USER:ubuntu" | chpasswd && adduser $USER sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# SIMD Everywhere
COPY simde/simde /usr/include/simde
COPY simde/README.md /home/ubuntu/README-simde.md

# Porting Advisor is available as a repository submodule, not installed here.
# See the project README for its separate setup and scanning exercise.

WORKDIR /home/ubuntu
USER ubuntu
