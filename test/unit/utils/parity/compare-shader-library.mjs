import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import {spawnSync,execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const root=new URL('../../../../',import.meta.url);
const metadata=JSON.parse(fs.readFileSync(new URL('utils/shader-sources.json',root)));
const actualFiles=['ShaderChunk','ShaderLib'].flatMap(group=>fs.readdirSync(path.join(upstream,'src/renderers/shaders',group)).filter(name=>name.endsWith('.glsl.js')).map(name=>'src/renderers/shaders/'+group+'/'+name)).sort();
assert.deepEqual(metadata.modules.map(m=>m.file).sort(),actualFiles,'complete upstream source inventory');
const expected=[];
for(const m of metadata.modules){const imported=await import(pathToFileURL(path.join(upstream,m.file)));assert.deepEqual(m.exports.map(e=>e.original).sort(),Object.keys(imported).sort(),'all exports '+m.file);expected.push({kind:'source',file:m.file,exports:Object.fromEntries(m.exports.map(e=>[e.original,imported[e.original]]))});}
const {ShaderChunk}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/ShaderChunk.js')));
const {UniformsLib}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/UniformsLib.js')));
const {ShaderLib}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/ShaderLib.js')));
function graph(){const seen=[];return function encode(v){
 if(v===undefined)return '$undefined';if(typeof v==='function')return '$function';if(v===null||typeof v!=='object')return v;
 const old=seen.indexOf(v);if(old!==-1)return {ref:old};const id=seen.length;seen.push(v);
 for(const type of ['Color','Vector2','Vector3','Vector4','Matrix3','Matrix4','Quaternion'])if(v['is'+type]){
  const values=type==='Color'?[v.r,v.g,v.b]:type.startsWith('Matrix')?v.elements.slice():type==='Vector2'?[v.x,v.y]:type==='Vector3'?[v.x,v.y,v.z]:[v.x,v.y,v.z,v.w];return {id,type,values};
 }
 return {id,type:Array.isArray(v)?'Array':'Object',fields:Object.getOwnPropertyNames(v).map(key=>{
  const d=Object.getOwnPropertyDescriptor(v,key);return {key,enumerable:d.enumerable,configurable:d.configurable,...('value'in d?{writable:d.writable,value:encode(d.value)}:{get:typeof d.get==='function',set:typeof d.set==='function'})};
 })};
};}
expected.push({kind:'chunk',value:graph()(ShaderChunk)});
const encode=graph();expected.push({kind:'library',uniforms:encode(UniformsLib),lib:encode(ShaderLib)});
const run=spawnSync(probe,[],{encoding:'utf8',maxBuffer:128*1024*1024});
if(run.status!==0)throw Error(`probe failed ${run.status}: ${run.stderr}`);
const actual=run.stdout.trim().split(/\r?\n/).map(JSON.parse);
let comparisons=0;
function compare(a,e,where=''){comparisons++;if(e&&typeof e==='object'){
 assert.equal(typeof a,'object',where);assert.ok(a!==null,where);assert.deepEqual(Object.keys(a).sort(),Object.keys(e).sort(),where+' keys');for(const k of Object.keys(e))compare(a[k],e[k],where+'.'+k);
}else assert.deepEqual(a,e,where);}
assert.equal(actual.length,expected.length);
for(let i=0;i<expected.length;++i)try{compare(actual[i],expected[i],`case[${i}]`);}catch(error){const diagnostic=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-shader-library-'));fs.writeFileSync(path.join(diagnostic,'actual.json'),JSON.stringify(actual[i],null,2));fs.writeFileSync(path.join(diagnostic,'expected.json'),JSON.stringify(expected[i],null,2));throw Error(`${error.message}\nDiagnostic: ${diagnostic}`,{cause:error});}
console.log(`ShaderLibrary parity: ${expected.length} scenarios / ${comparisons} comparisons passed against ${lock.commit}`);
function runCase(args){const run=spawnSync(probe,args.map(String),{encoding:'utf8',maxBuffer:128*1024*1024});if(run.status!==0)throw Error(`probe ${args} failed ${run.status}: ${run.stderr}`);return JSON.parse(run.stdout);}
function check(a,e,label){try{compare(a,e,label);}catch(error){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-shader-case-'));fs.writeFileSync(path.join(dir,'actual.json'),JSON.stringify(a,null,2));fs.writeFileSync(path.join(dir,'expected.json'),JSON.stringify(e,null,2));throw Error(`${error.message}\nDiagnostic ${dir}`);}}
const {cloneUniforms,mergeUniforms}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/UniformsUtils.js')));
function assignmentCase(mode){
 const trace=[],first={},next={},s={current:first};const event=v=>trace.push(v);
 Object.defineProperty(Object.prototype,'u',{configurable:true,get(){event('get-u');if(mode===3)throw RangeError('lhs');return s.current;},set(v){event('set-u');s.current=v;s.first=v;}});
 const callback=()=>{event('rhs');s.current=next;return 73;};
 const value=mode===1?[{isColor:true,clone:callback}]:mode===2?Object.assign([1,2],{slice:callback}):{isColor:true,clone:callback};
 let error='';try{cloneUniforms({u:{value}});}catch(e){error=e.name;}finally{delete Object.prototype.u;}
 const encode=graph();return {kind:'assignment',mode,error,trace,first:encode(s.first),next:encode(next)};
}
for(let mode=0;mode<4;++mode)check(runCase(['--assignment',mode]),assignmentCase(mode),'assignment'+mode);
let serial=0;
async function initCase(mode,point){
 const trace=[],event=v=>{trace.push(v);if(trace.length===point)throw RangeError('audit');};
 const common=UniformsLib.common;const saved=[];const replace=(o,k,d)=>{saved.push([o,k,Object.getOwnPropertyDescriptor(o,k)]);Object.defineProperty(o,k,{configurable:true,enumerable:true,...d});};
 for(const k of Object.keys(UniformsLib)){const original=UniformsLib[k];replace(UniformsLib,k,{get(){event('group:'+k);return original;}});}
 for(const k of Object.keys(ShaderChunk)){const original=ShaderChunk[k];replace(ShaderChunk,k,{get(){event('chunk:'+k);return original;}});}
 const cloneable={get isColor(){event('flag');return true;},get isRenderTargetTexture(){event('target');return false;},get clone(){event('get-clone');return function(){event('clone');if(mode===2){const nested=mergeUniforms([{x:{value:3}}]);event(nested.x.value);}if(mode===3)common.late={value:42};return {token:17};};}};
 replace(common,'audit',{value:{value:cloneable},writable:true});
 if(mode===1||mode===4)for(const k of ['basic','uniforms','vertexShader','fragmentShader','physical'])replace(Object.prototype,k,{set(v){event('set:'+k);Object.defineProperty(this,k,{value:v,writable:true,enumerable:true,configurable:true});}});
 if(mode===1)for(const k of ['basic','uniforms','vertexShader','fragmentShader','physical'])Object.defineProperty(Object.prototype,k,{enumerable:false});
 let result,error='';try{({ShaderLib:result}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/ShaderLib.js'))+'?audit='+serial++));}catch(e){error=e.name;}
 finally{for(const [o,k,d]of saved.reverse())if(d)Object.defineProperty(o,k,d);else delete o[k];delete common.late;}
 return {kind:'init',mode,point,error,trace,result:graph()(result)};
}
let extras=4;
for(let mode=0;mode<5;++mode){const e=await initCase(mode,-1);check(runCase(['--init',mode,-1]),e,'init'+mode);extras++;
 const points=mode===0?Array.from({length:e.trace.length},(_,i)=>i+1):[1,2,5,10,20,Math.floor(e.trace.length/2),e.trace.length];
 for(const point of points){check(runCase(['--init',mode,point]),await initCase(mode,point),`init ${mode} throw ${point}`);extras++;}
}
console.log(`Shader initialization/clone parity: ${extras} scenarios, cumulative ${comparisons} comparisons passed`);
function enumerationCase(level,mode){
 const def=x=>level===0?{value:x}:x;const prototype={b:def(20),inherited:def(30)},target=Object.assign(Object.create(prototype),{a:def(1),b:def(2),c:def(3)}),replacement={b:def(80),replacement:def(90)},trace=[];let n=0;
 if(mode===9)Object.defineProperty(target,'b',{enumerable:false});
 Object.defineProperty(target,'a',{enumerable:true,configurable:true,get(){trace.push('a');if(n++===0){
 if(mode===1)delete target.c;if(mode===2)delete target.b;if(mode===3){delete target.b;delete prototype.b;}
 if(mode===4)Object.defineProperty(target,'c',{enumerable:false});if(mode===5)target.late=def(40);if(mode===6)prototype.late=def(50);
 if(mode===7)Object.setPrototypeOf(target,replacement);if(mode===8)delete prototype.inherited;if(mode===9)delete target.b;
 if(mode===10){delete target.c;target.c=def(60);}if(mode===11)Object.setPrototypeOf(target,null);if(mode===12)throw RangeError('getter');
 if(mode===13)Object.defineProperty(prototype,'inherited',{enumerable:false});}return def(1);}});
 const source=level===0?target:{u:target};let result,error='';try{result=cloneUniforms(source);}catch(e){error=e.name;}
 return {kind:'enumeration',level,mode,trace,error,result:graph()(result)};
}
for(let level=0;level<2;level++)for(let mode=0;mode<14;mode++)check(runCase(['--enum',level,mode]),enumerationCase(level,mode),`enumeration ${level} ${mode}`);
console.log(`Uniform inheritance parity: 28 scenarios, cumulative ${comparisons} comparisons passed`);
