#!/usr/bin/env python3
import os
import shutil

ROOT = os.path.dirname(os.path.abspath(__file__))
VCPKG_DIR = os.path.join(ROOT, "vcpkg")
GRAMMARS_DIR = os.path.join(ROOT, "grammars")
ZIG_OUT = os.path.join(ROOT, "zig-out")
ZIG_CACHE = os.path.join(ROOT, "zig-cache")

def rm(path):
    if os.path.isdir(path):
        print(f"[-] Removing directory: {path}")
        shutil.rmtree(path, ignore_errors=True)
    elif os.path.isfile(path):
        print(f"[-] Removing file: {path}")
        try:
            os.remove(path)
        except OSError:
            pass

def main():
    print("[*] Cleaning up initialized environment...")
    rm(VCPKG_DIR)
    rm(GRAMMARS_DIR)
    rm(ZIG_OUT)
    rm(ZIG_CACHE)

    # optional: remove vcpkg lockfiles
    rm(os.path.join(ROOT, "vcpkg_installed"))
    rm(os.path.join(ROOT, "vcpkg-lock.json"))

    print("[✓] Cleanup complete.")

if __name__ == "__main__":
    main()
