"""Package this lab's existing CMake artifacts as a local CPython wheel.

This intentionally exposes the wheel format with standard-library tools. It
doesn't build code, download dependencies, install packages, or claim manylinux.
"""
import argparse
import base64
import csv
import hashlib
import io
from pathlib import Path
import platform
import sys
import sysconfig
import zipfile

sys.dont_write_bytecode = True
from inspect_binary import binary_architectures, host_architecture


def local_platform_tag():
    machine = host_architecture()
    if sys.platform == "darwin":
        # Conservatively require the build host's OS major version, rather than
        # mislabeling a new binary with an interpreter's older deployment tag.
        major, minor = platform.mac_ver()[0].split(".")[:2]
        # macOS 11+ compatibility tags use major_0, regardless of OS minor.
        if int(major) >= 11:
            minor = "0"
        return f"macosx_{major}_{minor}_{'arm64' if machine == 'aarch64' else machine}"
    if sys.platform.startswith("linux"):
        return f"linux_{machine}"
    raise ValueError("This lab packages native Linux and macOS builds")


def package(module, library, output):
    if sys.implementation.name != "cpython":
        raise ValueError("This module uses the CPython API")
    if module.name != "_moving2arm" + sysconfig.get_config_var("EXT_SUFFIX"):
        raise ValueError("Module filename doesn't match this interpreter's extension ABI")
    for artifact in (module, library):
        if binary_architectures(artifact) != {host_architecture()}:
            raise ValueError(f"{artifact}: wheel lab requires a single matching native architecture")
    interpreter = f"cp{sys.version_info.major}{sys.version_info.minor}"
    abi = interpreter + getattr(sys, "abiflags", "")
    tag = f"{interpreter}-{abi}-{local_platform_tag()}"
    name = "moving2arm_native"
    version = "0.1.0"
    info = f"{name}-{version}.dist-info"
    files = {
        module.name: module.read_bytes(),
        library.name: library.read_bytes(),
        f"{info}/METADATA": (
            "Metadata-Version: 2.1\nName: moving2arm-native\nVersion: 0.1.0\n"
            "Summary: Native dependency porting exercise\nRequires-Python: >=3.9\n\n"
        ).encode(),
        f"{info}/WHEEL": (
            f"Wheel-Version: 1.0\nGenerator: moving2arm-lab\nRoot-Is-Purelib: false\nTag: {tag}\n"
        ).encode(),
    }
    rows = []
    for path, content in files.items():
        digest = base64.urlsafe_b64encode(hashlib.sha256(content).digest()).rstrip(b"=").decode()
        rows.append((path, "sha256=" + digest, str(len(content))))
    rows.append((f"{info}/RECORD", "", ""))
    record = io.StringIO(newline="")
    csv.writer(record).writerows(rows)
    files[f"{info}/RECORD"] = record.getvalue().encode()
    output.mkdir(parents=True, exist_ok=True)
    wheel = output / f"{name}-{version}-{tag}.whl"
    with zipfile.ZipFile(wheel, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for path, content in files.items():
            archive.writestr(path, content)
    return wheel


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(package(args.module, args.library, args.output))


if __name__ == "__main__":
    main()
