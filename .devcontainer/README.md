# Local VS Code learning environment

The container supplies GCC/G++ 12, CMake, Ninja, GDB, Python, SIMDe, and xsimd.
You don't need a compiler, debugger, Python, CMake, or Git on your host for the
recommended workflow.

## Host requirements

- [Visual Studio Code desktop](https://code.visualstudio.com/download).
- The [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).
- A running local Docker engine with Linux containers. On macOS or Windows,
  [Docker Desktop](https://docs.docker.com/desktop/) is the simplest documented
  route. On Linux, use [Docker Engine](https://docs.docker.com/engine/install/)
  with permission to run containers as your user.
- Internet access for the first image build, repository/submodule clone,
  VS Code server, and extensions. Later rebuilds can reuse cached downloads.

Check `docker version` in a host terminal: both client and server must respond.
Students don't need the standalone Dev Containers CLI or Node.js.
See Microsoft's [local container prerequisites](https://code.visualstudio.com/docs/devcontainers/containers#_getting-started).

Apple's `container` CLI also supports an
[experimental desktop attachment workflow](README-apple-container.md), as well
as the project's CLI and [browser editor](../multi-platform-dev/README-code-server.md).
The automated clone/reopen workflow below uses Docker. Apple's attachment route
requires explicit container setup and remote extension installation. Both routes
use the same toolchain Dockerfile.

## Start without host Git

1. Start your container engine and open VS Code.
2. Open the Command Palette (F1) and run
   **Dev Containers: Clone Repository in Container Volume...**.
3. Enter `https://github.com/nvhans/moving2arm` and choose its default branch.
   If prompted for a configuration, use the repository's configuration.
4. Review the repository and accept workspace trust if appropriate. Wait for the
   image build and **Running postCreateCommand** to finish. Setup initializes
   submodules, configures Debug, and builds the scalar starter automatically.
5. Check that the lower-left remote indicator shows the moving2arm container.
   Open **Terminal > New Terminal**. Commands now run in Linux with the supplied
   tools.

This is VS Code's [clone-in-volume workflow](https://code.visualstudio.com/docs/devcontainers/containers#_quick-start-open-a-git-repository-or-github-pr-in-an-isolated-container-volume).
Your exercises live in a Docker volume. Closing VS Code or rebuilding the
container preserves that volume; deleting the volume loses uncommitted work.
Use VS Code's Source Control view or container Git to commit your exercises and
back them up to your own repository when ready.

For an existing local checkout, open its root folder in VS Code and run
**Dev Containers: Reopen in Container** instead. A new host checkout can use
`git clone --recurse-submodules https://github.com/nvhans/moving2arm.git` if host
Git is available. This route keeps the source and build output in that folder.

## Build, test, and debug

1. Run **CMake: Select Configure Preset > Porting: Debug**, then
   **CMake: Configure** to initialize CMake Tools' code navigation and target
   controls. Setup has already configured the starter build; this also selects
   that configuration in the extension.
2. Run **Tasks: Run Build Task** (`Ctrl+Shift+B`, or `Cmd+Shift+B` on macOS).
   The default **Porting: build Debug** task configures and builds all available
   examples and labs.
3. Run **Tasks: Run Task > Porting: test all (Debug)**. For just the SIMD
   exercises, select **Porting: test SIMD (Debug)**.
4. Open `simple_port/calculation.c`. Select **Porting: debug scalar calculation**
   in Run and Debug, then press F5. Execution stops at entry; set a breakpoint
   inside `calculate`, continue, and inspect `n`, `a[0]`, and `b[1]`. Step through
   the loop and compare the mask and square-root results.
5. Select a target with **CMake: Set Debug Target** and run **CMake: Debug** to
   explore another example. The F5 starter stays on the scalar Debug target.

The image selects its native architecture automatically: x86-64 enables SSE;
AArch64 enables NEON. Compare both on native systems when available. Debugging
under architecture translation isn't covered by this workflow.

Equivalent commands in the container terminal:

```sh
cmake --preset dev-debug
cmake --build --preset dev-debug
ctest --preset dev-debug
sh .devcontainer/check-debugger.sh
```

Additional presets use separate build directories:

| Preset | Purpose |
| --- | --- |
| `dev-debug` | Debug symbols, all examples and labs, Python extension disabled |
| `dev-release` | Optimized builds for comparing generated instructions |
| `dev-sanitize` | AddressSanitizer and UndefinedBehaviorSanitizer |
| `dev-python` | Debug build with the optional Python extension and wheel test |

For example, run `cmake --preset dev-python`,
`cmake --build --preset dev-python`, then `ctest --preset dev-python`.
Build trees are under `build/devcontainer-<amd64|arm64>/<preset>` so they don't
collide when a checkout moves between architectures. Presets target the Linux
container toolchain; use the main README's native commands outside it.

## Setup checks

If setup fails, open **Dev Containers: Show Container Log** and address the first
failure. A failed download or submodule clone needs working network/proxy access
inside the engine as well as on the host. Retry **Dev Containers: Rebuild
Container** after correcting that access.

If F5 can't start GDB, run `sh .devcontainer/check-debugger.sh` in the container
terminal and retain the error for diagnosis. The configuration doesn't require
privileged mode or an unrestricted seccomp profile.

The container uses the existing `ubuntu` image user. Dev Containers adjusts its
UID/GID for Linux bind mounts; host accounts aren't modified. See Microsoft's
[non-root user documentation](https://code.visualstudio.com/remote/advancedcontainers/add-nonroot-user).

The [container CI workflow](../.github/workflows/containers.yml) checks setup,
the full Debug suite, and GDB breakpoints/variables through the Dev Containers
CLI on native Linux x86-64 and AArch64. Desktop extension installation and UI
attachment require a desktop check. The
[Apple attachment guide](README-apple-container.md#verified-desktop-workflow)
records the completed native Arm64 desktop verification.
