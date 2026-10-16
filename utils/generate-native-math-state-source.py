"""Exact metadata for math methods required by the WebGLState protocol."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import importlib.util
import sys
sys.dont_write_bytecode = True
spec=importlib.util.spec_from_file_location('state_source',Path(__file__).with_name('generate-webgl-state-source.py'))
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
COMMIT,function=module.COMMIT,module.function

def generate(upstream):
    revision=subprocess.check_output(['git','-C',str(upstream),'rev-parse','HEAD'],text=True).strip()
    if revision!=COMMIT:raise RuntimeError('Wrong upstream commit')
    out='// Generated exact math method source; see THREEJS-LICENSE.\nnamespace NativeMathStateSource {\n'
    for name,methods in [('Vector4',['set','copy','fromArray','toArray']),('Color',['set','setRGB']),('Quaternion',['fromArray','toArray','_onChangeCallback','_onChange']),('Euler',['fromArray','toArray','_onChangeCallback','_onChange'])]:
        relative=f'src/math/{name}.js';raw=(upstream/relative).read_bytes()
        blob=subprocess.check_output(['git','-C',str(upstream),'show',f'{COMMIT}:{relative}'])
        if raw.replace(b'\r\n',b'\n')!=blob:raise RuntimeError('Modified upstream source: '+relative)
        text=raw.decode('utf-8')
        out+='// '+relative+' SHA256 '+hashlib.sha256(raw).hexdigest()+'\n'
        for method in methods:
            match=re.search(r'\t'+re.escape(method)+r'\(',text)
            if not match:raise RuntimeError('Missing method')
            source=function(text,match.start()+1)
            out+=f'inline constexpr const char* {name}_{method} = {json.dumps(source,ensure_ascii=True)};\n'
    return out+'}\n'

def main():
    parser=argparse.ArgumentParser();parser.add_argument('upstream',type=Path);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    target=Path(__file__).resolve().parents[1]/'src/core/NativeMathStateSource.inc';result=generate(args.upstream)
    if args.check:
        if target.read_text(encoding='utf-8')!=result:raise RuntimeError('Stale metadata')
    else:target.write_text(result,encoding='utf-8',newline='\n')
    print(hashlib.sha256(result.encode()).hexdigest())

if __name__=='__main__':main()
