"""Exercise the GUI image with Docker or Apple container and private state."""
import argparse
import hashlib
import http.cookiejar
import os
from pathlib import Path
import re
import socket
import subprocess
import time
import urllib.error
import urllib.parse
import urllib.request
import uuid


def wait_for_editor(url, timeout=90):
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with opener.open(url + "/healthz", timeout=3) as response:
                if response.status == 200:
                    return
        except (OSError, urllib.error.URLError):
            pass
        time.sleep(0.5)
    raise TimeoutError("code-server didn't become ready")


def check_login(url, config):
    text = config.read_text()
    assert re.search(r"^auth:\s*password\s*$", text, re.M)
    password = re.search(r"^password:\s*(\S+)\s*$", text, re.M)
    assert password, "No generated login password in the persisted configuration"
    opener = urllib.request.build_opener(
        urllib.request.ProxyHandler({}),
        urllib.request.HTTPCookieProcessor(http.cookiejar.CookieJar()))
    with opener.open(url + "/", timeout=30) as response:
        assert urllib.parse.urlsplit(response.url).path == "/login"
    request = urllib.request.Request(
        url + "/login", method="POST",
        data=urllib.parse.urlencode({"password": password.group(1)}).encode(),
        headers={"Content-Type": "application/x-www-form-urlencoded", "Origin": url})
    with opener.open(request, timeout=60) as response:
        assert response.status == 200
        assert urllib.parse.urlsplit(response.url).path != "/login", "Login was rejected"
        assert response.read(), "Editor returned an empty page"
    return hashlib.sha256(config.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runtime", choices=("docker", "container"), default="docker")
    parser.add_argument("--arch", choices=("amd64", "arm64"), help="Explicit image architecture")
    parser.add_argument("--image", required=True)
    parser.add_argument("--workspace", type=Path, required=True)
    parser.add_argument("--state-dir", type=Path, required=True)
    args = parser.parse_args()
    architecture_args = []
    if args.arch:
        architecture_args = ["--arch", args.arch] if args.runtime == "container" else [
            "--platform", "linux/" + args.arch]

    def runtime(*args_, capture=False):
        return subprocess.run([args.runtime, *map(str, args_)], check=True,
                              text=True, capture_output=capture).stdout

    workspace = args.workspace.resolve()
    state = args.state_dir.resolve()
    # Use a new test-state directory: never overwrite an existing user's config.
    state.mkdir(mode=0o700, parents=True, exist_ok=False)
    for directory in ("config", "data"):
        (state / directory).mkdir(mode=0o700)
    config = state / "config/code-server/config.yaml"
    prefix = "moving2arm-gui-" + uuid.uuid4().hex[:12]
    build_dir = "/workspace/build/" + prefix
    active = []
    try:
        config_digest = None
        for generation in (1, 2):
            # Select a free loopback port separately for each container.
            with socket.socket() as reservation:
                reservation.bind(("127.0.0.1", 0))
                port = reservation.getsockname()[1]
            url = f"http://127.0.0.1:{port}"
            name = f"{prefix}-{generation}"
            runtime("run", "--detach", "--rm", "--name", name,
                   "--user", f"{os.getuid()}:{os.getgid()}",
                   "--publish", f"127.0.0.1:{port}:8080",
                   "--volume", f"{workspace}:/workspace",
                   "--volume", f"{state / 'config'}:/config",
                   "--volume", f"{state / 'data'}:/data", *architecture_args, args.image)
            active.append(name)
            wait_for_editor(url)
            digest = check_login(url, config)
            if generation == 1:
                config_digest = digest
                runtime("exec", name, "sh", "-c", "printf persistent > /data/smoke-marker")
                runtime("exec", name, "cmake", "-S", "/workspace", "-B",
                       build_dir, "-DCMAKE_BUILD_TYPE=Debug")
                runtime("exec", name, "cmake", "--build", build_dir,
                       "--target", "check_simple_port", "--parallel", "2")
            else:
                assert digest == config_digest, "A fresh container changed the saved login config"
                marker = runtime("exec", name, "cat", "/data/smoke-marker", capture=True)
                assert marker == "persistent", "Editor data wasn't retained"
            runtime("stop", name)
            active.remove(name)
    finally:
        for name in active:
            subprocess.run([args.runtime, "stop", name],
                           check=False, capture_output=True)
    print("GUI login, editor page, CMake tests, and state across fresh containers passed.")


if __name__ == "__main__":
    main()
