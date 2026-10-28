# syntax=docker/dockerfile:1
# Shared image definition for Docker and Apple container, on either CPU target.
FROM ubuntu:22.04 AS development
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates vim wget sudo git tar build-essential cmake ninja-build curl \
    python3 python3-dev python3-pip python3-venv gdb valgrind htop strace gcc-12 g++-12 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Ubuntu 22.04's packaged xsimd predates the batch API used by these exercises.
ARG XSIMD_VERSION=8.1.0
ARG XSIMD_SHA256=d52551360d37709675237d2a0418e28f70995b5b7cdad7c674626bcfbbf48328
RUN curl --fail --location --retry 3 \
      "https://github.com/xtensor-stack/xsimd/archive/refs/tags/${XSIMD_VERSION}.tar.gz" \
      --output /tmp/xsimd.tar.gz \
    && printf '%s  /tmp/xsimd.tar.gz\n' "$XSIMD_SHA256" | sha256sum --check - \
    && install -d /tmp/xsimd /usr/local/include \
    && tar --extract --gzip --file /tmp/xsimd.tar.gz --strip-components=1 --directory /tmp/xsimd \
    && cp -R /tmp/xsimd/include/xsimd /usr/local/include/ \
    && rm -rf /tmp/xsimd /tmp/xsimd.tar.gz

ENV CC=gcc-12 CXX=g++-12

# Setup container user
ENV USER=ubuntu
RUN useradd --create-home -s /bin/bash -m $USER && echo "$USER:ubuntu" | chpasswd && adduser $USER sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Match the repository's SIMDe gitlink without requiring initialized submodules
# in the Docker context (VS Code can clone into a container volume first).
ARG SIMDE_REVISION=493614901bcecea23554b52a3a55fc0772c2de8b
ARG SIMDE_SHA256=d814dac1f4eefdbe21c61e066c592a93814486d3d6c5cc55c44edf446971b69d
RUN curl --fail --location --retry 3 \
      "https://github.com/simd-everywhere/simde/archive/${SIMDE_REVISION}.tar.gz" \
      --output /tmp/simde.tar.gz \
    && printf '%s  /tmp/simde.tar.gz\n' "$SIMDE_SHA256" | sha256sum --check - \
    && install -d /tmp/simde /usr/include \
    && tar --extract --gzip --file /tmp/simde.tar.gz --strip-components=1 --directory /tmp/simde \
    && cp -R /tmp/simde/simde /usr/include/ \
    && cp /tmp/simde/README.md /home/ubuntu/README-simde.md \
    && rm -rf /tmp/simde /tmp/simde.tar.gz

# Porting Advisor is available as a repository submodule, not installed here.
# See the project README for its separate setup and scanning exercise.

WORKDIR /home/ubuntu
USER ubuntu

# Desktop VS Code / Dev Containers: no browser server is needed in this profile.
FROM development AS devcontainer
ARG TARGETARCH
RUN test "$TARGETARCH" = amd64 || test "$TARGETARCH" = arm64
ENV MOVING2ARM_DEV_ARCH=$TARGETARCH

# Optional browser editor. Build with --target code-server on either architecture.
FROM development AS code-server
USER root
ARG CODE_SERVER_VERSION=4.143.1
ARG CODE_SERVER_AMD64_SHA256=e4b771c98fafbe2af3990ecd71d93ba22515588db550007076768a014ae34c02
ARG CODE_SERVER_ARM64_SHA256=79ca8df2db219b24de75e2a830c5302808124d8c2bf132f4bc6105cea834bb1c
RUN set -eu; \
    code_server_arch="$(dpkg --print-architecture)"; \
    case "$code_server_arch" in \
      amd64) code_server_sha="$CODE_SERVER_AMD64_SHA256" ;; \
      arm64) code_server_sha="$CODE_SERVER_ARM64_SHA256" ;; \
      *) echo "Unsupported code-server architecture: $code_server_arch" >&2; exit 1 ;; \
    esac; \
    curl --fail --location --retry 3 \
      "https://github.com/coder/code-server/releases/download/v${CODE_SERVER_VERSION}/code-server_${CODE_SERVER_VERSION}_${code_server_arch}.deb" \
      --output /tmp/code-server.deb; \
    printf '%s  /tmp/code-server.deb\n' "$code_server_sha" | sha256sum --check -; \
    apt-get update; \
    apt-get install -y --no-install-recommends dumb-init /tmp/code-server.deb; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/* /tmp/code-server.deb; \
    install -d -o ubuntu -g ubuntu /workspace /config /data; \
    dpkg-query --show code-server

# Writable bind mounts let learners run with their host UID/GID. Keep editor
# state, extensions, and the generated login config outside the source checkout.
ENV XDG_CONFIG_HOME=/config XDG_DATA_HOME=/data XDG_CACHE_HOME=/data/cache SHELL=/bin/bash
USER ubuntu
WORKDIR /workspace
EXPOSE 8080
ENTRYPOINT ["/usr/bin/dumb-init", "--", "/usr/bin/code-server"]
CMD ["--bind-addr", "0.0.0.0:8080", "--auth", "password", "/workspace"]

# Preserve the command-line image as the default when --target is omitted.
FROM development AS cli
