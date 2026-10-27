import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]);
const binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):undefined;
const directory=process.argv[4]&&path.resolve(process.argv[4]);
const pin=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),pin.commit);
const names=['ArrayElementNode','ConvertNode','JoinNode','SplitNode','SetNode','FlipNode','MemberNode'];
const constructors=Object.fromEntries(await Promise.all(names.map(async name=>[name,(await import(pathToFileURL(path.join(upstream,'src/nodes/utils/'+name+'.js')))).default])));
const {setConsoleFunction}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
function normal(v,seen=[]){
    if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(typeof v==='symbol')return '#symbol';
    if(typeof v==='number'){if(Number.isNaN(v))return '#NaN';if(!Number.isFinite(v))return v>0?'#Infinity':'#-Infinity';if(Object.is(v,-0))return '#-0';}
    if(v===null||typeof v!=='object')return v;if(v.fixtureTag)return '@'+v.fixtureTag;if(seen.includes(v))return '#cycle';seen=[...seen,v];
    const out=Array.isArray(v)?new Array(v.length):{};for(const key of Object.keys(v))Object.defineProperty(out,key,{value:normal(v[key],seen),writable:true,enumerable:true,configurable:true});return out;
}
const decode=v=>v&&typeof v==='object'&&v.tag==='undefined'?undefined:v;
function descriptor(object,key){const d=Object.getOwnPropertyDescriptor(object,key),value=d.value;return [key,Object.hasOwn(d,'value')?'data':'accessor',normal(value),!!d.writable,!!d.enumerable,!!d.configurable,typeof value==='function'?value.name:normal(d.get),typeof value==='function'?value.length:normal(d.set)];}
function metadata(r){const C=constructors[r.class],args=(r.args||[]).map(decode),n=new C(...args);n.id=100;n._uuid='fixture-uuid';const other=new C(...args);return {own:Object.getOwnPropertyNames(n).map(key=>[key,normal(n[key])]),constructor:Object.getOwnPropertyNames(C).map(key=>descriptor(C,key)),prototype:Object.getOwnPropertyNames(C.prototype).map(key=>descriptor(C.prototype,key)),parent:Object.getPrototypeOf(C.prototype).constructor.name,staticParent:Object.getPrototypeOf(C).name,freshNodes:r.class==='JoinNode'?n.nodes!==other.nodes:false};}
function constructProtocol(r){const events=[],values={},result={},C=constructors[r.class],original=Object.getPrototypeOf(C);for(const key of ['node','indexNode','isArrayElementNode','convertTo','nodes','components','isSplitNode','sourceNode','targetNode','structNode','property','isMemberNode'])Object.defineProperty(result,key,{enumerable:true,configurable:true,set(value){events.push(['set',key,normal(value),this===result]);if(r.throwAt===key)throw new TypeError('setter');Object.defineProperty(values,key,{value,writable:true,enumerable:true,configurable:true});}});Object.setPrototypeOf(C,function Base(...args){events.push(['base',new.target===C,...args.map(v=>normal(v))]);return result;});let output;try{output=new C(...r.args.map(decode))===result;}catch(e){output='#'+e.name;}finally{Object.setPrototypeOf(C,original);}return {result:output,events,values:normal(values)};}
function observe(s,object,key,label){const value=object[key];Object.defineProperty(object,key,{configurable:true,enumerable:true,get(){s.events.push(['get',label,this===object]);if(s.request.throwAt===label)throw new TypeError(label);return value;}});}
function fn(s,object,key,label,body){object[key]=function(...args){return body.call(this,args)};observe(s,object,key,label);}
function fixtureNode(s,tag,type,snippet){const node={fixtureTag:tag,typeValue:type,snippet};for(const key of ['getNodeType','getElementType','getMemberType','getScope','build','hasMember'])fn(s,node,key,tag+'.'+key,function(args){s.events.push(['call',tag+'.'+key,this===node,...args.map(v=>normal(v))]);if(s.request.throwAt===tag+'.'+key+'()')throw new TypeError('call');if(key==='build')return this.snippet;if(key==='getScope')return s.scope;if(key==='hasMember')return s.request.hasMember;if(key==='getMemberType')return s.request.memberType;return this.typeValue;});return node;}
function width(type){const t=String(type);if(t.length>3&&(t.slice(0,3)==='vec'||t.slice(1,4)==='vec'))return Number(t.at(-1));if(t==='void')return 0;return 1;}
function protocol(r){
    const s={request:r,events:[],scope:{fixtureTag:'scope'},cache:{fixtureTag:'cache'}};
    const builder=s.builder={fixtureTag:'builder'};
    for(const key of ['getTypeLength','getTypeFromLength','getVectorType','getComponentType','format','getType','isVector','isInteger','getVarFromNode','getPropertyName','addLineFlowCode','getStructTypeNode','generateConst'])fn(s,builder,key,'builder.'+key,function(a){
        s.events.push(['call','builder.'+key,this===builder,...a.map(v=>normal(v))]);if(r.throwAt==='builder.'+key+'()')throw new TypeError('builder call');
        if(key==='getTypeLength')return width(a[0]);
        if(key==='getTypeFromLength'){const n=Number(a[0]),c=a[1]===undefined?'float':a[1];if(n===1)return c;return (c==='int'?'i':c==='uint'?'u':c==='bool'?'b':'')+'vec'+n;}
        if(key==='getVectorType'||key==='getType')return a[0];if(key==='getComponentType'){const t=String(a[0]);return t.startsWith('ivec')||t==='int'?'int':t.startsWith('uvec')||t==='uint'?'uint':t.startsWith('bvec')||t==='bool'?'bool':'float';}
        if(key==='format')return 'format('+a[0]+','+a[1]+','+a[2]+')';if(key==='isVector')return width(a[0])>1;if(key==='isInteger')return a[0]==='int'||a[0]==='uint';
        if(key==='getVarFromNode')return s.cache;if(key==='getPropertyName')return 'tmp';if(key==='getStructTypeNode')return s.source;if(key==='generateConst')return 'zero('+a[0]+')';
    });
    const source=s.source=fixtureNode(s,'source',Object.hasOwn(r,'sourceType')?decode(r.sourceType):'vec3','SOURCE');source.isMemberNode=r.nested;
    const index=fixtureNode(s,'index',Object.hasOwn(r,'indexType')?decode(r.indexType):'int','INDEX'),target=fixtureNode(s,'target',Object.hasOwn(r,'targetType')?decode(r.targetType):'float','TARGET');
    const components=Object.hasOwn(r,'components')?decode(r.components):'xy',inputs=[source,index,target];let args;
    if(r.class==='ArrayElementNode')args=[source,index];if(r.class==='ConvertNode')args=[source,Object.hasOwn(r,'convertTo')?decode(r.convertTo):'float|vec3|ivec3'];
    if(r.class==='JoinNode')args=[inputs,Object.hasOwn(r,'nodeType')?decode(r.nodeType):null];if(r.class==='SplitNode')args=[source,components];if(r.class==='SetNode')args=[source,components,target];if(r.class==='FlipNode')args=[source,components];if(r.class==='MemberNode')args=[source,Object.hasOwn(r,'property')?decode(r.property):'member'];
    const n=s.main=new constructors[r.class](...args);n.fixtureTag='main';n.id=100;n._uuid='fixture-uuid';n.getNodeType=function(...a){s.events.push(['call','main.getNodeType',this===n,...a.map(v=>normal(v))]);if(r.throwAt==='main.getNodeType()')throw new TypeError('type');return Object.hasOwn(r,'outputType')?decode(r.outputType):'vec3';};
    for(const key of Object.keys(n))if(!['fixtureTag','id','_uuid'].includes(key))observe(s,n,key,'main.'+key);
    if(r.iterable){let position=0;const iterator={fixtureTag:'iterator'};fn(s,iterator,'next','iterator.next',function(){s.events.push(['call','iterator.next',this===iterator]);if(r.throwAt==='iterator.next()')throw new TypeError('next');const i=position++;return {done:i>=3,value:inputs[i]};});fn(s,iterator,'return','iterator.return',function(){s.events.push(['call','iterator.return',this===iterator]);if(r.closeThrow)throw new TypeError('return');return {};});Object.defineProperty(n,'nodes',{value:{fixtureTag:'iterable',[Symbol.iterator](){s.events.push(['iterator']);return iterator;}},writable:true,enumerable:true,configurable:true});}
    setConsoleFunction((...a)=>{s.events.push(['console',...a.map(v=>normal(v))]);if(r.throwAt==='console')throw new TypeError('console');});
    const a=[];if(!['getScope','getVectorLength','isCacheable'].includes(r.method))a.push(builder);if(r.method==='generate')a.push(Object.hasOwn(r,'output')?decode(r.output):'result');if(r.method==='getMemberType')a.push('child');
    let result;try{result=normal(n[r.method](...a));}catch(e){result='#'+e.name;}setConsoleFunction(null);return {result,events:s.events};
}
const cases=[];
for(const name of names)for(const args of [[],[{tag:'undefined'}],[null],[1,'xy',false],[{},null,{}]])cases.push({kind:'metadata',class:name,args});
const constructorFields={ArrayElementNode:['node','indexNode','isArrayElementNode'],ConvertNode:['node','convertTo'],JoinNode:['nodes'],SplitNode:['node','components','isSplitNode'],SetNode:['sourceNode','components','targetNode'],FlipNode:['sourceNode','components'],MemberNode:['structNode','property','isMemberNode']};
for(const name of names)for(const args of [[],[{tag:'undefined'},{tag:'undefined'}],[null,null,null],[1,'xy',false]])for(const throwAt of [null,...constructorFields[name]])cases.push({kind:'construct',class:name,args,throwAt});
const methods={ArrayElementNode:['isCacheable','getScope','generateNodeType','getMemberType','generate'],ConvertNode:['generateNodeType','generate'],JoinNode:['generateNodeType','generate'],SplitNode:['isCacheable','getScope','getVectorLength','getComponentType','generateNodeType','generate'],SetNode:['generateNodeType','generate'],FlipNode:['generateNodeType','generate'],MemberNode:['isCacheable','getScope','hasMember','generateNodeType','getMemberType','generate']};
for(const name of names)for(const method of methods[name])cases.push({kind:'protocol',class:name,method,memberType:'vec2',hasMember:true});
for(const indexType of ['int','uint','float','ivec2','vec3','bool','void'])cases.push({kind:'protocol',class:'ArrayElementNode',method:'generate',indexType});
for(const sourceType of ['float','vec2','vec3','vec4','ivec3','void'])for(const convertTo of ['float|vec2|ivec3','vec4|float','float','',null])cases.push({kind:'protocol',class:'ConvertNode',method:'generateNodeType',sourceType,convertTo});
for(const sourceType of ['float','vec2','vec3','vec4','ivec3'])for(const components of ['x','xy','xyz','xyzw','w','yx','xz','',null])cases.push({kind:'protocol',class:'SplitNode',method:'generate',sourceType,components});
for(const name of ['SetNode','FlipNode'])for(const outputType of ['float','vec2','vec3','vec4','ivec3'])for(const components of ['x','xy','xyz','w','yx','xz','',null])cases.push({kind:'protocol',class:name,method:'generate',outputType,components});
for(const nodeType of [null,'float','vec2','vec3','ivec3','vec4'])for(const sourceType of ['float','vec2','ivec4'])cases.push({kind:'protocol',class:'JoinNode',method:'generate',nodeType,outputType:nodeType||'vec3',sourceType});
for(const nested of [false,true])for(const hasMember of [false,true,0,null])for(const memberType of ['void','vec2',null])for(const method of ['hasMember','generateNodeType','getMemberType','generate'])cases.push({kind:'protocol',class:'MemberNode',method,nested,hasMember,memberType});
for(const name of names){const method='generate',base={kind:'protocol',class:name,method,memberType:'vec2',hasMember:true};const events=protocol(base).events;for(const label of new Set(events.flatMap(e=>e[0]==='get'?[e[1]]:e[0]==='call'?[e[1]+'()']:e[0]==='console'?['console']:[])))cases.push({...base,throwAt:label});}
for(const sourceType of ['float','vec4'])for(const throwAt of [null,'source.build()','source.getNodeType()','iterator.next()','console'])for(const closeThrow of [false,true])cases.push({kind:'protocol',class:'JoinNode',method:'generate',iterable:true,sourceType,outputType:'vec2',throwAt,closeThrow});
const expected=cases.map(r=>r.kind==='metadata'?metadata(r):r.kind==='construct'?constructProtocol(r):protocol(r));
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.jsonl'),cases.map(r=>JSON.stringify(r)).join('\n')+'\n');fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(JSON.stringify({oracle:cases.length}));process.exit(0);}
const raw=execFileSync(binary,[],{input:cases.map(r=>JSON.stringify(r)).join('\n')+'\n',encoding:'utf8',maxBuffer:128*1024*1024});const actual=raw.trimEnd().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,expected.length);
const failures=[];for(let i=0;i<cases.length;i++)try{assert.deepEqual(actual[i],expected[i]);}catch{failures.push({index:i,case:cases[i],expected:expected[i],actual:actual[i]});}
if(directory){fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(failures,null,2));}
assert.equal(failures.length,0,JSON.stringify(failures.slice(0,3),null,2));console.log(`Utility nodes: ${cases.length} direct pinned-upstream cases passed`);
