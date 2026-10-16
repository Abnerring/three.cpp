import json,re,sys,subprocess
from pathlib import Path
BASE=Path(__file__).resolve().parent
UPSTREAM=Path(sys.argv[1] if len(sys.argv)>1 else '../three.js').resolve()
assert subprocess.check_output(['git','-C',str(UPSTREAM),'rev-parse','HEAD'],text=True).strip()=='11738a9d55e502fb484cefffcea3a66dc44e9fe9'
source=(UPSTREAM/'src/extras/TextureUtils.js').read_text(encoding='utf-8')
constants=(UPSTREAM/'src/constants.js').read_text(encoding='utf-8')
names=[v.strip() for v in re.search(r'import \{(.*?)\}',source).group(1).split(',')]
values={name:int(re.search(r'export const '+re.escape(name)+r' = ([-0-9]+);',constants).group(1)) for name in names}
formats=[name for name in names if not name.endswith('Type')];types=[name for name in names if name.endswith('Type')]
U={'$':'undefined'};S={'$':'symbol'};N={'$':'NaN'}
cases=[]
def case(name,args,mode='bytes'):cases.append({'name':name,'mode':mode,'args':args})
def cv(label,value,**kw):return {'$convert':dict(label=label,value=value,**kw)}
for fmt in formats:
 for typ in types:
  for i,(width,height) in enumerate([(3,7),(0,4),(-1,3),({'$':'-0'},4),(1.5,2.5),('5','9'),(None,True),(U,4),(N,2),({'$':'Infinity'},0),([2],[4]),({},2)]):
   case(f'{fmt}-{typ}-{i}',[width,height,values[fmt],values[typ]])
for typ in types:case('type-'+typ,[values[typ]],'type')
for i,value in enumerate([U,None,False,True,0,-1,1.5,N,S,'1009',{},[],[1009],cv('type',1009),cv('type',S),cv('type',1009,throw=True),cv('type',1009,getThrow=True),{'$ordinary':{'toString':{'label':'type.toString','value':{}},'valueOf':{'label':'type.valueOf','value':1009}}}]):
 case('invalid-type-'+str(i),[cv('width',7),cv('height',3),cv('format',1023),value])
 case('private-type-'+str(i),[value],'type')
for i,value in enumerate([U,None,False,True,0,-1,1.5,N,S,'1023',{},[],[1023],cv('format',1023),cv('format',S),cv('format',1023,throw=True)]):case('invalid-format-'+str(i),[cv('width',7),cv('height',3),value,1009])
for fmt in formats:
 for i,(width,height) in enumerate([(cv('width',7),cv('height',3)),(cv('width','7'),cv('height','3')),(cv('width',7,throw=True),cv('height',3)),(cv('width',7),cv('height',3,throw=True)),(cv('width',7,getThrow=True),cv('height',3)),(cv('width',7),cv('height',3,getThrow=True)),(cv('width',S),cv('height',3)),(cv('width',7),cv('height',S)),(cv('width',{},gc=True),cv('height',3)),(S,cv('height',3)),(cv('width',7),S)]):case('conversion-'+fmt+'-'+str(i),[width,height,values[fmt],1009])
 for i,width in enumerate([{'$ordinary':{'valueOf':{'label':'width.valueOf','value':3},'toString':{'label':'width.toString','value':'7'}}},{'$ordinary':{'valueOf':{'label':'width.valueOf','value':{}},'toString':{'label':'width.toString','value':'7'}}},cv('width',7,methodValue=None),cv('width',7,methodValue=3)]):case('ordinary-'+fmt+'-'+str(i),[width,cv('height',3),values[fmt],1009])
BASE.joinpath('texture-utils-scenarios.jsonl').write_text(''.join(json.dumps(c,ensure_ascii=True,separators=(',',':'))+'\n' for c in cases),encoding='utf-8',newline='\n')
BASE.joinpath('TextureUtilsConstants.json').write_text(json.dumps(values,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'scenarios':len(cases),'formats':len(formats),'types':len(types)}))
