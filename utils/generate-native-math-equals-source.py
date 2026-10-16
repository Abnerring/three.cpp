#!/usr/bin/env python3
"""Record the six ported equals methods as source metadata, never executable JS."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REVISION = "11738a9d55e502fb484cefffcea3a66dc44e9fe9"
NAMES = ("Vector2", "Vector3", "Vector4", "Color", "Matrix3", "Matrix4")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("upstream", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    revision = subprocess.check_output(["git", "-C", str(args.upstream), "rev-parse", "HEAD"], text=True).strip()
    if revision != REVISION:
        raise ValueError("Unexpected three.js revision")
    output = []
    for name in NAMES:
        relative = f"src/math/{name}.js"
        raw = (args.upstream / relative).read_bytes()
        blob = subprocess.check_output(["git", "-C", str(args.upstream), "show", f"HEAD:{relative}"])
        if raw.replace(b"\r\n", b"\n") != blob:
            raise ValueError(f"Modified upstream source: {relative}")
        source = raw.decode("utf-8")
        match = re.search(r"\bequals\( [^\r\n]* \) \{", source)
        if not match:
            raise ValueError(f"Missing equals method: {relative}")
        close = re.search(r"\r?\n\t\}", source[match.end():])
        if not close:
            raise ValueError(f"Missing equals method end: {relative}")
        method = source[match.start():match.end() + close.end()]
        output.append(f"constexpr const char* source_equals_{name} = {json.dumps(method, ensure_ascii=True)};")
    generated = ("\n".join(output) + "\n").encode("utf-8")
    target = root / "src/core/NativeMathEqualsSource.inc"
    if args.check:
        # C++ file line endings are independent of the escaped source strings.
        # json.dumps preserves the upstream method CR/LF as literal backslashes.
        if target.read_text(encoding="utf-8") != generated.decode("utf-8"):
            raise ValueError("Generated metadata differs from the exact upstream checkout text")
    else:
        target.write_bytes(generated)
    print(f"Six exact equals sources: {hashlib.sha256(generated).hexdigest()}")

if __name__ == "__main__":
    main()
