import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=async file=>import(pathToFileURL(path.join(upstream,'src',file)));
const {WebGLGeometries}=await load('renderers/webgl/WebGLGeometries.js'),{WebGLObjects}=await load('renderers/webgl/WebGLObjects.js'),{EventDispatcher}=await load('core/EventDispatcher.js');
const values=[undefined,null,false,true,0,-0,NaN,1,2,10,'0','2','10','x',Infinity,-Infinity];
const encode=v=>v===undefined?'undefined':typeof v==='function'?'function':typeof v==='number'?Number.isNaN(v)?'NaN':v===Infinity?'Infinity':v===-Infinity?'-Infinity':Object.is(v,-0)?'-0':v:Array.isArray(v)?v.map(encode):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const label=v=>v&&(['object','function'].includes(typeof v))?typeof v.label==='string'?v.label:'wire':String(v);
const data=(o,k,v)=>Object.defineProperty(o,k,Object.assign(Object.create(null),{value:v,writable:true,enumerable:true,configurable:true}));
function watch(target,field,trace,caption){let value=target[field];Object.defineProperty(target,field,{get(){trace.push(['get',caption,this===target]);return value;},set(v){trace.push(['set',caption,this===target,v&&typeof v==='object'?label(v):v]);value=v;},enumerable:true,configurable:true});}
class Fixture{
  constructor(r){
    this.trace=[];this.gl={ARRAY_BUFFER:34962};this.info={memory:{geometries:0},render:{frame:0}};this.binding={};this.attributes={};this.behavior=0;
    const f=this;
    for(const name of ['update','remove'])this.attributes[name]=function(...args){f.trace.push(name==='update'?[name,label(args[0]),args[1],this===f.attributes]:[name,label(args[0]),this===f.attributes]);if(name==='update'&&f.behavior===1&&label(args[0])==='matrix')f.objects.dispose();if(name==='remove'&&f.behavior===2)throw new TypeError('remove');if(name==='remove'&&f.behavior===3)f.g.attributes={position:'changed',extra:'changed-extra'};};
    for(const name of ['releaseStatesOfGeometry','releaseStatesOfObject'])this.binding[name]=function(value){f.trace.push([name,label(value),this===f.binding]);};
    this.g=new EventDispatcher();Object.assign(this.g,{label:'geometry',id:r.id===undefined?7:r.id,index:null});
    this.position={label:'position',array:new Float32Array(r.length===undefined?9:r.length),count:r.count===undefined?(r.length===undefined?9:r.length)/3:r.count,version:0};
    this.g.attributes={};if(!r.missing)this.g.attributes.position=this.position;
    this.index={};if(r.indices!==undefined){this.index={label:'index',array:new Uint32Array(r.indices),version:0};this.g.index=this.index;}
    this.geometries=WebGLGeometries(this.gl,this.attributes,this.info,this.binding);this.objects=WebGLObjects(this.gl,this.geometries,this.attributes,this.binding,this.info);
    this.instance=new EventDispatcher();Object.assign(this.instance,{geometry:this.g,isInstancedMesh:true,instanceMatrix:{label:'matrix'},instanceColor:{label:'color'}});
  }
  geo(name,args=[]){return this.geometries[name].apply({},args);}
  obj(name,args=[]){return this.objects[name].apply(null,args);}
  dispose(target){target.dispatchEvent({type:'dispose'});}
  memory(){return Number(this.info.memory.geometries);}
}
function wireState(v){return v===undefined?'undefined':{type:v.array.constructor.name,array:Array.from(v.array,encode),version:encode(v.version),count:encode(v.count),itemSize:encode(v.itemSize),usage:encode(v.usage),gpuType:encode(v.gpuType),normalized:encode(v.normalized),ranges:encode(v.updateRanges)};}
function wire(r){const f=new Fixture(r);if(r.indices!==undefined)f.index.version=values[r.first];else f.position.version=values[r.first];
  if(r.watch){for(const k of ['index','attributes'])watch(f.g,k,f.trace,'geometry.'+k);for(const k of ['array','count','version'])watch(f.position,k,f.trace,'position.'+k);if(r.indices!==undefined)for(const k of ['array','version'])watch(f.index,k,f.trace,'index.'+k);}
  const a=f.geo('getWireframeAttribute',[f.g]),before=wireState(a);if(a!==undefined)data(a,'label','wire0');if(r.indices!==undefined)f.index.version=values[r.next];else f.position.version=values[r.next];const b=f.geo('getWireframeAttribute',[f.g]);return {first:before,second:wireState(b),same:a===b,trace:encode(f.trace)};
}
function geometryCase(r){const f=new Fixture(r),s=r.scenario;let restore,error='none';
  if(s===2)Object.assign(f.g,{isInstancedBufferGeometry:values[r.flag],_maxInstanceCount:8});
  if(s===3){const p={inherited:f.position,hidden:{}},map=Object.create(p);Object.defineProperty(map,'hidden',{value:f.index,writable:true,enumerable:false,configurable:true});map.own=f.position;f.g.attributes=map;}
  if(s===4){f.g.attributes.extra=f.position;f.behavior=3;}
  if(s===5){const d=Object.getOwnPropertyDescriptor(FinalizationRegistry.prototype,'register');Object.defineProperty(FinalizationRegistry.prototype,'register',{value:function(){f.trace.push(['register-throw']);throw new TypeError('register');},writable:true,enumerable:false,configurable:true});restore=()=>Object.defineProperty(FinalizationRegistry.prototype,'register',d);}
  if(s===6){const d=Object.getOwnPropertyDescriptor(WeakRef.prototype,'deref'),old=d.value;Object.defineProperty(WeakRef.prototype,'deref',{get(){f.trace.push(['deref-get',typeof this==='object']);return old;},configurable:true});restore=()=>Object.defineProperty(WeakRef.prototype,'deref',d);}
  if(r.watch){for(const k of ['id','index','attributes','isInstancedBufferGeometry'])watch(f.g,k,f.trace,'geometry.'+k);for(const k of ['remove','update'])watch(f.attributes,k,f.trace,'attributes.'+k);for(const k of ['addEventListener','removeEventListener'])watch(f.g,k,f.trace,'geometry.'+k);watch(f.info,'memory',f.trace,'info.memory');watch(f.info.memory,'geometries',f.trace,'memory.geometries');watch(f.gl,'ARRAY_BUFFER',f.trace,'gl.ARRAY_BUFFER');}
  try{f.geo('get',[undefined,f.g]);f.geo('get',[undefined,f.g]);if(s===1)f.g.id=8;f.geo('update',[f.g]);if(s!==3&&s!==4)f.geo('getWireframeAttribute',[f.g]);if(s===7)f.geo('dispose');else{f.dispose(f.g);if(s===1)f.geo('dispose');}}catch(e){error=e instanceof TypeError?'TypeError':e instanceof RangeError?'RangeError':'Error';}finally{restore?.();}
  const memory=f.memory();return {error,memory,max:encode(f.g._maxInstanceCount),trace:encode(f.trace)};
}
function objectCase(r){const f=new Fixture(r),s=r.scenario;f.info.render.frame=values[r.frame];let restore;
  const skeleton={label:'skeleton',update(){f.trace.push(['skeleton.update',this===skeleton]);if(s===8)throw new TypeError('skeleton');}};Object.assign(f.instance,{isSkinnedMesh:true,skeleton});
  if(s===1)f.behavior=1;if(s===2)f.instance.instanceColor=null;if(s===3)f.instance.hasEventListener=function(){return values[r.flag];};if(s===4)f.instance.isInstancedMesh=false;
  if(s===5){f.geometries.get=function(o){return o;};f.geometries.update=function(){f.trace.push(['geometry.update-object']);};}
  if(s===6||s===7){const n=s===6?'get':'set',p=WeakMap.prototype,d=Object.getOwnPropertyDescriptor(p,n),original=d.value;Object.defineProperty(p,n,{get(){f.trace.push(['WeakMap.'+n,'getter']);if(s===7)f.objects.dispose();return function(...args){f.trace.push(['WeakMap.'+n,label(args[0]),typeof this==='object']);return original.apply(this,args);};},configurable:true});restore=()=>Object.defineProperty(p,n,d);}
  if(r.watch){for(const k of ['geometry','isInstancedMesh','hasEventListener','addEventListener','instanceMatrix','instanceColor','isSkinnedMesh','skeleton'])watch(f.instance,k,f.trace,'object.'+k);for(const k of ['get','update'])watch(f.geometries,k,f.trace,'geometries.'+k);watch(f.info,'render',f.trace,'info.render');watch(f.info.render,'frame',f.trace,'render.frame');watch(f.gl,'ARRAY_BUFFER',f.trace,'gl.ARRAY_BUFFER');}
  const errors=[];try{for(let i=0;i<3;i++){try{f.obj('update',[f.instance]);errors.push('none');}catch(e){errors.push(e instanceof TypeError?'TypeError':'Error');}}try{if(s===9){f.dispose(f.instance);f.obj('update',[f.instance]);}else if(s===10){f.obj('dispose');f.obj('update',[f.instance]);}}catch(e){if(e instanceof TypeError)errors.push('TypeError');else throw e;}}finally{restore?.();}
  return {errors,memory:f.memory(),trace:encode(f.trace)};
}
function metadata(fn){return {name:fn.name,length:fn.length,own:Object.getOwnPropertyNames(fn),flags:['length','name','prototype'].map(k=>{const d=Object.getOwnPropertyDescriptor(fn,k);return [!!d.writable,!!d.enumerable,!!d.configurable];}),constructible:(()=>{try{Reflect.construct(String,[],fn);return true;}catch{return false;}})(),prototypeConstructor:fn.prototype.constructor===fn};}
function constructCase(r){const f=new Fixture(r),api=r.group==='geometry'?f.geometries:f.objects,fn=api[r.method],custom={marker:8},meta=metadata(fn);const s=r.scenario;function Target(){f.trace.push(['targetBody']);}let newTarget=fn;
  if(s===1){Target.prototype=custom;newTarget=Target;}if(s===2){Target.prototype=3;newTarget=Target;}if(s===3)fn.prototype=custom;if(s===4)newTarget=()=>{};
  let args=r.method==='get'?[undefined,f.g]:r.method==='dispose'?[]:[r.group==='geometry'?f.g:f.instance];if(s===5)args=[];let result;
  try{const v=s===6?fn.apply(null,args):Reflect.construct(fn,args,newTarget);result={object:(typeof v==='object'&&v!==null)||typeof v==='function',geometry:v===f.g,own:v&&(['object','function'].includes(typeof v))?Object.getOwnPropertyNames(v):encode(v),fnPrototype:v&&typeof v==='object'?Object.getPrototypeOf(v)===fn.prototype:false,targetPrototype:v&&typeof v==='object'?Object.getPrototypeOf(v)===Target.prototype:false,customPrototype:v&&typeof v==='object'?Object.getPrototypeOf(v)===custom:false};}catch(e){result=e instanceof TypeError?'TypeError':'Error';}
  return {metadata:meta,result,memory:f.memory(),trace:encode(f.trace)};
}
function listenerCase(r){const f=new Fixture(r),s=r.scenario,object=r.group==='geometry'?f.g:f.instance;if(r.group==='geometry')f.geo('get',[undefined,f.g]);else f.obj('update',[f.instance]);const fn=object._listeners.dispose[0],meta=metadata(fn),custom={};function Target(){}Target.prototype=custom;const event={target:s===3?{}:object},args=s===4?[]:[event];let result;
  try{const value=s===2?fn.apply(null,args):Reflect.construct(fn,args,s===1?Target:fn);result={object:value!==null&&typeof value==='object',own:value!==null&&typeof value==='object'?Object.getOwnPropertyNames(value):encode(value),fnPrototype:value!==null&&typeof value==='object'?Object.getPrototypeOf(value)===fn.prototype:false,customPrototype:value!==null&&typeof value==='object'?Object.getPrototypeOf(value)===custom:false};}catch(e){result=e instanceof TypeError?'TypeError':'Error';}
  return {metadata:meta,result,memory:f.memory(),trace:encode(f.trace)};
}
function pollutionCase(r){const f=new Fixture(r),s=r.scenario,p=Object.prototype,old=[];const fields=s<2?['get','update','getWireframeAttribute','dispose']:s===2?['updateMap','geometries','registry','wireframeAttributes','onGeometryDispose','onInstancedMeshDispose']:['7'];
  for(const name of fields){old.push([name,Object.getOwnPropertyDescriptor(p,name)]);const d=Object.assign(Object.create(null),{configurable:true});if(s===1||s===3)Object.assign(d,{value:17,writable:false});else d.set=function(){f.trace.push(['polluted-set',name]);};Object.defineProperty(p,name,d);}
  let result;try{f.geometries=WebGLGeometries(f.gl,f.attributes,f.info,f.binding);f.objects=WebGLObjects(f.gl,f.geometries,f.attributes,f.binding,f.info);f.obj('update',[f.instance]);result={names:Object.keys(f.geometries),memory:f.memory(),trace:encode(f.trace)};}catch(e){if(e instanceof TypeError)result='TypeError';else throw e;}finally{for(const[name,d]of old)if(d)Object.defineProperty(p,name,Object.assign(Object.create(null),d));else delete p[name];}return result;
}
async function gcCase(r){const f=new Fixture(r);f.geo('get',[undefined,f.g]);f.geo('getWireframeAttribute',[f.g]);const weak=new WeakRef(f.g),id=f.g.id;let finalized=false;const witness=new FinalizationRegistry(()=>{finalized=true;});witness.register(f.g,0);if(r.scenario===1)f.dispose(f.g);f.g=null;f.position=null;f.index=null;f.instance=null;
  for(let i=0;i<50&&!finalized;i++){await new Promise(resolve=>setImmediate(resolve));global.gc();await new Promise(resolve=>setImmediate(resolve));}
  assert.ok(finalized,'Host GC did not deliver the witness callback within 50 independent turns');await new Promise(resolve=>setImmediate(resolve));const expired=weak.deref()===undefined;
  const replacement=new EventDispatcher();Object.assign(replacement,{label:'replacement',id,index:null,attributes:{}});f.geo('get',[undefined,replacement]);f.geo('get',[undefined,replacement]);return {expired,memory:f.memory(),trace:encode(f.trace)};
}
const cases=[];
for(const indices of [undefined,[0,1,2],[3,4,5,1,2]])for(const count of [3,65534,65535])for(const [first,next] of [[0,0],[4,7],[7,8],[11,12],[12,11],[6,7],[2,3],[3,4]])cases.push({mode:'wire',indices,count,first,next,watch:false});
for(let first=0;first<16;first++)for(let next=0;next<16;next++)cases.push({mode:'wire',indices:[0,1,2],first,next});
for(const length of [0,1,2,3,4,6,8,9,10,12,15])for(const count of [0,65534,65535])cases.push({mode:'wire',length,count,first:4,next:7,watch:true});
cases.push({mode:'wire',missing:true,first:4,next:7});
for(let scenario=0;scenario<8;scenario++)for(const watch of [false,true])cases.push({mode:'geometry',scenario,watch,indices:[0,1,2]});
for(const id of ['constructor','__proto__','toString','hasOwnProperty',null,'7'])cases.push({mode:'geometry',scenario:0,id,indices:[0,1,2],watch:true});
for(let flag=0;flag<16;flag++)cases.push({mode:'geometry',scenario:2,flag,indices:[0,1,2]});
for(let scenario=0;scenario<11;scenario++)for(const frame of [0,2,4,5,6,7,10,13])for(const watch of [false,true])cases.push({mode:'objects',scenario,frame,flag:2,watch});
for(let flag=0;flag<16;flag++)cases.push({mode:'objects',scenario:3,frame:4,flag});
for(const [group,methods] of [['geometry',['get','update','getWireframeAttribute','dispose']],['objects',['update','dispose']]])for(const method of methods)for(let scenario=0;scenario<7;scenario++)cases.push({mode:'construct',group,method,scenario});
for(const group of ['geometry','objects'])for(let scenario=0;scenario<5;scenario++)cases.push({mode:'listener',group,scenario});
for(let scenario=0;scenario<4;scenario++)cases.push({mode:'pollution',scenario});
if(typeof global.gc!=='function')throw Error('Run Node with --expose-gc to verify real geometry finalization');
for(let scenario=0;scenario<2;scenario++)cases.push({mode:'gc',scenario});
const expected=[];for(const r of cases){try{expected.push(r.mode==='wire'?wire(r):r.mode==='geometry'?geometryCase(r):r.mode==='objects'?objectCase(r):r.mode==='listener'?listenerCase(r):r.mode==='pollution'?pollutionCase(r):r.mode==='gc'?await gcCase(r):constructCase(r));}catch(e){expected.push(e instanceof TypeError?'TypeError':e instanceof RangeError?'RangeError':{unexpected:e.message});}}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:64*1024*1024}));
assert.equal(actual.length,expected.length);let comparisons=0;const failures=[];
function compare(a,b,at){if(b&&typeof b==='object'){assert.equal(Array.isArray(a),Array.isArray(b),at);assert.deepEqual(Object.keys(a),Object.keys(b),at);for(const k of Object.keys(b))compare(a[k],b[k],at+'.'+k);}else{assert.deepEqual(a,b,at);comparisons++;}}
for(let i=0;i<cases.length;i++)try{compare(actual[i],expected[i],i+' '+JSON.stringify(cases[i]));}catch(e){failures.push(e.message);}
fs.writeFileSync(path.join(path.dirname(binary),'geometry-objects-expected.json'),JSON.stringify(expected));fs.writeFileSync(path.join(path.dirname(binary),'geometry-objects-actual.json'),JSON.stringify(actual));
assert.equal(failures.length,0,failures.slice(0,20).join('\n'));console.log(`WebGL geometry/objects parity: ${cases.length} scenarios / ${comparisons} comparisons against ${lock.commit}`);
