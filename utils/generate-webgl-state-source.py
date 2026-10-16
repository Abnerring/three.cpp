"""Generate exact Function#toString metadata from the pinned physical source."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

COMMIT = '11738a9d55e502fb484cefffcea3a66dc44e9fe9'
RELATIVE = 'src/renderers/webgl/WebGLState.js'

def function(text, start):
    opening = text.index('{', start)
    depth, index = 1, opening + 1
    while depth:
        char = text[index]
        if text.startswith('//', index):
            index = text.index('\n', index) + 1
            continue
        if text.startswith('/*', index):
            index = text.index('*/', index) + 2
            continue
        if char in ('"', "'", '`'):
            quote = char
            index += 1
            while text[index] != quote:
                index += 2 if text[index] == '\\' else 1
            index += 1
            continue
        if char == '{': depth += 1
        elif char == '}': depth -= 1
        index += 1
    return text[start:index]

def generate(upstream):
    revision = subprocess.check_output(['git', '-C', str(upstream), 'rev-parse', 'HEAD'], text=True).strip()
    if revision != COMMIT: raise RuntimeError('Wrong upstream commit')
    raw = (upstream / RELATIVE).read_bytes()
    blob = subprocess.check_output(['git', '-C', str(upstream), 'show', f'{COMMIT}:{RELATIVE}'])
    if raw.replace(b'\r\n', b'\n') != blob: raise RuntimeError('Modified upstream source')
    text = raw.decode('utf-8')
    sources = {}
    for match in re.finditer(r'function ([A-Za-z0-9_]+)\(', text):
        name = match.group(1)
        sources[name] = function(text, match.start())
    for group in ('ColorBuffer', 'DepthBuffer', 'StencilBuffer'):
        body = sources[group]
        for match in re.finditer(r'([A-Za-z0-9_]+): (function \()', body):
            sources[group + '_' + match.group(1)] = function(body, match.start(2))
    output = '// Exact source metadata; generated from pinned three.js. See THREEJS-LICENSE.\n'
    output += '// Physical upstream SHA256: ' + hashlib.sha256(raw).hexdigest() + '\n'
    output += 'namespace WebGLStateSource {\n'
    for name, source in sources.items():
        output += f'inline constexpr const char* {name} = {json.dumps(source, ensure_ascii=True)};\n'
    return output + '}\n'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('upstream', type=Path)
    parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1] / 'src/renderers/webgl/WebGLStateSource.inc')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    result = generate(args.upstream)
    if args.check:
        if args.output.read_text(encoding='utf-8') != result: raise RuntimeError('Stale source metadata')
    else: args.output.write_text(result, encoding='utf-8', newline='\n')
    print(hashlib.sha256(result.encode('utf-8')).hexdigest())

if __name__ == '__main__': main()
