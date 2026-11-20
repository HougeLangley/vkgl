#!/usr/bin/env python3
"""Source-tree, build-flag, manifest, and public-release policy checks."""

import argparse
import hashlib
import json
import platform
from pathlib import Path
import re
import subprocess
import sys

REQUIRED = [
    "README.md", "CONTRIBUTING.md", "ROADMAP.md", "HARDWARE.md", "TESTING.md",
    "SECURITY.md", "SUPPORT.md", "CHANGELOG.md", "LICENSE", "NOTICE",
    "CODE_OF_CONDUCT.md", ".github/PULL_REQUEST_TEMPLATE.md",
    ".github/ISSUE_TEMPLATE/bug_report.yml", ".github/ISSUE_TEMPLATE/config.yml",
    ".github/workflows/build.yml", "tests/shaders/basic.vert", "tests/shaders/basic.frag",
    "licenses/SPIRV-Cross-LICENSE", "licenses/SPIRV-Cross-MIT.txt",
]
BINARY_SUFFIXES = {".o", ".a", ".so", ".pyc", ".bin", ".png", ".jpg", ".zip"}
PRIVATE_PATH = re.compile(r"(?:/h[o]me/[^\s/'\"]+/|/U[s]ers/[^\s/'\"]+/)")

# Keep a self-test that exercises real private-path detection without spelling
# a private absolute path literally in this public detector.
for _private_example in ("/" + "home" + "/example/project", "/" + "Users" + "/example/project"):
    if not PRIVATE_PATH.search(_private_example):
        raise AssertionError("private-path detector negative self-test failed")
if PRIVATE_PATH.search("/etc/vkgl/config.json"):
    raise AssertionError("private-path detector reported a public system path")


def fail(message: str) -> None:
    raise AssertionError(message)


def is_forbidden_artifact(path: Path) -> bool:
    return path.suffix in BINARY_SUFFIXES or ".so." in path.name


def public_source_files(root: Path, build: Path) -> list[Path]:
    """Enumerate publishable sources, never the ignored in-tree build output.

    Git checkouts include tracked and untracked/non-ignored sources. Archives
    exclude only the active build directory. Root-level binaries are added to
    the audit even when ignored, so a real libvkgl.so cannot hide behind
    .gitignore. Tracked binaries under build directories remain forbidden.
    """
    files: set[Path] = set()
    if (root / ".git").exists():
        try:
            output = subprocess.check_output([
                "git", "-C", str(root), "ls-files", "-z",
                "--cached", "--others", "--exclude-standard",
            ])
        except (OSError, subprocess.CalledProcessError) as error:
            fail(f"cannot enumerate public sources with git ls-files: {error}")
        for relative in output.decode("utf-8").split("\0"):
            if relative:
                files.add(root / relative)
    else:
        for path in root.rglob("*"):
            if not path.is_file() or ".git" in path.parts:
                continue
            if build == path or build in path.parents:
                continue
            files.add(path)

    for path in root.iterdir():
        if path.is_file() and is_forbidden_artifact(path):
            files.add(path)
    return sorted(files)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", required=True, type=Path)
    parser.add_argument("--build-dir", required=True, type=Path)
    args = parser.parse_args()
    root = args.source_dir.resolve()
    build = args.build_dir.resolve()

    for relative in REQUIRED:
        if not (root / relative).is_file():
            fail(f"missing required public file: {relative}")

    expected_lines = (root / "tests/source-sha256.txt").read_text().splitlines()
    if len(expected_lines) != 1:
        fail("tests/source-sha256.txt must contain exactly one source hash")
    expected, relative = expected_lines[0].split(maxsplit=1)
    actual = hashlib.sha256((root / relative).read_bytes()).hexdigest()
    if actual != expected:
        fail(f"source hash mismatch: {relative}: {actual} != {expected}")

    files = public_source_files(root, build)
    if not is_forbidden_artifact(root / "release-policy-negative.so"):
        fail("root-level .so negative self-test failed")
    if is_forbidden_artifact(root / "src" / "vkgl.cpp"):
        fail("source file misclassified as a binary artifact")
    for path in files:
        if not path.is_file():
            fail(f"enumerated public source is missing: {path}")
        relative = path.relative_to(root)
        if is_forbidden_artifact(path):
            fail(f"binary/build artifact in public sources: {relative}")
        if path.stat().st_size > 3_000_000:
            fail(f"oversized source-tree file: {relative}")
        text = path.read_text(errors="replace")
        if PRIVATE_PATH.search(text):
            fail(f"private absolute path in public source: {relative}")

    notice = (root / "NOTICE").read_text()
    for marker in ("Collabora", "Permission is hereby granted", "SPIRV-Cross",
                   "Copyright 2015-2021 Arm Limited", "Copyright 2016-2021 Arm Limited",
                   "Copyright 2018-2021 Arm Limited", "Hans-Kristian Arntzen",
                   "Copyright: 2014-2024 The Khronos Group Inc.",
                   "Copyright 2016-2021 Google Inc."):
        if marker not in notice:
            fail(f"NOTICE lacks required marker: {marker}")
    if "Google Inc. and SPIRV-Cross contributors" in notice:
        fail("NOTICE contains an unverified merged SPIRV-Cross attribution")

    for shader in ("basic.vert", "basic.frag"):
        text = (root / "tests/shaders" / shader).read_text()
        if "#version 100" not in text or "void main" not in text:
            fail(f"invalid public shader fixture: {shader}")

    commands_path = build / "compile_commands.json"
    commands = json.loads(commands_path.read_text())
    vkgl_commands = [
        entry for entry in commands
        if entry["file"].endswith("/src/vkgl.cpp")
        or entry["file"].endswith("/protocol/linux-dmabuf-protocol.c")
    ]
    if len(vkgl_commands) != 2:
        fail(f"expected two vkgl compile commands, got {len(vkgl_commands)}")
    is_loongarch64 = platform.machine().lower() == "loongarch64"
    for entry in vkgl_commands:
        command = entry["command"]
        if is_loongarch64:
            for flag in ("-O3", "-march=loongarch64", "-mtune=la664", "-flto"):
                if flag not in command:
                    fail(f"missing loongarch64 production flag {flag}: {command}")
        else:
            for flag in ("-march=loongarch64", "-mtune=la664"):
                if flag in command:
                    fail(f"LoongArch flag leaked to {platform.machine()}: {command}")

    build_manifest = json.loads((build / "vkgl_icd_build.json").read_text())
    library = Path(build_manifest["ICD"]["library_path"])
    if not library.is_file() or build not in library.parents:
        fail(f"build ICD manifest does not reference the built library: {library}")
    install_manifest = json.loads((build / "vkgl_icd.json").read_text())
    if install_manifest["ICD"]["api_version"] != "1.0.0":
        fail("unexpected ICD API version")

    candidate_doc = root / "EXTERNAL_DMABUF.md"
    if candidate_doc.exists():
        text = candidate_doc.read_text()
        for marker in ("15.35%", "not been released to production", "Vulkan Video",
                       "full zero-copy", "external synchronization"):
            if marker not in text:
                fail(f"candidate boundary missing from EXTERNAL_DMABUF.md: {marker}")

    print(f"PASS host-repository-policy files={len(files)} sha256={actual} arch={platform.machine()}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (AssertionError, OSError, ValueError, json.JSONDecodeError) as error:
        print(f"FAIL host-repository-policy: {error}", file=sys.stderr)
        sys.exit(1)
