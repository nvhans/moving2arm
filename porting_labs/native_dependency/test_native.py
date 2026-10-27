"""Validate the C extension, CPU metadata, relocation, and wheel RECORD hashes."""
import argparse
import base64
import csv
import hashlib
import importlib.util
import io
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import zipfile

sys.dont_write_bytecode = True
from inspect_binary import binary_architectures, host_architecture
from package_wheel import package


def checksum_reference(data):
    value = 0xCBF29CE484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001B3) & ((1 << 64) - 1)
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert host_architecture() in binary_architectures(args.module)
    assert host_architecture() in binary_architectures(args.library)
    spec = importlib.util.spec_from_file_location("_moving2arm", args.module)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    for data in (b"", b"hello", b"\0\xff", bytes(range(256)) * 4):
        assert module.checksum(data) == checksum_reference(data)
    try:
        module.checksum("not bytes")
    except TypeError:
        pass
    else:
        raise AssertionError("Extension accepted text instead of bytes")

    # Inspect synthetic headers without executing them. Loading the wrong real
    # architecture is a separate reproducible exercise in README.md.
    args.output.mkdir(parents=True, exist_ok=True)
    stage = Path(tempfile.mkdtemp(prefix="wheel-check-", dir=args.output))
    for machine, expected in ((62, "x86_64"), (183, "aarch64")):
        header = bytearray(64)
        header[:6] = b"\x7fELF\x02\x01"
        struct.pack_into("<H", header, 18, machine)
        fixture = stage / f"elf-{machine}"
        fixture.write_bytes(header)
        assert binary_architectures(fixture) == {expected}
    for cpu, expected in ((0x01000007, "x86_64"), (0x0100000C, "aarch64")):
        fixture = stage / f"macho-{cpu}"
        fixture.write_bytes(b"\xcf\xfa\xed\xfe" + struct.pack("<I", cpu) + bytes(24))
        assert binary_architectures(fixture) == {expected}
    other = "x86_64" if host_architecture() == "aarch64" else "aarch64"
    guard = subprocess.run([sys.executable, str(Path(__file__).with_name("inspect_binary.py")),
                            str(args.library), "--expect", other], capture_output=True, text=True)
    assert guard.returncode == 1 and "mismatch" in guard.stderr
    foreign_header = stage / ("elf-62" if other == "x86_64" else "elf-183")
    try:
        package(args.module, foreign_header, stage)
    except ValueError:
        pass
    else:
        raise AssertionError("Packager mislabeled a foreign-architecture library as native")

    wheel = package(args.module, args.library, stage)
    extracted = stage / "extracted"
    with zipfile.ZipFile(wheel) as archive:
        records = [name for name in archive.namelist() if name.endswith(".dist-info/RECORD")]
        assert len(records) == 1
        rows = list(csv.reader(io.StringIO(archive.read(records[0]).decode())))
        assert {row[0] for row in rows} == set(archive.namelist())
        for name, digest, size in rows:
            assert not Path(name).is_absolute() and ".." not in Path(name).parts
            if name == records[0]:
                assert digest == size == ""
            else:
                content = archive.read(name)
                expected = base64.urlsafe_b64encode(hashlib.sha256(content).digest()).rstrip(b"=").decode()
                assert digest == "sha256=" + expected and int(size) == len(content)
        archive.extractall(extracted)

    # Run a fresh isolated interpreter with no path back to the build directory.
    env = {key: value for key, value in os.environ.items()
           if key not in ("LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH")}
    check = (
        "import sys; sys.path.insert(0, sys.argv[1]); import _moving2arm; "
        "assert _moving2arm.checksum(b'hello') == 0xa430d84680aabd0b"
    )
    subprocess.run([sys.executable, "-I", "-c", check, str(extracted.resolve())],
                   check=True, env=env, cwd=stage)
    missing_library = stage / "missing-library"
    missing_library.mkdir()
    shutil.copy2(args.module, missing_library / args.module.name)
    missing = subprocess.run([sys.executable, "-I", "-c", check, str(missing_library.resolve())],
                             capture_output=True, text=True, env=env, cwd=stage)
    assert missing.returncode != 0 and "ImportError" in missing.stderr
    print(f"Extension, CPU guard, wheel hashes, relocation, and missing-library rejection passed: {wheel}")


if __name__ == "__main__":
    main()
