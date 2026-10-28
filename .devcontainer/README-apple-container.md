# VS Code attached to Apple `container`

Apple silicon Macs can use VS Code's experimental **Attach to Running Apple
Container** support with the same development image and exercises. This route
builds and starts the container explicitly, then attaches the desktop editor.
It doesn't execute `devcontainer.json` lifecycle commands or install its listed
extensions automatically. The Docker workflow in [the main student guide](README.md)
provides that automation.

The attachment entry point is described in this
[Apple maintainer's example](https://github.com/apple/container/discussions/912).
Keep `dev.containers.dockerPath` at its normal value: setting it to `container`
doesn't make Apple's CLI implement Docker's commands.

## Build and prepare

Use a Mac with Apple silicon, macOS 26 or later, the
[Apple `container` CLI](https://github.com/apple/container), desktop VS Code,
and the Dev Containers extension. You'll also need a local checkout of this
repository. No host compiler, CMake, Python, or debugger is required.

From the repository root in a host terminal:

```sh
container system start
container build --arch arm64 --target devcontainer \
  --file multi-platform-dev/Dockerfile \
  --tag moving2arm:devcontainer-arm64 multi-platform-dev
container run --detach --name moving2arm-vscode --arch arm64 \
  --cpus 2 --memory 4g \
  --volume "$PWD:/workspace" --workdir /workspace \
  moving2arm:devcontainer-arm64 sleep infinity
container exec moving2arm-vscode sh -c 'cd /workspace && sh .devcontainer/setup.sh'
```

The mounted checkout retains edits and build output on your Mac. The container
runs as the image's `ubuntu` user and doesn't publish ports. Keep the named
container to retain its VS Code server and extensions between sessions.
Allocate memory explicitly for the desktop tools rather than relying on the
CLI's 1 GiB default. Start with one attached VS Code window.

## Attach the editor

1. Launch VS Code from the host terminal with `code .` so its PATH includes
   `container`. Check `command -v container` if the extension can't find it.
2. Open Settings and enable **Dev > Containers: Experimental Apple Container
   Support**, or add this to your VS Code user settings:

   ```json
   "dev.containers.experimentalAppleContainerSupport": true
   ```

   You can use a separate VS Code profile or user-data directory for this
   experimental workflow.
3. Run **Dev Containers: Attach to Running Apple Container...** from the Command
   Palette and select `moving2arm-vscode`.
4. In the attached window, open `/workspace`. Allow the remote VS Code server
   installation to finish.
5. In Extensions, install **C/C++** (`ms-vscode.cpptools`) and **CMake Tools**
   (`ms-vscode.cmake-tools`) in the Apple container. A host installation alone
   doesn't supply the container's debugger adapter.
6. Select **CMake: Select Configure Preset > Porting: Debug**, then
   **CMake: Configure**. To use CMake for IntelliSense, set
   `C_Cpp.default.configurationProvider` to `ms-vscode.cmake-tools` in remote
   settings.

Use **Tasks: Run Task > Porting: test all (Debug)** to build and test, then F5
with **Porting: debug scalar calculation** to debug. The
[student exercise](README.md#build-test-and-debug) covers breakpoints and values
to inspect. Start with native Arm64; debugging translated x86-64 isn't covered.

For CMake Tools' separate **CMake: Debug** command, add this remote setting to
match the F5 launcher's GDB configuration:

```json
"cmake.debugConfig": {
  "MIMode": "gdb",
  "miDebuggerPath": "/usr/bin/gdb",
  "setupCommands": [{"text": "set disable-randomization off"}]
}
```

## Resume and check

For a stopped container, run `container start moving2arm-vscode`, then attach
again. When finished, close the attached editor window and run
`container stop moving2arm-vscode`. Rebuild the image and create a container
with a new name when changing image dependencies; an existing container keeps
its original image contents.

For a command-line check in the same container:

```sh
container exec moving2arm-vscode sh -c \
  'cd /workspace && cmake --build --preset dev-debug && ctest --preset dev-debug && sh .devcontainer/check-debugger.sh'
```

The project doesn't need privileged mode, extra tracing capabilities, or a
disabled seccomp profile for its native Arm64 debugger check.

## Verified desktop workflow

Verified on October 28, 2026, with macOS 26.7.1, Apple `container` 1.0.0,
VS Code 1.139.1, Dev Containers 0.469.0, C/C++ 1.34.4, and CMake Tools 1.24.42.
The native Arm64 container used two CPUs and 4 GiB of RAM.

The check used an isolated VS Code profile and the real remote extension host:

- Attached the desktop editor and opened the exercise source in `/workspace`.
- Selected `dev-debug` and configured through CMake Tools.
- Executed **Porting: test all (Debug)** through VS Code's task API, including
  its configure/build dependencies; all 24 tests passed.
- Started **Porting: debug scalar calculation** through VS Code's debug API,
  hit a source breakpoint in `calculate`, inspected `n=4`, `a[0]=1`, and `b[1]=2`,
  stepped through the source, and continued to normal exit.

A first attempt using the default 1 GiB stalled and required a forced test-VM
shutdown. The 4 GiB, single-window retry passed; the exact cause of the first
stall wasn't established. Host VS Code settings weren't changed.

This validates experimental attachment to an existing Apple container. It
doesn't validate Docker's clone-in-volume/reopen lifecycle on Apple's engine,
or x86-64 debugging under translation.
