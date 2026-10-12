import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-object-group.mjs <upstream> <binary>');
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {AnimationObjectGroup:G,PropertyBinding:P}=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const encode=v=>v===undefined?'$undefined':typeof v==='function'?'$function':typeof v==='number'?(Number.isNaN(v)?'$NaN':v===Infinity?'$Infinity':v===-Infinity?'$-Infinity':Object.is(v,-0)?'$-0':v):Array.isArray(v)?Array.from({length:v.length},(_,i)=>Object.hasOwn(v,i)?encode(v[i]):'$hole'):v!==null&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const root=uuid=>({uuid,name:uuid,value:1,other:2,third:3,children:[]});
const bindingArray=a=>a===undefined?'$undefined':Array.from({length:a.length},(_,i)=>!Object.hasOwn(a,i)?'$hole':a[i]===undefined?'$undefined':({path:encode(a[i].path),root:encode(a[i].rootNode.uuid),parsed:encode(a[i].parsedPath)}));
const state=g=>({objects:g._objects.map(o=>encode(o.uuid)),cached:encode(g.nCachedObjects_),indices:encode(g._indicesByUUID),paths:encode(g._paths),parsed:encode(g._parsedPaths),bindings:Array.from({length:g._bindings.length},(_,i)=>Object.hasOwn(g._bindings,i)?bindingArray(g._bindings[i]):'$hole'),pathIndices:encode(g._bindingsIndicesByPath),stats:[encode(g.stats.objects.total),encode(g.stats.objects.inUse),encode(g.stats.bindingsPerObject)]});
function normal(r){
 const roots=r.uuids.map(root),g=new G(...r.initial.map(i=>roots[i])),results=[state(g)],subscriptions=[];
 for(const a of r.actions){let error='',returned;try{
  if(a.op==='subscribe'){returned=g.subscribe_(a.path,a.parsed);subscriptions.push(returned);}
  else if(a.op==='unsubscribe')returned=g.unsubscribe_(a.path);
  else if(a.op==='set')P.create(g,a.path).setValue(new Float64Array([a.value]),0);
  else returned=g[a.op](...a.ids.map(i=>roots[i]));
 }catch(e){error=e.name;}
 results.push({error,returnUndefined:returned===undefined,state:state(g),retained:subscriptions.map(bindingArray),values:roots.map(r=>encode(r.value))});
 }return results;
}
function callbacks(r){
 const events=[],g=new G(root('a'),root('b'));
 if(r.name==='constructor'){
  const old=Object.getOwnPropertyDescriptor(Array.prototype,'slice'),replacement=function(...args){events.push(['slice',this.length,this[0].uuid]);return Reflect.apply(old.value,this,args);};
  const originalCall=replacement.call;Object.defineProperty(replacement,'call',{get(){events.push('getCall');return originalCall;},enumerable:true,configurable:true});
  Object.defineProperty(Array.prototype,'slice',{get(){events.push('getSlice');return replacement;},enumerable:true,configurable:true});
  const created=new G(root('x'),root('y'));Object.defineProperty(Array.prototype,'slice',old);return {events,state:state(created)};
 }
 if(r.name==='stats'){
  g.remove(g._objects[0]);const counts=g.stats.objects,other={total:20};
  return [Object.getOwnPropertyDescriptor(counts,'total').get.call(other),Object.getOwnPropertyDescriptor(counts,'inUse').get.call(other),Object.getOwnPropertyDescriptor(g.stats,'bindingsPerObject').get.call(other),g.add===new G().add,Object.hasOwn(g,'add'),Object.getOwnPropertyNames(Object.getPrototypeOf(g))];
 }
 if(r.name==='uuidReenter'){
  const a=root('c');Object.defineProperty(a,'uuid',{enumerable:true,configurable:true,get(){events.push('uuid');g._objects=[];return 'c';}});g.add(a);return {events,state:state(g)};
 }
 if(r.name==='fieldOrder'){
  for(const k of ['_objects','_indicesByUUID','_paths','_parsedPaths','_bindings','nCachedObjects_']){const value=g[k];Object.defineProperty(g,k,{enumerable:true,configurable:true,get(){events.push(k);return value;},set(value){events.push(['set',k,value]);}});}
  g.add();g.remove();g.uncache();return events;
 }
 if(r.name==='pushOrder'){
  for(const [array,label] of [[g._objects,'objects'],[g._paths,'paths'],[g._parsedPaths,'parsed'],[g._bindings,'bindings']]){const original=array.push;Object.defineProperty(array,'push',{enumerable:true,configurable:true,get(){events.push(label);return original;}});}
  g.subscribe_('.value');g.add(root('c'));return {events,state:state(g)};
 }
 if(r.name==='keyOrder'){
  const key={toString(){events.push('key');return '.value';}},parsed=P.parseTrackName('.value'),bindings=g.subscribe_(key,parsed);g.unsubscribe_(key);return {events,state:state(g),retained:bindingArray(bindings)};
 }
 if(r.name==='readonlyDelete'){
  Object.defineProperty(g._indicesByUUID,'a',{configurable:false});let error='';try{g.uncache(g._objects[0]);}catch(e){error=e.name;}return {error,state:state(g)};
 }
 if(r.name==='uuidKey'){
  const a=root({toString(){events.push('uuidKey');return 'c';}});g.add(a);g.remove(a);g.add(a);g.uncache(a);return {events,state:state(g)};
 }
 if(r.name==='bindingPushOrder'){
  const bindings=g.subscribe_('.value'),original=bindings.push;Object.defineProperty(bindings,'push',{get(){events.push('binding.push');return original;},enumerable:true,configurable:true});
  for(const field of ['_paths','_parsedPaths']){const array=g[field],value=array[0];Object.defineProperty(array,'0',{get(){events.push(field);return value;},enumerable:true,configurable:true});}
  g.add(root('c'));return events;
 }
 if(r.name==='indexObject'){
  const a=g._objects[0];g.remove(a);g._indicesByUUID.a={valueOf(){events.push('number');return 0;},toString(){events.push('string');return '0';}};g.add(a);return {events,state:state(g)};
 }
 throw TypeError('Unknown callback scenario');
}
const requests=[];
for(const initial of [[],[0],[0,1,2],[0,0,1]])requests.push({uuids:['a','b','c','d','e'],initial,actions:[
 {op:'subscribe',path:'.value'},{op:'remove',ids:[0,2]},{op:'subscribe',path:'.other'},{op:'add',ids:[3,0,1]},
 {op:'set',path:'.value',value:7},{op:'uncache',ids:[2,1]},{op:'unsubscribe',path:'.other'},{op:'subscribe',path:'.other'},
 {op:'remove',ids:[4,3,0]},{op:'uncache',ids:[0,3]},{op:'add',ids:[4]},{op:'unsubscribe',path:'.value'},{op:'subscribe',path:'.value'}]});
for(const key of ['toString','valueOf','constructor','hasOwnProperty','__proto__','toLocaleString','isPrototypeOf','propertyIsEnumerable','__defineGetter__','__defineSetter__','__lookupGetter__','__lookupSetter__']){
 requests.push({uuids:[key,'ordinary'],initial:[0],actions:[{op:'add',ids:[0,1]},{op:'remove',ids:[0]},{op:'uncache',ids:[0]}]});
 requests.push({uuids:['a','b'],initial:[0,1],actions:[{op:'subscribe',path:key},{op:'unsubscribe',path:key},{op:'subscribe',path:key}]});
}
for(let seed=1;seed<=50;seed++){
 let random=seed;const next=()=>random=(Math.imul(random,1664525)+1013904223)>>>0;
 const actions=[];for(let i=0;i<30;i++){const op=['add','remove','uncache','subscribe','unsubscribe'][next()%5];if(op==='subscribe'||op==='unsubscribe')actions.push({op,path:['.value','.other','.third'][next()%3]});else actions.push({op,ids:Array.from({length:next()%4},()=>next()%5)});}
 requests.push({uuids:['a','b','c','d','e'],initial:[0,1,2],actions});
}
for(const name of ['constructor','stats','uuidReenter','fieldOrder','pushOrder','keyOrder','readonlyDelete','uuidKey','bindingPushOrder','indexObject'])requests.push({kind:'callback',name});
const expected=requests.map(r=>{const messages=[],previous=console.error;console.error=(...args)=>messages.push(args.join(' '));let result;try{result=r.kind==='callback'?callbacks(r):normal(r);}catch(e){result={error:e.name};}finally{console.error=previous;}return {result,messages};});
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:80*1024*1024}));
let checks=0;function compare(a,b,where='$'){checks++;if(typeof b==='number')assert.ok(typeof a==='number'&&Math.abs(a-b)<=1e-13*Math.max(1,Math.abs(b)),`${where}: ${a} != ${b}`);
 else if(b&&typeof b==='object'){assert.equal(Array.isArray(a),Array.isArray(b),where);assert.deepEqual(Object.keys(a),Object.keys(b),`${where}: keys`);for(const k of Object.keys(b))compare(a[k],b[k],`${where}.${k}`);}else assert.equal(a,b,where);}
compare(actual,expected);console.log(`AnimationObjectGroup parity: ${requests.length} scenarios / ${checks} comparisons passed`);
