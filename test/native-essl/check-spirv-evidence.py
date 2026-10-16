import argparse, json, math, pathlib, struct
parser=argparse.ArgumentParser();parser.add_argument('output',type=pathlib.Path);args=parser.parse_args()
checks=0
def verify(condition,message):
    global checks
    checks+=1
    if not condition: raise AssertionError(message)
def string(words): return struct.pack('<'+'I'*len(words),*words).split(b'\0',1)[0].decode('utf-8')
def module(path):
    data=path.read_bytes();verify(len(data)%4==0 and len(data)>=20,str(path)+' framing')
    words=struct.unpack('<'+'I'*(len(data)//4),data);verify(words[0]==0x07230203,str(path)+' magic')
    names={};membernames={};types={};constants={};decorations={};memberdecorations={};variables={};sources=[];aliases={};loads=[]
    i=5
    while i<len(words):
        count=words[i]>>16;opcode=words[i]&65535;verify(count>0 and i+count<=len(words),str(path)+' instruction framing')
        operands=words[i+1:i+count];i+=count
        if opcode==5:names[operands[0]]=string(operands[1:])
        elif opcode==6:membernames[operands[:2]]=string(operands[2:])
        elif opcode==3:sources.append(operands[:2])
        elif opcode in range(19,40):types[operands[0]]=(opcode,operands[1:])
        elif opcode==43:constants[operands[1]]=(operands[0],operands[2:])
        elif opcode==59:variables[operands[1]]=(operands[0],operands[2])
        elif opcode==71:decorations.setdefault(operands[0],{})[operands[1]]=operands[2:]
        elif opcode==72:memberdecorations.setdefault(operands[:2],{})[operands[2]]=operands[3:]
        elif opcode in (65,66,67,70):aliases[operands[1]]=operands[2]
        elif opcode==83:aliases[operands[1]]=operands[2]
        elif opcode==61:loads.append(operands[2])
    verify((1,300) in sources,str(path)+' original ESSL300 retained')
    def signature(typeid):
        opcode,operands=types[typeid]
        if opcode in (23,24,27,32):return (opcode,*(operands[:-1] if opcode==32 else ()),signature(operands[-1] if opcode==32 else operands[0]),*(operands[1:] if opcode!=32 else ()))
        if opcode==29:return (opcode,signature(operands[0]),decorations.get(typeid,{}).get(6))
        if opcode==28:return (opcode,signature(operands[0]),constants[operands[1]][1],decorations.get(typeid,{}).get(6))
        if opcode==30:return (opcode,tuple((signature(t),tuple(sorted((d,v) for d,v in memberdecorations.get((typeid,j),{}).items() if d in (4,5,7,35)))) for j,t in enumerate(operands)))
        if opcode==25:return (opcode,signature(operands[0]),*operands[1:])
        return (opcode,*operands)
    def locations(typeid):
        opcode,operands=types[typeid]
        if opcode==24:return operands[1]
        if opcode==28:return locations(operands[0])*constants[operands[1]][1][0]
        if opcode==30:return sum(locations(t) for t in operands)
        return 1
    used=set()
    for pointer in loads:
        seen=set()
        while pointer in aliases and pointer not in seen:seen.add(pointer);pointer=aliases[pointer]
        used.add(pointer)
    blocks={};samplers={};inputs={};activeinputs={};outputs={};occupied={1:set(),3:set()}
    floats=[]
    for typeid,value in constants.values():
        if types[typeid]==(22,(32,)):floats.append(struct.unpack('<f',struct.pack('<I',value[0]))[0])
    for variable,(pointer,storage) in variables.items():
        _,pointerdata=types[pointer];typeid=pointerdata[1];d=decorations.get(variable,{})
        if storage==2 and 2 in decorations.get(typeid,{}):
            fields=[]
            for index,fieldtype in enumerate(types[typeid][1]):
                md=memberdecorations.get((typeid,index),{})
                verify(35 in md,str(path)+' block member offset')
                fields.append({'name':membernames[(typeid,index)],'type':signature(fieldtype),'offset':md[35][0],'matrixStride':md.get(7),'columnMajor':5 in md})
            verify(d.get(33)==(0,) and d.get(34)==(0,),str(path)+' uniform block binding/set')
            blocks[names[variable]]=fields
        elif storage==0 and 33 in d:
            verify(34 in d,str(path)+' sampler set');samplers[names[variable]]=(d[34],d[33],signature(typeid))
        elif storage in (1,3) and 30 in d:
            location=d[30][0];slotcount=locations(typeid)
            slots=set(range(location,location+slotcount));verify(not (slots & occupied[storage]),str(path)+' duplicate IO location')
            occupied[storage]|=slots
            (inputs if storage==1 else outputs)[names[variable]]=(location,signature(typeid))
            if storage==1 and variable in used:activeinputs[names[variable]]=(location,signature(typeid))
    return {'blocks':blocks,'samplers':samplers,'inputs':inputs,'activeInputs':activeinputs,'outputs':outputs,'floatConstants':floats}

results=json.loads((args.output/'RESULTS.json').read_text(encoding='utf-8'))['results'];audited=[]
for result in results:
    name=result['name']
    if not result['ok']:
        verify(name=='RawShaderMaterial-default','Unexpected rejected shader '+name)
        verify(result['phase']=='parse-original-vertex' and "undeclared identifier" in result['error'],'Expected raw-default undeclared source error')
        continue
    verify(result['originalVersions']==[300,300],name+' input versions')
    verify(all(m['validated'] for m in result['modules']),name+' Vulkan validator')
    vs=module(args.output/(name+'.vert.spv'));fs=module(args.output/(name+'.frag.spv'))
    for block in vs['blocks'].keys() & fs['blocks'].keys():verify(vs['blocks'][block]==fs['blocks'][block],name+' identical shared std140 block schema')
    for sampler in vs['samplers'].keys() & fs['samplers'].keys():verify(vs['samplers'][sampler]==fs['samplers'][sampler],name+' shared sampler schema')
    for inputname,inputdata in fs['activeInputs'].items():verify(vs['outputs'].get(inputname)==inputdata,name+' active cross-stage varying '+inputname)
    if name in ('shader-version-environment-uniform-varying','raw-version-environment-uniform-varying'):
        target=vs if name.startswith('shader-') else fs
        verify(any(math.isclose(v,0.3,abs_tol=1e-7) for v in target['floatConstants']),name+' ESSL300 selected 0.3')
        verify(not any(math.isclose(v,0.9,abs_tol=1e-7) for v in target['floatConstants']),name+' 450 branch 0.9 absent')
        verify(0.5 in fs['floatConstants'],name+' GL_ES branch selected 0.5')
    audited.append({'name':name,'vertex':vs,'fragment':fs})
verify(len(audited)==25,'Expected exactly 25 emitted real-source pairs')
(args.output/'SPIRV-EVIDENCE.json').write_text(json.dumps({'checks':checks,'pairs':len(audited),'audit':audited},indent=2),encoding='utf-8')
print(f'{len(audited)} pairs / {checks} framing, reflection, linkage and original-branch checks passed')
