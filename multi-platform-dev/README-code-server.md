# Browser editor with code-server

The optional `code-server` image target provides a VS Code-based browser editor,
the repository workspace, and a Linux terminal with the existing GCC, GDB,
CMake, Python, SIMDe, and xsimd tools. It uses upstream release packages rather
than cloning or building code-server's sources.

The regular image build continues to produce the command-line environment.
Select `--target code-server` explicitly to build the editor profile.

## Build with Docker

Run from the repository root on Linux x86-64:

```sh
docker build --platform linux/amd64 --target code-server \
  --file multi-platform-dev/Dockerfile \
  --tag moving2arm:code-amd64 multi-platform-dev
```

For AArch64, replace `linux/amd64` and `moving2arm:code-amd64` with
`linux/arm64` and `moving2arm:code-arm64`.

## Build with Apple `container`

Use the prerequisites and service setup in the
[macOS guide](README-container.md), then build the native Arm64 profile:

```sh
container build --arch arm64 --target code-server \
  --file multi-platform-dev/Dockerfile \
  --tag moving2arm:code-arm64 multi-platform-dev
```

Use `--arch amd64` and the `moving2arm:code-amd64` tag for an x86-64 image when
architecture translation is available. This browser workflow uses the runtime's
port publishing; it doesn't require VS Code's Dev Containers extension.

## Prepare persistent editor state

Run on the host, outside the container. These directories hold editor settings,
extensions, caches, and the generated login configuration across container runs:

```sh
CODE_SERVER_STATE_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/moving2arm/code-server"
install -d -m 700 "$CODE_SERVER_STATE_DIR" \
  "$CODE_SERVER_STATE_DIR/config" "$CODE_SERVER_STATE_DIR/data"
```

The image uses `/config/code-server/config.yaml` and `/data/code-server` through
the XDG directory variables. The host-state directory stays outside the Git
checkout. The commands below run with your host UID/GID so workspace changes and
editor state use your account's file ownership.

## Start with Docker

From the repository root, with the state variable above still set:

```sh
docker run --rm -it --name moving2arm-code --platform linux/amd64 \
  --user "$(id -u):$(id -g)" \
  --publish 127.0.0.1:8080:8080 \
  --volume "$PWD:/workspace" \
  --volume "$CODE_SERVER_STATE_DIR/config:/config" \
  --volume "$CODE_SERVER_STATE_DIR/data:/data" \
  moving2arm:code-amd64
```

For Arm64, use `linux/arm64` and `moving2arm:code-arm64`.

## Start with Apple `container`

Use the same host-state directories:

```sh
container run --rm --interactive --tty --name moving2arm-code --arch arm64 \
  --user "$(id -u):$(id -g)" \
  --publish 127.0.0.1:8080:8080 \
  --volume "$PWD:/workspace" \
  --volume "$CODE_SERVER_STATE_DIR/config:/config" \
  --volume "$CODE_SERVER_STATE_DIR/data:/data" \
  moving2arm:code-arm64
```

For the translated x86-64 variant, change the architecture and image tag to
`amd64` and `moving2arm:code-amd64`.

## Open the editor and run the labs

Open [the browser editor](http://127.0.0.1:8080). On first launch, code-server
generates its password in the persistent config file. In a second host terminal,
read your own configuration to obtain the login password:

```sh
CODE_SERVER_STATE_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/moving2arm/code-server"
cat "$CODE_SERVER_STATE_DIR/config/code-server/config.yaml"
```

The `/workspace` checkout opens in the editor. Open **Terminal → New Terminal**
and run:

```sh
cmake -S . -B "build/editor-$(uname -m)" -DCMAKE_BUILD_TYPE=Debug
cmake --build "build/editor-$(uname -m)" --parallel
ctest --test-dir "build/editor-$(uname -m)" --output-on-failure
cmake --build "build/editor-$(uname -m)" --target check_simple_port
```

Save a source change in the editor, rebuild, and inspect compiler diagnostics or
test failures in the terminal. Keep a separate build directory per architecture.
Additional language and debugger extensions can be selected through code-server's
extension interface; extension availability differs from Microsoft's marketplace.
[code-server extension documentation](https://coder.com/docs/code-server/FAQ#why-cant-code-server-use-microsofts-extension-marketplace)

Keep the launch terminal open; use Ctrl+C to stop the server. The source checkout
and the external state directories persist when the container exits. Reuse those
directories for later sessions.

The launch commands publish only on the host's loopback interface and keep
password authentication enabled. For a server on another machine, use SSH
forwarding or an authenticated HTTPS setup following the
[upstream access guide](https://coder.com/docs/code-server/guide). Enable shared
network access only after choosing that access model.

## Version and build validation

The Dockerfile pins code-server `4.143.1` and the SHA-256 digests for both upstream
Debian packages. For an update, change the version and both architecture digests
together, using the assets in the
[upstream release](https://github.com/coder/code-server/releases/tag/v4.143.1).
The development base also pins xsimd 8.1.0 with an archive checksum because
Ubuntu 22.04's package uses an older API. Its version and digest must likewise be
updated together. Other base-image package versions follow Ubuntu's repositories.

Container smoke tests are defined in
[the GUI image workflow](../.github/workflows/containers.yml). They build on
native x86-64 and AArch64 runners, run the porting tests, and check the editor's
login requirement and restart with persistent state. To run the GUI checks
with Apple `container` on a native Arm64 system:

```sh
python3 tests/smoke_code_server.py --runtime container --arch arm64 \
  --image moving2arm:code-arm64 --workspace "$PWD" \
  --state-dir "$PWD/build/code-server-smoke-state-$(date +%s)"
```

With Docker, use `--runtime docker` and the desired architecture/image tag.
Each smoke run creates a fresh private state directory and stops its own test
containers. The driver also accepts `--arch amd64` for the translated variant.
