import pathlib,subprocess,sys
compiler,vulkan,opengl,sources,output,validator=map(pathlib.Path,sys.argv[1:])
output.mkdir(parents=True,exist_ok=True)
def run(name,args):
    result=subprocess.run([str(x) for x in args],text=True,capture_output=True,timeout=120)
    (output/(name+'.stdout.log')).write_text(result.stdout,encoding='utf-8')
    (output/(name+'.stderr.log')).write_text(result.stderr,encoding='utf-8')
    if result.returncode:raise RuntimeError(name+' failed: '+result.stdout+result.stderr)
run('compile-and-mapping',[compiler,sources,output/'modules',validator])
run('actual-vulkan',[vulkan,output/'modules',output/'vulkan'])
run('original-ESSL-native-GL',[opengl,sources,output/'opengl'])
run('pixel-compare',[sys.executable,pathlib.Path(__file__).with_name('compare-pixels.py'),output/'vulkan',output/'opengl',output/'comparison.json'])
print((output/'pixel-compare.stdout.log').read_text(),end='')
