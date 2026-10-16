import argparse,hashlib,json,math,pathlib,struct
p=argparse.ArgumentParser();p.add_argument('vulkan',type=pathlib.Path);p.add_argument('gl',type=pathlib.Path);p.add_argument('output',type=pathlib.Path);a=p.parse_args();cases=[];checks=0;maximum=0
files=sorted(a.vulkan.glob('*.rgba32f'));assert len(files)==13 and {p.name for p in files}=={p.name for p in a.gl.glob('*.rgba32f')}
for p in files:
    vb=p.read_bytes();gb=(a.gl/p.name).read_bytes();assert len(vb)==len(gb) and len(vb)%4==0
    vs=struct.unpack('<'+str(len(vb)//4)+'f',vb);gs=struct.unpack('<'+str(len(gb)//4)+'f',gb)
    difference=max(abs(v-g) for v,g in zip(vs,gs));assert all(math.isfinite(x) for x in vs+gs);assert difference<=0.0003,(p.name,difference)
    checks+=len(vs);maximum=max(maximum,difference);cases.append({'name':p.stem,'channels':len(vs),'maximumAbsDifference':difference,'vulkanSHA256':hashlib.sha256(vb).hexdigest(),'glSHA256':hashlib.sha256(gb).hexdigest()})
assert checks==17280
a.output.write_text(json.dumps({'cases':13,'pixelChannelComparisons':checks,'maximumAbsDifference':maximum,'nativeOpenGLOriginalESSL':True,'browserWebGL':False,'comparison':cases},indent=2),encoding='utf-8')
print('13 actual original-ESSL GL / Vulkan cases / 17280 pixel channels passed; maximum abs difference',maximum)
