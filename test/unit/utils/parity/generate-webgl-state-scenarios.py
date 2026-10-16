import json
from pathlib import Path
import re

BASE = Path(__file__).resolve().parent
U={'$':'undefined'}
N={'$':'NaN'}
S={'$':'symbol'}
VALUES=[U,None,False,True,0,{'$':'-0'},-1,1,2,3,4,1.5,'0','1','',N,{'$':'Infinity'}]
def ref(name):return {'$ref':name}
def call(name,*args,path='manager'):return ['call',path,name,list(args)]
def buffer(kind,name,*args):return call(name,*args,path=['manager','buffers',kind])
def obj(name,value):return ['object',name,value]
def setv(name,key,value):return ['set',name,key,value]
def watch(name,key,**spec):return ['watch',name,key,spec]
def vec(*values):return {'$vector':list(values)}
def color(*values):return {'$color':list(values)}
CASES=[]
def case(name,*ops,config=None):CASES.append({'name':name,'config':config or {},'ops':list(ops)})
MATERIAL={'side':0,'blending':1,'transparent':False,'blendEquation':100,'blendSrc':204,'blendDst':205,'blendEquationAlpha':None,
    'blendSrcAlpha':None,'blendDstAlpha':None,'blendColor':color(0,0,0),'blendAlpha':0,'premultipliedAlpha':False,
    'depthFunc':3,'depthTest':True,'depthWrite':True,'colorWrite':True,'stencilWrite':False,'stencilWriteMask':255,'stencilFunc':519,
    'stencilRef':1,'stencilFuncMask':255,'stencilFail':7680,'stencilZFail':7680,'stencilZPass':7680,
    'polygonOffset':False,'polygonOffsetFactor':1,'polygonOffsetUnits':2,'alphaToCoverage':False}
case('initial',call('getParameter',999),call('setLineWidth',1),call('activeTexture'))
for i,version in enumerate(['WebGL 1.0','WebGL 2.0','OpenGL ES 1.0','OpenGL ES 2.0','OpenGL 4.5','other WebGL 2.0','WebGL nope','OpenGL ES nope',None,2,'WebGL 0.0','OpenGL ES 9.0']):
    case(f'version-{i}',call('setLineWidth',2),call('setLineWidth',2),call('reset'),config={'version':version})
for i,value in enumerate(VALUES):
    for kind,method in [('color','setMask'),('depth','setMask'),('depth','setTest'),('depth','setFunc'),('depth','setClear'),('stencil','setTest'),('stencil','setMask'),('stencil','setClear')]:
        case(f'buffer-{kind}-{method}-{i}',buffer(kind,method,value),buffer(kind,method,value),buffer(kind,'setLocked',True),buffer(kind,method,not bool(value)),buffer(kind,'reset'),buffer(kind,method,value))
    case(f'reversed-{i}',buffer('depth','setReversed',value),buffer('depth','getReversed'),buffer('depth','setFunc',2),buffer('depth','setClear',.25),buffer('depth','setReversed',False),buffer('depth','getReversed'))
    case(f'clear-color-{i}',buffer('color','setClear',value,2,3,.5,False),buffer('color','setClear',value,2,3,.5,False),buffer('color','setClear',value,2,3,.5,True),buffer('color','reset'),buffer('color','setClear',value,2,3,.5,True))
    case(f'stencil-fields-{i}',buffer('stencil','setFunc',value,1,255),buffer('stencil','setFunc',value,1,255),buffer('stencil','setOp',value,1,2),buffer('stencil','setOp',value,1,2),buffer('stencil','setLocked',True),buffer('stencil','setFunc',value,2,127),buffer('stencil','setOp',value,2,3))
    case(f'manual-flags-{i}',call('enable',value),call('enable',value),call('disable',value),call('disable',value),call('setFlipSided',value),call('setFlipSided',value),call('setCullFace',value),call('setLineWidth',value),call('setLineWidth',value),call('setScissorTest',value),call('setPolygonOffset',value,2,3),call('setPolygonOffset',value,2,3))
for premul in VALUES:
    for blend in [0,1,2,3,4,6,'1',None]:
        args=[blend,100,204,205,None,None,None,color(.1,.2,.3),.4,premul]
        case(f'blending-{len(CASES)}',call('setBlending',*args),call('setBlending',*args),call('setBlending',0),call('setBlending',*args))
for factor in list(range(200,215))+[None,U,'200','constructor','__proto__',S]:
    case(f'custom-factor-{len(CASES)}',call('setBlending',5,100,factor,205,None,None,None,color(.25,.5,.75),.5,False),call('setBlending',5,100,factor,205,101,201,202,color(.25,.5,.75),.5,True),call('setBlending',1))
for equation in [100,101,102,103,104,None,U,0,'100','constructor','__proto__',S]:
    case(f'custom-equation-{len(CASES)}',call('setBlending',5,equation,201,200,0,0,0,color(0,0,0),0,False),call('setBlending',5,equation,201,200,0,0,0,color(0,0,0),0,False))
for alpha in VALUES:
    case(f'custom-color-{len(CASES)}',obj('c',color('0',2,3)),call('setBlending',5,100,201,200,None,None,None,ref('c'),alpha),call('setBlending',5,100,201,200,None,None,None,ref('c'),alpha),setv('c','r',1),call('setBlending',5,100,201,200,None,None,None,ref('c'),alpha))
for field in MATERIAL:
    for v in [None,'1',False,True]:
        if field=='blendColor':continue
        material=dict(MATERIAL);material[field]=v
        case(f'material-{field}-{str(v)}',obj('material',material),call('setMaterial',ref('material'),False),call('setMaterial',ref('material'),False),call('setMaterial',ref('material'),True))
case('material-custom',obj('material',{**MATERIAL,'blending':5,'transparent':True,'stencilWrite':True,'polygonOffset':True,'alphaToCoverage':True}),call('setMaterial',ref('material'),True),call('setMaterial',ref('material'),True),buffer('depth','setReversed',True),call('setMaterial',ref('material'),False),call('reset'),call('setMaterial',ref('material'),False))
for target in [36160,36009,36008,'36160','__proto__','constructor',S]:
    case(f'framebuffer-{len(CASES)}',obj('f',{'tag':'fb'}),call('bindFramebuffer',target,ref('f')),call('bindFramebuffer',target,ref('f')),call('bindFramebuffer',36160,ref('f')),call('bindFramebuffer',36009,ref('f')),call('bindFramebuffer',36008,None),call('reset'),call('bindFramebuffer',target,ref('f')))
for n in [0,1,2,3,4,'2',-1,1.5,None,U]:
    case(f'drawbuffers-{len(CASES)}',obj('f',{'tag':'fb'}),obj('rt',{'textures':{'length':n}}),call('drawBuffers',ref('rt'),ref('f')),call('drawBuffers',ref('rt'),ref('f')),setv(['rt','textures'],'length',2),call('drawBuffers',ref('rt'),ref('f')),call('drawBuffers',None,None),call('drawBuffers',None,None),call('reset'),call('drawBuffers',ref('rt'),ref('f')))
for value in VALUES+[S,'__proto__','constructor']:
    case(f'texture-{len(CASES)}',call('unbindTexture'),call('activeTexture',value),call('bindTexture',3553,None),call('bindTexture',3553,None),obj('t',{'tag':'usertexture'}),call('bindTexture',3553,ref('t')),call('bindTexture',34067,ref('t'),value),call('unbindTexture'),call('unbindTexture'),call('bindTexture',3553,False,value),call('reset'),call('bindTexture',3553,0,value))
for key in [3333,'3333','__proto__','constructor',S,U,None,{'$coerce':{'name':'parameterKey','value':'3333'}}]:
    case(f'parameter-{len(CASES)}',call('getParameter',key),call('pixelStorei',key,4),call('pixelStorei',key,4),call('getParameter',key),call('pixelStorei',key,U),call('getParameter',key),call('reset'),call('getParameter',key))
for vector in [vec(0,0,640,480),vec(1,2,3,4),vec('1',2,3,4),vec(1,2,3,U),vec(N,2,3,4),{},None,U]:
    case(f'vector-{len(CASES)}',obj('v',vector),call('scissor',ref('v')),call('scissor',ref('v')),call('viewport',ref('v')),call('viewport',ref('v')),call('reset'),call('viewport',ref('v')))
for program in [{'tag':'program'},None,U,1,'p']:
    for group in [{'tag':'group','name':'Group'},None,U,1]:
        case(f'ubo-{len(CASES)}',obj('p',program),obj('g',group),obj('b',{'tag':'buffer'}),call('uniformBlockBinding',ref('g'),ref('p'),ref('b')),call('updateUBOMapping',ref('g'),ref('p')),call('updateUBOMapping',ref('g'),ref('p')),call('uniformBlockBinding',ref('g'),ref('p'),ref('b')),call('uniformBlockBinding',ref('g'),ref('p'),ref('b')),call('reset'),call('updateUBOMapping',ref('g'),ref('p')),call('uniformBlockBinding',ref('g'),ref('p'),ref('b')))
for index in [0,1,7,4294967295,U,'7','__proto__','constructor']:
    case(f'ubo-index-{len(CASES)}',obj('p',{'tag':'program'}),obj('g',{'tag':'group','name':'Group'}),obj('b',{'tag':'buffer'}),call('updateUBOMapping',ref('g'),ref('p')),call('uniformBlockBinding',ref('g'),ref('p'),ref('b')),call('uniformBlockBinding',ref('g'),ref('p'),ref('b')),call('reset'),call('updateUBOMapping',ref('g'),ref('p')),call('uniformBlockBinding',ref('g'),ref('p'),ref('b')),config={'blockIndex':index})
textureCommands=['compressedTexImage2D','compressedTexImage3D','texImage2D','texImage3D','texStorage2D','texStorage3D','texSubImage2D','texSubImage3D','compressedTexSubImage2D','compressedTexSubImage3D']
for name in textureCommands:
    case('forward-'+name,call(name,1,None,'x',U,False),['fail',name],call(name,1,None,'x',U,False),['fail',''],call(name),['fail',name],['trigger','console',[call('pixelStorei',3333,8)]],call(name,1,2),['fail','console'],call(name))
commands={
    'enable':call('enable',99),'disable':call('disable',99),'colorMask':buffer('color','setMask',True),'clearColor':buffer('color','setClear',1,2,3,4),
    'depthMask':buffer('depth','setMask',False),'depthFunc':buffer('depth','setFunc',2),'clearDepth':buffer('depth','setClear',.5),
    'stencilMask':buffer('stencil','setMask',127),'stencilFunc':buffer('stencil','setFunc',519,2,127),'stencilOp':buffer('stencil','setOp',1,2,3),'clearStencil':buffer('stencil','setClear',2),
    'frontFace':call('setFlipSided',True),'cullFace':call('setCullFace',2),'blendEquation':call('setBlending',1),'blendFuncSeparate':call('setBlending',1),
    'blendFunc':call('setBlending',2,U,U,U,U,U,U,U,U,True),'blendEquationSeparate':call('setBlending',5,101,201,200,None,None,None,color(0,0,0),0),
    'blendColor':call('setBlending',5,100,201,200,None,None,None,color(.1,.2,.3),.5),'lineWidth':call('setLineWidth',2),'polygonOffset':call('setPolygonOffset',True,2,3),
    'activeTexture':call('activeTexture',33985),'bindTexture':call('bindTexture',3553,False),'bindFramebuffer':call('bindFramebuffer',36160,None),'useProgram':call('useProgram',1),
    'drawBuffers':call('drawBuffers',None,None),'pixelStorei':call('pixelStorei',3333,8),'scissor':call('scissor',vec(1,2,3,4)),'viewport':call('viewport',vec(1,2,3,4)),
    'clipControlEXT':buffer('depth','setReversed',True),'getUniformBlockIndex':call('updateUBOMapping',ref('g'),ref('p')),'uniformBlockBinding':call('updateUBOMapping',ref('g'),ref('p')),
    'bindBufferBase':call('uniformBlockBinding',ref('g'),ref('p'),ref('b'))}
for name,op in commands.items():
    setup=[obj('p',{'tag':'program'}),obj('g',{'tag':'group','name':'Group'}),obj('b',{'tag':'buffer'})]
    if name=='bindBufferBase':setup+=[call('updateUBOMapping',ref('g'),ref('p'))]
    case('failure-'+name,*setup,['fail',name],op,['fail',''],op,op)
    for action in [call('reset'),call('useProgram',2),call('bindTexture',3553,None,33985)]:
        case(f'reentry-{name}-{len(CASES)}',*setup,['trigger',name,[action]],op,op,call('reset'),op)
    case('command-getter-'+name,*setup,watch('gl' if name!='clipControlEXT' else 'extension',name),op,op)
case('material-getters',obj('material',MATERIAL),*[watch('material',key)for key in MATERIAL],*[watch('gl',key)for key in ['CULL_FACE','CW','CCW','DEPTH_TEST','SAMPLE_ALPHA_TO_COVERAGE']],call('setMaterial',ref('material'),False),call('setMaterial',ref('material'),True))
case('vector-getters',obj('v',vec(1,2,3,4)),*[watch('v',field)for field in 'xyzw'],call('scissor',ref('v')),call('scissor',ref('v')),call('viewport',ref('v')))
case('depth-clear-borrow',buffer('depth','setClear',.25),['borrow',['manager','buffers','depth'],'setReversed',[True],None],buffer('depth','getReversed'),buffer('depth','setReversed',False))
case('reset-getters',*[watch('canvas',field)for field in ['width','height']],call('reset'),call('reset'))
case('cache-key-coercion',call('enable',{'$coerce':{'name':'cap','value':99}}),call('disable',{'$coerce':{'name':'cap','value':99}}),call('bindFramebuffer',{'$coerce':{'name':'fbkey','value':36160}},None),call('bindTexture',3553,None,{'$coerce':{'name':'slot','value':33984}}))
case('polygon-coercion',buffer('depth','setReversed',True),call('setPolygonOffset',True,{'$coerce':{'name':'factor','value':2}}, {'$coerce':{'name':'units','value':3}}),call('setPolygonOffset',True,{'$coerce':{'name':'factor','value':2,'throw':True}},3))
case('group-name-getter',obj('p',{'tag':'program'}),obj('g',{'tag':'group','name':'Group'}),watch('g','name'),call('updateUBOMapping',ref('g'),ref('p')),call('updateUBOMapping',ref('g'),ref('p')),call('reset'),call('updateUBOMapping',ref('g'),ref('p')))
methods=['enable','disable','bindFramebuffer','drawBuffers','useProgram','setBlending','setMaterial','setFlipSided','setCullFace','setLineWidth','setPolygonOffset','setScissorTest','activeTexture','bindTexture','unbindTexture','compressedTexImage2D','compressedTexImage3D','texImage2D','texImage3D','pixelStorei','getParameter','updateUBOMapping','uniformBlockBinding','texStorage2D','texStorage3D','texSubImage2D','texSubImage3D','compressedTexSubImage2D','compressedTexSubImage3D','scissor','viewport','reset']
for name in methods:case('metadata-'+name,['metadata','manager',name])
for kind,names in [('color',['setMask','setLocked','setClear','reset']),('depth',['setReversed','getReversed','setTest','setMask','setFunc','setLocked','setClear','reset']),('stencil',['setTest','setMask','setFunc','setOp','setLocked','setClear','reset'])]:
    for name in names:case('metadata-'+kind+'-'+name,['metadata',['manager','buffers',kind],name])
case('metadata-constructor',['metadata','constructorHolder','ctor'])
case('gc-every-command',*sum(([call('bindTexture',3553,None,i+33984),['gc']]for i in range(4)),[]),obj('p',{'tag':'program'}),obj('g',{'name':'Group'}),call('updateUBOMapping',ref('g'),ref('p')),['gc'],call('reset'),['gc'],call('updateUBOMapping',ref('g'),ref('p')))
for text in ['abcabc','','馃榾x馃榾','\ud800X\udc00','a\u0000b',42,None,U]:
    for needle in ['a','',U,None,'馃榾','\ud800','x',2]:
        case(f'string-indexOf-{len(CASES)}',obj('s',{'text':text}),*[call('indexOf',needle,position,path=['s','text'])for position in [U,None,0,-1,1,2,3,{'$':'Infinity'},{'$':'-Infinity'},N,1.5,'2']])
for which in [0,1,2]:
    values=[{'$coerce':{'name':'self','value':'abcabc'}},{'$coerce':{'name':'needle','value':'bc'}},{'$coerce':{'name':'position','value':2}}]
    values[which]['$coerce']['throw']=True
    case(f'indexOf-conversion-throw-{which}',['borrow','stringPrototype','indexOf',[values[1],values[2]],values[0]])
case('indexOf-conversion-order',['borrow','stringPrototype','indexOf',[{'$coerce':{'name':'needle','value':'bc'}},{'$coerce':{'name':'position','value':2}}],{'$coerce':{'name':'self','value':'abcabc'}}])
for value in ['\u00a0 2.5garbage','  -2.1e2 trailing','+Infinity!','0x2','2e+','\ufeff2','.5','invalid',U]:
    case(f'version-exec-override-{len(CASES)}',call('setLineWidth',2),config={'before':[['function','regexpPrototype','exec',{'label':'exec','result':{'1':value}}]]})
for prototype,names in [('vectorPrototype',['set','copy','equals','fromArray']),('colorPrototype',['set','setRGB','copy','equals'])]:
    for name in names:
        before=[['function',prototype,name,{'label':prototype+'.'+name,'forward':True}]]
        case(f'prototype-forward-{prototype}-{name}',buffer('color','setClear',1,2,3,.5),call('scissor',vec(1,2,3,4)),call('setBlending',5,100,201,200,None,None,None,color(.1,.2,.3),.5),call('reset'),config={'before':before})
for result in [False,True,None,U,0,'false']:
    case(f'equals-override-{len(CASES)}',['function','vectorPrototype','equals',{'label':'equals','result':result}],buffer('color','setClear',1,2,3,4),call('scissor',vec(1,2,3,4)),call('viewport',vec(1,2,3,4)))
case('copy-w-changing-getter',obj('v',vec(1,2,3,4)),watch('v','w',action=[setv('v','x',9)]),call('scissor',ref('v')),call('scissor',ref('v')))
for name in ['createTexture','texParameteri','texImage2D','texImage3D','clearColor','clearDepth','clearStencil','depthFunc','frontFace','cullFace']:
    case('constructor-command-failure-'+name,config={'fail':name})
for value in [0,1,2,3,4,5,6,7,U,None,'2']:
    case(f'reversed-mapping-{len(CASES)}',buffer('depth','setReversed',True),setv('reversed','2',value),buffer('depth','setFunc',2),buffer('depth','setFunc',2),['delete','reversed','2'],buffer('depth','setFunc',2),call('reset'),buffer('depth','setReversed',True),buffer('depth','setFunc',2))
case('reversed-mapping-getter',buffer('depth','setReversed',True),watch('reversed','2'),buffer('depth','setFunc',2),buffer('depth','setFunc',2))
for proto,field in [('vectorPrototype','x'),('vectorPrototype','w'),('colorPrototype','r'),('colorPrototype','isColor')]:
    case(f'prototype-setter-{proto}-{field}',buffer('color','setClear',1,2,3,4),call('reset'),config={'before':[watch(proto,field)]})
for proto,names in [('vectorPrototype',['set','copy','fromArray']),('colorPrototype',['set','setRGB'])]:
    for name in names:case(f'metadata-math-{proto}-{name}',['metadata',proto,name])
case('metadata-indexOf',['metadata','stringPrototype','indexOf'])
case('vector-fromArray-symbol-offset',obj('v',vec(0,0,0,0)),obj('a',{}),watch('v','x'),['borrow','vectorPrototype','fromArray',[ref('a'),S],ref('v')])
for value in [U,None,False,True,{},S]:
    case(f'color-set-single-noop-{len(CASES)}',obj('c',color(1,2,3)),['borrow','colorPrototype','set',[value],ref('c')])

# Preserve assignment RHS reads before ToPropertyKey, then offset+1 conversion.
for proto,fields in [('vector2Prototype','xy'),('vector3Prototype','xyz'),('vectorPrototype','xyzw'),('colorPrototype','rgb')]:
    vals={f:i+7 for i,f in enumerate(fields)}
    first=fields[0]
    case('toArray-symbol-write-'+proto,obj('v',vals),obj('a',{}),watch('v',first),['borrow',proto,'toArray',[ref('a'),S],ref('v')],['getKey','a',S])
    case('fromArray-symbol-read-'+proto,obj('v',vals),obj('a',{}),['setKey','a',S,29],watch('v',first),['borrow',proto,'fromArray',[ref('a'),S],ref('v')],['getKey','v',first])
for offset in [{'$coerce':{'name':'offset','value':2}},{'$coerce':{'name':'offset','value':'key'}},{'$coerce':{'name':'offset','value':S}},{'$coerce':{'name':'offset','value':2,'throw':True}}]:
    case('toArray-key-conversion-'+str(len(CASES)),obj('v',vec(7,8,9,10)),obj('a',{}),watch('v','x'),watch('v','y'),['borrow','vectorPrototype','toArray',[ref('a'),offset],ref('v')],['getKey','a',S])
case('toArray-getter-throws-before-key',obj('v',vec(7,8,9,10)),obj('a',{}),watch('v','x',throw=True),['borrow','vectorPrototype','toArray',[ref('a'),{'$coerce':{'name':'offset','value':'key'}}],ref('v')])

for typ in ['Quaternion','Euler']:
    case('native-'+typ+'-toArray-symbol-write',obj('v',{'$native'+typ:True}),obj('a',{}),call('toArray',ref('a'),S,path='v'),['getKey','a',S])
    case('native-'+typ+'-toArray-key-conversion',obj('v',{'$native'+typ:True}),obj('a',{}),call('toArray',ref('a'),{'$coerce':{'name':'offset','value':'key'}},path='v'))
    case('native-'+typ+'-fromArray-callback',obj('v',{'$native'+typ:True}),obj('a',[4,5,6,'ZYX'] if typ=='Euler' else [4,5,6,7]),call('fromArray',ref('a'),path='v'),['getKey','v','_x'])
case('native-Quaternion-fromArray-symbol-read',obj('v',{'$nativeQuaternion':True}),obj('a',{}),['setKey','a',S,29],call('fromArray',ref('a'),S,path='v'),['getKey','v','_x'])
case('metadata-vector-toArray',['metadata','vectorPrototype','toArray'])
for proto,typ in [('quaternionPrototype','Quaternion'),('eulerPrototype','Euler')]:
    val={'_x':7,'_y':8,'_z':9,'_w':10,'_order':'ZYX'}
    case('borrow-'+typ+'-toArray-symbol-write',obj('v',val),obj('a',{}),watch('v','_x'),['borrow',proto,'toArray',[ref('a'),S],ref('v')],['getKey','a',S])
    case('borrow-'+typ+'-toArray-four-components',obj('v',val),obj('a',{}),['borrow',proto,'toArray',[ref('a'),2],ref('v')])
    case('borrow-'+typ+'-fromArray-default-callback',['borrow',proto,'_onChangeCallback',[],{}])
    for callback in [{'label':'onChange'},{'label':'onChange','throw':True},{'label':'onChange','action':[['set','v','_x',99]]}]:
        case('borrow-'+typ+'-fromArray-callback-'+str(len(CASES)),obj('v',val),obj('a',[3,4,5,'XYZ'] if typ=='Euler' else [3,4,5,6]),['function','v','_onChangeCallback',callback],watch('v','_x'),['borrow',proto,'fromArray',[ref('a'),0 if typ=='Quaternion' else S],ref('v')])
    case('borrow-'+typ+'-fromArray-missing-callback',obj('v',val),obj('a',[3,4,5,'XYZ'] if typ=='Euler' else [3,4,5,6]),watch('v','_x'),['borrow',proto,'fromArray',[ref('a')],ref('v')])
    case('native-'+typ+'-fromArray-overridden-callback',obj('v',{'$native'+typ:True}),obj('a',[3,4,5,'XYZ'] if typ=='Euler' else [3,4,5,6]),['function','v','_onChangeCallback',{'label':'custom'}],call('fromArray',ref('a'),path='v'))
    for method in ['fromArray','toArray','_onChangeCallback']:
        case('metadata-'+typ+'-'+method,['metadata',proto,method])
for proto in ['vector2Prototype','vector3Prototype','vectorPrototype','colorPrototype','quaternionPrototype','eulerPrototype']:
    vals={'x':1,'y':2,'z':3,'w':4,'r':1,'g':2,'b':3,'_x':1,'_y':2,'_z':3,'_w':4,'_order':'XYZ'}
    first='r' if proto=='colorPrototype' else '_x' if proto in ['quaternionPrototype','eulerPrototype'] else 'x'
    case('toArray-null-target-'+proto,obj('v',vals),watch('v',first),['borrow',proto,'toArray',[None,{'$coerce':{'name':'offset','value':'key'}}],ref('v')])
for typ,proto in [('Quaternion','quaternionPrototype'),('Euler','eulerPrototype')]:
    values=[1,2,3,'XYZ'] if typ=='Euler' else [1,2,3,4]
    case('callback-prototype-empty-'+typ,obj('v',{'$native'+typ:True}),['borrow',proto,'_onChangeCallback',[],ref('v')])
    for installed in [False,True]:
        case('callback-own-descriptor-'+typ+str(installed),obj('v',{'$native'+typ:installed}),['descriptor','v','_onChangeCallback'],call('_onChangeCallback',path='v'),call('fromArray',values,path='v'))
    case('callback-own-borrow-'+typ,obj('v',{'$native'+typ:True}),['borrow','v','_onChangeCallback',[],{}])
    case('callback-own-delete-'+typ,obj('v',{'$native'+typ:True}),['delete','v','_onChangeCallback'],['descriptor','v','_onChangeCallback'],call('fromArray',values,path='v'))
    case('callback-own-undefined-'+typ,obj('v',{'$native'+typ:True}),['set','v','_onChangeCallback',U],call('fromArray',values,path='v'))
    case('callback-own-prototype-descriptor-'+typ,['descriptor',proto,'_onChangeCallback'])
case('object3d-owned-callbacks',obj('o',{'$nativeObject3D':True}),['define',['o','quaternion'],'tag','objectQuaternion'],['descriptor',['o','quaternion'],'_onChangeCallback'],['descriptor',['o','rotation'],'_onChangeCallback'],call('fromArray',[0,0,0,1],path=['o','quaternion']),['getKey',['o','rotation'],'_x'])
def register(label,**spec):return ['nativeRegister','v',label,spec]
def native_set(value):return ['nativeSet','v',value]
def capture(name='saved'):return ['save',name,['v','_onChangeCallback']]
def invoke(name='saved'):return call('call',U,path=name)
for typ,proto in [('Quaternion','quaternionPrototype'),('Euler','eulerPrototype')]:
    start=obj('v',{'$native'+typ:False});values=[1,2,3,'XYZ'] if typ=='Euler' else [1,2,3,4]
    case('interop-late-register-'+typ,start,register('first'),['descriptor','v','_onChangeCallback'],native_set(2),capture(),invoke())
    case('interop-old-snapshot-'+typ,start,register('old'),capture(),register('new'),invoke(),native_set(3),invoke(),native_set(4))
    case('interop-managed-fromArray-'+typ,start,register('new'),call('fromArray',values,path='v'))
    case('interop-managed-onChange-'+typ,start,obj('cb',{}),['function','cb','invoke',{'label':'JS'}],['save','callback',['cb','invoke']],call('_onChange',ref('callback'),path='v'),native_set(3),register('native'),native_set(4))
    case('interop-own-delete-'+typ,start,register('old'),['delete','v','_onChangeCallback'],native_set(1),['descriptor','v','_onChangeCallback'],register('new'),native_set(2))
    case('interop-own-undefined-'+typ,start,register('old'),setv('v','_onChangeCallback',U),native_set(7),['getKey','v','_x'])
    case('interop-own-readonly-'+typ,start,register('old'),['callbackFlags','v',{'writable':False}],register('new'),native_set(3),['descriptor','v','_onChangeCallback'])
    case('interop-prototype-'+typ,start,['function',proto,'_onChangeCallback',{'label':'prototype'}],native_set(3),register('native'),native_set(4))
    case('interop-own-getter-'+typ,start,register('old'),watch('v','_onChangeCallback'),native_set(1),register('new'),native_set(2))
    case('interop-reentrant-register-'+typ,start,register('old',action=[register('new'),native_set(2)]),capture(),native_set(1),invoke())
    case('interop-reentrant-delete-'+typ,start,register('old',action=[['delete','v','_onChangeCallback'],native_set(2)]),native_set(1),native_set(3))
    case('interop-native-throws-'+typ,start,register('throw',throw=True),native_set(5),['getKey','v','_x'],native_set(6))
    case('interop-gc-in-callback-'+typ,start,register('gc',action=[['gc']]),native_set(1),native_set(2))
    case('interop-getter-throws-'+typ,start,register('old'),watch('v','_onChangeCallback',throw=True),native_set(1),['getKey','v','_x'])
    case('interop-js-forward-old-'+typ,start,register('old'),['function','v','_onChangeCallback',{'label':'forward','forward':True,'action':[register('new')]}],native_set(1),native_set(2))
    case('interop-old-prototype-'+typ,start,capture(),register('new'),invoke(),native_set(2))
    case('metadata-'+typ+'-onChange',['metadata',proto,'_onChange'])

for typ in ['Quaternion','Euler']:
    start=obj('v',{'$native'+typ:False})
    case('interop-register-throwing-setter-'+typ,start,register('old'),watch('v','_onChangeCallback',setThrow=True),register('new'),native_set(3))
BASE.joinpath('webgl-state-scenarios.jsonl').write_text(''.join(json.dumps(value,ensure_ascii=True,separators=(',',':'))+'\n'for value in CASES),encoding='utf-8',newline='\n')
cpp=BASE.joinpath('WebGLStateFixture.h').read_text(encoding='utf-8')
constants={name:int(value)for name,value in re.findall(r'\{"([A-Z_0-9]+)",([0-9]+)\}',cpp)}
BASE.joinpath('WebGLStateConstants.json').write_text(json.dumps(constants,indent=2)+'\n',encoding='utf-8',newline='\n')
print(len(CASES))
