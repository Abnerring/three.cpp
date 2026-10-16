"""Regenerate exact reflection text from the pinned raw source file."""
import hashlib
import json
import pathlib
import subprocess
import sys

COMMIT = "11738a9d55e502fb484cefffcea3a66dc44e9fe9"
RAW_SHA256 = "02392701664a8386da60a0b1e797b28342ff4036f4cf317737c8cf654406257b"
upstream = pathlib.Path(sys.argv[1]).resolve()
actual_commit = subprocess.check_output(["git", "-C", str(upstream), "rev-parse", "HEAD"], text=True).strip()
if actual_commit != COMMIT:
    raise SystemExit(f"Expected upstream commit {COMMIT}, got {actual_commit}")
raw = (upstream / "src/renderers/webgl/WebGLUniformsGroups.js").read_bytes()
actual_hash = hashlib.sha256(raw).hexdigest()
if actual_hash != RAW_SHA256:
    raise SystemExit(f"Expected raw source SHA256 {RAW_SHA256}, got {actual_hash}")
source = raw.decode("utf-8")

def function(name):
    start = source.index("function " + name + "(")
    brace = source.index("{", start)
    depth = 0
    # The pinned text has no braces in quoted literals or template literals.
    for i in range(brace, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[start:i + 1]
    raise ValueError(name)

result = f"// Exact raw source reflection metadata from locked three.js {COMMIT}.\n// Raw source SHA256: {RAW_SHA256}. Newlines and comments are preserved.\n// Never parsed or executed. See THREEJS-LICENSE.\n"
for name in ["WebGLUniformsGroups", "bind", "update", "onUniformsGroupsDispose", "dispose"]:
    result += f"static constexpr const char* source_{name} =\n"
    result += "".join(json.dumps(line, ensure_ascii=True) + "\n" for line in function(name).splitlines(keepends=True)) + ";\n"
output = pathlib.Path(sys.argv[2]) if len(sys.argv) > 2 else pathlib.Path(__file__).resolve().parents[1] / "src/renderers/webgl/WebGLUniformsGroupsSource.inc"
output.write_text(result, encoding="utf-8", newline="\n")
