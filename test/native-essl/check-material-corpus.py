import hashlib,pathlib,subprocess,sys
probe,captures,output=map(pathlib.Path,sys.argv[1:])
assert hashlib.sha256((captures/'MANIFEST.json').read_bytes()).hexdigest()=='f90af9ef58dbf7fe60c1a810a54b88d7acb19e75637b8cb7e69a28230e940b71','Pinned Programs capture manifest changed'
output.mkdir(parents=True,exist_ok=True)
for name,args in [('compile',[sys.executable,pathlib.Path(__file__).with_name('run-captured-portable.py'),probe,captures,output]),('binary-audit',[sys.executable,pathlib.Path(__file__).with_name('check-spirv-evidence.py'),output])]:
    result=subprocess.run([str(x) for x in args],text=True,capture_output=True,timeout=120)
    (output/(name+'.stdout.log')).write_text(result.stdout,encoding='utf-8');(output/(name+'.stderr.log')).write_text(result.stderr,encoding='utf-8')
    if result.returncode:raise RuntimeError(name+' failed: '+result.stdout+result.stderr)
    print(result.stdout,end='')
