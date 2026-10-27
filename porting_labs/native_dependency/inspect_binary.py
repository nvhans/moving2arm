"""Inspect ELF/Mach-O CPU types, then optionally exercise the platform loader."""
import argparse
import ctypes
from pathlib import Path
import platform
import struct
import sys


def host_architecture():
    machine = platform.machine().lower()
    return {"arm64": "aarch64", "amd64": "x86_64"}.get(machine, machine)


def binary_architectures(path):
    data = Path(path).read_bytes()
    if data.startswith(b"\x7fELF"):
        if len(data) < 20 or data[4] not in (1, 2) or data[5] not in (1, 2):
            raise ValueError("Truncated or invalid ELF header")
        order = "<" if data[5] == 1 else ">"
        machine = struct.unpack_from(order + "H", data, 18)[0]
        return {62: {"x86_64"}, 183: {"aarch64"}}.get(machine, {f"ELF-machine-{machine}"})
    thin = {b"\xcf\xfa\xed\xfe": "<", b"\xfe\xed\xfa\xcf": ">"}
    cpu_names = {0x01000007: "x86_64", 0x0100000C: "aarch64"}
    if data[:4] in thin:
        if len(data) < 32:
            raise ValueError("Truncated Mach-O header")
        cpu = struct.unpack_from(thin[data[:4]] + "I", data, 4)[0]
        return {cpu_names.get(cpu, f"Mach-O-cpu-{cpu}")}
    fat = {b"\xca\xfe\xba\xbe": (">", 20), b"\xbe\xba\xfe\xca": ("<", 20),
           b"\xca\xfe\xba\xbf": (">", 32), b"\xbf\xba\xfe\xca": ("<", 32)}
    if data[:4] in fat:
        if len(data) < 8:
            raise ValueError("Truncated fat Mach-O header")
        order, entry_size = fat[data[:4]]
        count = struct.unpack_from(order + "I", data, 4)[0]
        if not 1 <= count <= 32 or len(data) < 8 + count * entry_size:
            raise ValueError("Invalid fat Mach-O architecture table")
        cpus = [struct.unpack_from(order + "I", data, 8 + i * entry_size)[0]
                for i in range(count)]
        return {cpu_names.get(cpu, f"Mach-O-cpu-{cpu}") for cpu in cpus}
    raise ValueError("Expected a Linux ELF or 64-bit macOS Mach-O binary")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binaries", type=Path, nargs="+")
    parser.add_argument("--expect", choices=("host", "x86_64", "aarch64"), default="host")
    parser.add_argument("--load", action="store_true", help="Also load your built shared library with ctypes")
    args = parser.parse_args()
    expected = host_architecture() if args.expect == "host" else args.expect
    failed = False
    for binary in args.binaries:
        try:
            arches = binary_architectures(binary)
            print(f"{binary.name}: {', '.join(sorted(arches))}; expected {expected}", flush=True)
            if expected not in arches:
                print("CPU architecture mismatch", file=sys.stderr)
                failed = True
            if args.load:
                ctypes.CDLL(str(binary.resolve()))
                print("Platform loader accepted the shared library.")
        except (OSError, ValueError) as error:
            print(f"{binary}: {error}", file=sys.stderr)
            failed = True
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
