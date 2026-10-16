import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]),input=path.resolve(process.argv[4]);
assert.equal(spawnSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).stdout.trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const constants=await import(pathToFileURL(path.join(upstream,'src/constants.js')));
const {getByteLength,contain,cover,fill,TextureUtils}=await import(pathToFileURL(path.join(upstream,'src/extras/TextureUtils.js')));
const source=fs.readFileSync(path.join(upstream,'src/extras/TextureUtils.js'),'utf8');
// The private helper is executed verbatim from the pinned module, with the actual imported constants.
const start=source.indexOf('function getTextureTypeByteLength('),end=source.indexOf('\n}',start)+2;
const getType=new Function(...Object.keys(constants),source.slice(start,end)+';return getTextureTypeByteLength;')(...Object.values(constants));
function encode(value){if(value===undefined)return 'undefined';if(typeof value==='number'){if(Number.isNaN(value))return 'NaN';if(!Number.isFinite(value))return value<0?'-Infinity':'Infinity';if(Object.is(value,-0))return '-0';}if(typeof value==='symbol')return 'symbol';if(typeof value==='function')return 'function';if(Array.isArray(value))return value.map(encode);if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).map(k=>[k,encode(value[k])]));return value;}
function decode(value,trace){
 if(value===null||typeof value!=='object')return value;
 if(Object.hasOwn(value,'$'))return {undefined:undefined,NaN,Infinity,'-Infinity':-Infinity,'-0':-0,symbol:Symbol('offset')}[value.$];
 function methodGetter(spec){return function(){trace.push([spec.label,'get']);if(spec.getThrow)throw RangeError('conversion getter');if(Object.hasOwn(spec,'methodValue'))return decode(spec.methodValue,trace);return function(hint){trace.push([spec.label,'call',encode(hint)]);if(spec.gc)global.gc?.();if(spec.throw)throw RangeError('conversion');return decode(spec.value,trace);};};}
 if(Object.hasOwn(value,'$convert'))return Object.defineProperty({},Symbol.toPrimitive,{get:methodGetter(value.$convert),enumerable:true,configurable:true});
 if(Object.hasOwn(value,'$ordinary')){const out={};for(const [key,spec]of Object.entries(value.$ordinary))Object.defineProperty(out,key,{get:methodGetter(spec),enumerable:true,configurable:true});return out;}
 return Array.isArray(value)?value.map(v=>decode(v,trace)):Object.fromEntries(Object.entries(value).map(([key,v])=>[key,decode(v,trace)]));
}
const functions={contain,cover,fill,getByteLength};
function step(state,event){
 state.trace.push([event]);const count=++state.steps;
 if(state.spec.gcAt===count)global.gc?.();
 if(state.spec.swapAt===count){state.storage.texture.repeat=state.objects.newRepeat;state.storage.texture.offset=state.objects.newOffset;}
 if(state.spec.throwAt===count)throw RangeError('fixture step');
 if(state.spec.reenterAt===count&&!state.reentered){state.reentered=true;functions[state.spec.op](state.objects.texture,state.spec.reenterAspect);}
}
function watched(state,name,initial){
 if(initial===null||typeof initial!=='object')return initial;
 const target={},backing={};state.storage[name]=backing;state.objects[name]=target;
 for(const key of Object.keys(initial)){
  backing[key]=initial[key];
  const d={configurable:true,enumerable:true,get(){step(state,'get '+name+'.'+key);return state.storage[name][key];}};
  if(state.spec.readonly!==name+'.'+key)d.set=function(value){step(state,'set '+name+'.'+key);state.storage[name][key]=value;};
  Object.defineProperty(target,key,d);
 }
 return target;
}
function scenario(spec){
 const state={trace:[],steps:0,spec,storage:{},objects:{}};
 const initial=decode(spec.texture,state.trace);let texture=initial;
 if(spec.watch&&initial!==null&&typeof initial==='object'){
  for(const name of ['image','repeat','offset'])if(Object.hasOwn(initial,name))initial[name]=watched(state,name,initial[name]);
  const spare={x:17,y:19};watched(state,'newRepeat',spare);watched(state,'newOffset',spare);
  texture=watched(state,'texture',initial);
 }
 let aspect=decode(spec.aspect,state.trace);
 if(Object.hasOwn(spec,'aspectValues')){
  aspect={};state.aspectIndex=0;
  Object.defineProperty(aspect,Symbol.toPrimitive,{enumerable:true,configurable:true,get(){
   step(state,'get aspect conversion');return function(hint){step(state,'convert aspect '+hint);const values=spec.aspectValues;return decode(values[Math.min(state.aspectIndex++,values.length-1)],state.trace);};
  }});
 }
 let result;
 try{
  let value;
  if(spec.entry==='class')value=TextureUtils[spec.op].call(null,texture,aspect);
  else if(spec.entry==='construct')value=new functions[spec.op](texture,aspect);
  else value=functions[spec.op].call(null,texture,aspect);
  result={same:value===texture};
 }catch(e){result=e instanceof TypeError?{error:'TypeError'}:e instanceof RangeError?{error:'RangeError'}:{error:e.name,message:e.message};}
 const values={};
 if(spec.watch&&initial!==null&&typeof initial==='object'){
  for(const name of ['repeat','offset','newRepeat','newOffset'])if(Object.hasOwn(state.storage,name))values[name]=encode(state.storage[name]);
 }else if(texture!==null&&typeof texture==='object')for(const name of ['repeat','offset'])values[name]=encode(texture[name]);
 return {name:spec.name,result,values,trace:state.trace.map(encode)};
}
function functionInfo(fn){
 let constructible=true;try{Reflect.construct(function(){},[],fn);}catch(e){constructible=false;}
 const own=Object.getOwnPropertyNames(fn);
 return {name:fn.name,length:fn.length,source:fn.toString(),own,descriptors:own.map(name=>{const d=Object.getOwnPropertyDescriptor(fn,name);return [!!d.writable,!!d.enumerable,!!d.configurable];}),constructible};
}
function metadata(){
 const out={};for(const name of Object.keys(functions)){out[name]=functionInfo(functions[name]);out['static_'+name]=functionInfo(TextureUtils[name]);}
 out.TextureUtils=functionInfo(TextureUtils);
 const instance=new TextureUtils(),prototype=TextureUtils.prototype,d=Object.getOwnPropertyDescriptor(prototype,'constructor');
 let callable=true;try{TextureUtils();}catch(e){callable=false;}
 function Other(){}const result=Reflect.construct(TextureUtils,[],Other);
 return {functions:out,instanceOwn:Object.getOwnPropertyNames(instance),instancePrototype:Object.getPrototypeOf(instance)===prototype,prototypeOwn:Object.getOwnPropertyNames(prototype),constructorDescriptor:[d.writable,d.enumerable,d.configurable],prototypeConstructor:prototype.constructor===TextureUtils,callable,newTarget:Object.getPrototypeOf(result)===Other.prototype,separate:contain!==TextureUtils.contain,shared:TextureUtils===TextureUtils&&contain===contain};
}
const cases=fs.readFileSync(input,'utf8').trim().split(/\r?\n/).map(JSON.parse);
const native=spawnSync(probe,[input],{encoding:'utf8',maxBuffer:128*1024*1024,timeout:900000});
const output=process.argv[5]??probe+'.actual.jsonl';
fs.writeFileSync(output,native.stdout);fs.writeFileSync(output+'.stderr.log',native.stderr);assert.equal(native.status,0,native.stderr);
const actual=native.stdout.trim().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,cases.length);
const expected=cases.map(spec=>spec.mode==='metadata'?metadata():scenario(spec));fs.writeFileSync(output+'.expected.jsonl',expected.map(v=>JSON.stringify(v)).join('\n')+'\n');
let comparisons=0;function compare(a,b,location){comparisons++;if(a!==null&&typeof a==='object'&&b!==null&&typeof b==='object'){assert.equal(Array.isArray(a),Array.isArray(b),location);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),location);for(const key of Object.keys(b))compare(a[key],b[key],location+'.'+key);}else assert.deepEqual(a,b,location);}
for(let i=0;i<cases.length;i++)compare(actual[i],expected[i],cases[i].name??'metadata');console.log(JSON.stringify({scenarios:cases.length,comparisons}));
