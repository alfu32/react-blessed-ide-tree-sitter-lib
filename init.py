#!/usr/bin/env python3
import csv, os, subprocess, sys, platform, shutil, urllib.request, zipfile, tarfile

ROOT = os.path.dirname(os.path.abspath(__file__))
VCPKG_DIR = os.path.join(ROOT, "vcpkg")
GRAMMARS_DIR = os.path.join(ROOT, "grammars")
ZIG_DIR = os.path.join(ROOT, "zig")
CSV_PATH = os.path.join(ROOT, "dl-libs.csv")
ZIG_VERSION = "0.13.0"  # change if needed

def run(cmd, cwd=None):
    print("[*]", " ".join(cmd))
    subprocess.run(cmd, cwd=cwd, check=True)

def ensure_vcpkg():
    if not os.path.isdir(VCPKG_DIR):
        print("[+] Cloning vcpkg...")
        run(["git", "clone", "https://github.com/microsoft/vcpkg.git", VCPKG_DIR])
        run(["./bootstrap-vcpkg.sh", "-disableMetrics"], cwd=VCPKG_DIR)
    else:
        print("[=] vcpkg already present")

def install_vcpkg_deps():
    print("[+] Installing dependencies via vcpkg manifest...")
    vcpkg_bin = os.path.join(VCPKG_DIR, "vcpkg")
    run([vcpkg_bin, "install"])

def ensure_zig():
    if os.path.isdir(ZIG_DIR):
        print("[=] Zig already downloaded.")
        return

    system = platform.system().lower()
    arch = platform.machine().lower()
    if "arm" in arch or "aarch" in arch:
        arch = "aarch64"
    elif arch in ("x86_64", "amd64"):
        arch = "x86_64"
    else:
        arch = "x86_64"

    if system == "linux":
        ext = "tar.xz"
    elif system == "darwin":
        ext = "tar.xz"
    elif system == "windows":
        ext = "zip"
    else:
        print("Unsupported OS:", system)
        sys.exit(1)

    zig_filename = f"zig-{system}-{arch}-{ZIG_VERSION}.{ext}"
    zig_url = f"https://ziglang.org/download/{ZIG_VERSION}/{zig_filename}"
    local_path = os.path.join(ROOT, zig_filename)

    print(f"[+] Downloading Zig {ZIG_VERSION} from {zig_url}")
    urllib.request.urlretrieve(zig_url, local_path)

    print("[*] Extracting Zig...")
    if ext == "zip":
        with zipfile.ZipFile(local_path, "r") as zf:
            zf.extractall(ROOT)
    else:
        # fallback: use system tar (handles xz)
        subprocess.run(["tar", "-xf", local_path, "-C", ROOT], check=True)

    unpacked = [d for d in os.listdir(ROOT) if d.startswith("zig-") and os.path.isdir(d)]
    if unpacked:
        os.rename(unpacked[0], ZIG_DIR)

    os.remove(local_path)
    print("[✓] Zig installed in", ZIG_DIR)

def bind_zig():
    zig_bin = os.path.join(ZIG_DIR, "zig.exe" if platform.system() == "Windows" else "zig")
    os.environ["PATH"] = f"{os.path.dirname(zig_bin)}{os.pathsep}{os.environ['PATH']}"
    print("[*] Zig bound to current console PATH.")
    try:
        subprocess.run([zig_bin, "version"], check=True)
    except subprocess.CalledProcessError:
        print("[!] Zig binding check failed")

def load_csv(path):
    items = []
    with open(path, newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f, delimiter=";")
        for row in reader:
            items.append(row)
    return items

def clone_grammars(entries):
    os.makedirs(GRAMMARS_DIR, exist_ok=True)
    for e in entries:
        name = e["NAME"].strip()
        url = e["URL"].strip()
        dl_flag = e["DOWNLOAD"].strip().lower()
        target = os.path.join(GRAMMARS_DIR, f"tree-sitter-{name}")
        if dl_flag in ("true", "yes", "1"):
            if os.path.isdir(target):
                print(f"[=] {name} already cloned")
            else:
                print(f"[+] Cloning {name} from {url}")
                try:
                    run(["git", "clone", "--depth", "1", url, target])
                except subprocess.CalledProcessError:
                    print(f"[!] Failed to clone {url}")
        elif dl_flag == "maybe":
            print(f"[?] Skipping {name} (marked maybe)")
        else:
            print(f"[-] Skipping {name} (DOWNLOAD={dl_flag})")

def main():
    if not os.path.exists(CSV_PATH):
        print("Error: dl-libs.csv not found")
        sys.exit(1)

    grammars = load_csv(CSV_PATH)
    ensure_vcpkg()
    install_vcpkg_deps()
    ensure_zig()
    bind_zig()
    clone_grammars(grammars)

    print("[✓] Initialization complete.")
    print(f"vcpkg root: {VCPKG_DIR}")
    print(f"Zig path: {ZIG_DIR}")
    print(f"Grammars: {GRAMMARS_DIR}")

if __name__ == "__main__":
    main()
