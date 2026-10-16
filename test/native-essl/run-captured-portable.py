import argparse, hashlib, json, pathlib, subprocess
parser=argparse.ArgumentParser()
parser.add_argument('executable',type=pathlib.Path)
parser.add_argument('sources',type=pathlib.Path)
parser.add_argument('output',type=pathlib.Path)
args=parser.parse_args()
args.output.mkdir(parents=True,exist_ok=True)
capture=json.loads((args.sources/'MANIFEST.json').read_text(encoding='utf-8'))
assert capture['commit']=='11738a9d55e502fb484cefffcea3a66dc44e9fe9'
results=[]
for item in capture['cases']:
    name=item['name']
    for source in item['sources']:
        data=(args.sources/source['file']).read_bytes()
        assert hashlib.sha256(data).hexdigest()==source['sha256']
    run=subprocess.run([str(args.executable),str(args.sources/(name+'.vert')),str(args.sources/(name+'.frag')),str(args.output/name),'--gl-builtins'],text=True,capture_output=True,timeout=30)
    (args.output/(name+'.stdout')).write_text(run.stdout,encoding='utf-8')
    (args.output/(name+'.stderr')).write_text(run.stderr,encoding='utf-8')
    try: result=json.loads(run.stdout.strip())
    except Exception: result={'ok':False,'phase':'native-process','error':run.stderr,'exit':run.returncode}
    result['name']=name;result['exit']=run.returncode
    results.append(result)
    print(name, 'PASS' if result.get('ok') and all(m['validated'] for m in result['modules']) else 'FAIL',result.get('phase',''),flush=True)
manifest={'captureCommit':capture['commit'],'sourceManifestSHA256':hashlib.sha256((args.sources/'MANIFEST.json').read_bytes()).hexdigest(),'results':results}
(args.output/'RESULTS.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
parsed=sum(r.get('ok',False) for r in results)
valid=sum(r.get('ok',False) and all(m['validated'] for m in r['modules']) for r in results)
print(f'parsed/mapped={parsed}/{len(results)} validated={valid}/{len(results)}')
rejected=[r for r in results if not (r.get('ok',False) and all(m['validated'] for m in r.get('modules',[])))]
assert len(results)==26 and valid==25 and len(rejected)==1 and rejected[0]['name']=='RawShaderMaterial-default' and rejected[0]['phase']=='parse-original-vertex'
print('25 valid pairs; exact sole invalid Raw default retained')
