#!/usr/bin/env python3
"""Small offline secret scan for public-source and CI checks.

Findings report only file, line, and rule name; candidate secret text is never printed.
"""

from pathlib import Path
import re
import sys

RULES = {
    "private-key": re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
    "github-token": re.compile(r"(?:ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{30,})"),
    "aws-access-key": re.compile(r"\bAKIA[0-9A-Z]{16}\b"),
    "generic-assignment": re.compile(
        r"(?i)\b(?:api[_-]?key|access[_-]?token|auth[_-]?token|password|client[_-]?secret)\b"
        r"\s*[:=]\s*['\"][^'\"\n]{8,}['\"]"
    ),
}
SKIP_DIRS = {".git", "build", "__pycache__"}
TEXT_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hpp", ".py", ".sh", ".md", ".txt",
                 ".json", ".yml", ".yaml", ".vert", ".frag", ".in"}


def main() -> int:
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
    findings = []
    checked = 0
    for path in root.rglob("*"):
        if not path.is_file() or any(part in SKIP_DIRS for part in path.relative_to(root).parts):
            continue
        if path.name not in {"LICENSE", "NOTICE", ".gitignore"} and path.suffix not in TEXT_SUFFIXES:
            continue
        checked += 1
        text = path.read_text(errors="replace")
        for line_number, line in enumerate(text.splitlines(), 1):
            for rule, pattern in RULES.items():
                if pattern.search(line):
                    findings.append((path.relative_to(root), line_number, rule))
    for path, line, rule in findings:
        print(f"SECRET_CANDIDATE file={path} line={line} rule={rule}", file=sys.stderr)
    if findings:
        print(f"SECRET_SCAN=FAIL findings={len(findings)}", file=sys.stderr)
        return 1
    print(f"SECRET_SCAN=PASS files={checked}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
