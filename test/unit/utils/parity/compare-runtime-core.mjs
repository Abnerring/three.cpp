import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-runtime-core.mjs <upstream> <binary>');
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {EventDispatcher}=await import(pathToFileURL(path.join(upstream,'src/core/EventDispatcher.js')));
const encode=v=>v===undefined?'$undefined':typeof v==='function'?'$function':typeof v==='number'?(Number.isNaN(v)?'$NaN':v===Infinity?'$Infinity':v===-Infinity?'$-Infinity':Object.is(v,-0)?'$-0':v):Array.isArray(v)?Array.from({length:v.length},(_,i)=>Object.hasOwn(v,i)?encode(v[i]):'$hole'):v!==null&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const decode=v=>v==='$undefined'?undefined:v==='$NaN'?NaN:v==='$Infinity'?Infinity:v==='$-Infinity'?-Infinity:v==='$-0'?-0:v;
const snapshot=o=>Object.getOwnPropertyNames(o).map(key=>{const d=Object.getOwnPropertyDescriptor(o,key);return 'value'in d?{key,enumerable:d.enumerable,configurable:d.configurable,value:encode(d.value),writable:d.writable}:{key,enumerable:d.enumerable,configurable:d.configurable,get:typeof d.get==='function',set:typeof d.set==='function'};});
function effect(self,s){if(Object.hasOwn(s,'erase'))delete self[s.erase];if(Object.hasOwn(s,'setLength'))self.length=decode(s.setLength);if(Object.hasOwn(s,'setKey'))self[s.setKey]=decode(s.setValue);if(s.throw)throw TypeError('test callback');}
function converted(s,self,log){return s!==null&&typeof s==='object'&&Object.hasOwn(s,'coerce')?{valueOf(){log.push(s.coerce);effect(self,s);return decode(s.value);}}:decode(s);}
function arrayScenario(r){
 const log=[],self=r.array?[]:{};for(const[k,v]of Object.entries(r.entries))self[k]=decode(v);self.length=decode(r.length);
 if(r.prototype){const p=Object.create(Object.getPrototypeOf(self));for(const[k,v]of Object.entries(r.prototype))p[k]=decode(v);Object.setPrototypeOf(self,p);}
 for(const s of r.descriptors??[]){const d={configurable:s.configurable??true,enumerable:s.enumerable??true};if(s.accessor){d.get=function(){log.push(['get',s.key]);effect(this,s);return decode(s.value);};if(s.setter)d.set=function(v){log.push(['set',s.key,v]);};}else {d.value=decode(s.value);d.writable=s.writable??true;}Object.defineProperty(self,s.key,d);}
 if(Object.hasOwn(r,'constructor')){const s=r.constructor;if(s!==null&&typeof s==='object'&&Object.hasOwn(s,'get'))Object.defineProperty(self,'constructor',{enumerable:true,configurable:true,get(){log.push('constructor');effect(this,s);return s.get==='$object'?{}:decode(s.get);}});else self.constructor=s==='$object'?{}:converted(s,self,log);}
 if(r.freezeLength)Object.defineProperty(self,'length',{writable:false});if(r.preventExtensions)Object.preventExtensions(self);
 const args=r.args.map(s=>converted(s,self,log));let result,error='';try{result=Reflect.apply(Array.prototype[r.method],self,args);}catch(e){error=e.name;}
 return {error,result:encode(result),state:snapshot(self),log:encode(log)};
}
function eventScenario(r){
 const self=new EventDispatcher(),log=[],event={type:'x'},record=label=>function(e){log.push([label,e.target===this]);};let a=record('a'),b=record('b'),c=record('c'),error='';
 if(r.name==='metadata'){
  try{self.constructor();}catch(e){error=e.name;}
  return {names:Object.getOwnPropertyNames(Object.getPrototypeOf(self)),constructor:self.constructor.name,prototypeIdentity:self.constructor.prototype===Object.getPrototypeOf(self),methods:['addEventListener','hasEventListener','removeEventListener','dispatchEvent'].map(k=>[k,self[k].name,self[k].length,self[k]===new EventDispatcher()[k],Object.hasOwn(self,k)]),error};
 }
 try{
  if(r.name==='basic'){log.push(self.hasEventListener('x',a));self.removeEventListener('x',a);self.addEventListener('x',a);self.addEventListener('x',a);self.addEventListener('x',b);log.push(self.hasEventListener('x',a));self.dispatchEvent(event);self.removeEventListener('x',a);self.dispatchEvent(event);log.push(self.hasEventListener('x',a));}
  else if(r.name==='mutate'){a=function(e){log.push(['a',e.target===this]);this.removeEventListener('x',b);this.addEventListener('x',c);};self.addEventListener('x',a);self.addEventListener('x',b);self.dispatchEvent(event);self.dispatchEvent(event);}
  else if(r.name==='throw'){a=function(){log.push('throw');throw TypeError('listener');};self.addEventListener('x',a);self.addEventListener('x',b);self.dispatchEvent(event);}
  else if(r.name==='reenter'){let depth=0;a=function(e){log.push(['a',e.target===this]);if(depth===0){depth=1;this.dispatchEvent(e);depth=0;}};self.addEventListener('x',a);self.addEventListener('x',b);self.dispatchEvent(event);}
  else if(r.name==='callOverride'){const listener={};Object.defineProperty(listener,'call',{get(){log.push('getCall');return function(receiver,e){log.push(['call',this===listener,receiver===self,e.target===receiver]);};},enumerable:true,configurable:true});self.addEventListener('x',listener);self.dispatchEvent(event);}
  else if(r.name==='early'){Object.defineProperty(event,'type',{get(){throw TypeError('must not read');},enumerable:true,configurable:true});self.dispatchEvent(event);}
  else if(r.name==='readonlyTarget'){Object.defineProperty(event,'target',{value:3,writable:false,enumerable:true,configurable:true});self.addEventListener('x',a);self.dispatchEvent(event);}
  else if(r.name==='typeOrder'){const key={toString(){log.push('type');return 'x';}},listeners={};Object.defineProperty(self,'_listeners',{get(){log.push('listeners');return listeners;},enumerable:true,configurable:true});self.addEventListener(key,a);self.hasEventListener(key,a);self.removeEventListener(key,a);event.type=key;self.dispatchEvent(event);}
  else if(r.name==='sliceOverride'){const length={valueOf(){log.push('length');return 2;}},copied={0:a,1:b,length},listeners=[a];Object.defineProperty(listeners,'slice',{get(){log.push('slice');return function(start){log.push(['sliceCall',start]);return copied;};},enumerable:true,configurable:true});self._listeners={x:listeners};self.dispatchEvent(event);}
  else if(r.name==='prototypeKey')self.addEventListener(r.key,a);else throw TypeError('Unknown event scenario');
 }catch(e){error=e.name;}
 return {error,log:encode(log),target:event.target===self?'$self':encode(event.target)};
}
function functionScenario(r){const log=[],f=function(...args){log.push([this,args]);return 17;};f.call=17;let result,error='';try{result=Reflect.apply(Function.prototype.call,r.nonCallable?{}:f,r.args.map(decode));}catch(e){error=e.name;}return {error,result:encode(result),log:encode(log)};}
const cases=[];
const base=(method,args=[],array=true)=>({kind:'array',method,args,array,entries:{0:10,2:20,4:'$undefined'},length:5});
const argsByMethod={push:[[],[9],[9,8]],pop:[[]],slice:[[],[1],[1,4],[-3],[-10,10],['$NaN','$Infinity'],['$Infinity'],[3,1]],indexOf:[[20],['$undefined'],[10,-20],[20,3],['$NaN'],[20,'$-Infinity'],[20,'$Infinity']],splice:[[],[1],[1,0,9],[1,2],[1,3,9,8,7,6],[-2,1],[-20,20],[20,0,9],[2,'$NaN',1]]};
for(const array of [true,false])for(const[method,allArgs]of Object.entries(argsByMethod))for(const args of allArgs)for(const variant of ['plain','inherited','frozenLength','sealedIndex','getter','getterSetter','nonextensible']){
 const r=base(method,args,array);if(variant==='inherited')r.prototype={1:11,3:33};if(variant==='frozenLength')r.freezeLength=true;if(variant==='sealedIndex')r.descriptors=[{key:'2',value:20,writable:false,configurable:false}];if(variant==='getter'||variant==='getterSetter')r.descriptors=[{key:'1',value:11,accessor:true,setter:variant==='getterSetter'}];if(variant==='nonextensible')r.preventExtensions=true;cases.push(r);
}
for(const array of [true,false])for(const method of ['slice','splice'])for(const constructor of [null,1,'$undefined','$object',{get:'$undefined'},{get:'$object',erase:'2'},{get:'$undefined',setLength:0},{get:'$object',throw:true}])cases.push({...base(method,[{coerce:'start',value:1},{coerce:'end',value:3}],array),constructor});
for(const method of ['slice','splice','indexOf'])for(const effect of [{erase:'2'},{setLength:0},{setKey:'3',setValue:99},{throw:true}]){
 const args=method==='indexOf'?[20,{coerce:'from',value:0,...effect}]:[{coerce:'start',value:0,...effect},{coerce:'end',value:4}];cases.push(base(method,args));
}
for(const method of ['slice','splice','indexOf','pop'])for(const change of [{erase:'2'},{setLength:0},{setKey:'3',setValue:99},{throw:true}])cases.push({...base(method,method==='indexOf'?[20]:method==='splice'?[0,4]:[]),descriptors:[{key:method==='pop'?'4':'0',value:10,accessor:true,...change}]});
for(const length of [0,-1,1.5,'$NaN','$-Infinity','3'])for(const method of ['push','pop','slice','indexOf','splice'])cases.push({...base(method,method==='indexOf'?[10]:[],false),length});
for(const method of ['push','pop','slice','indexOf','splice'])cases.push({...base(method,method==='indexOf'?[10,{coerce:'never',value:0,throw:true}]:[]),entries:{},length:0});
for(const length of [4294967294,4294967295])for(const[method,args]of [['push',[]],['push',[7,8]],['pop',[]],['slice',[length-2]],['splice',[length,0,7]],['splice',[length-1,1]]])cases.push({...base(method,args),entries:{},length});
for(const length of [9007199254740990,9007199254740991,'$Infinity'])for(const[method,args]of [['push',[]],['push',[7,8]],['pop',[]],['slice',[9007199254740989]],['indexOf',[7,9007199254740989]],['splice',[9007199254740991,0,7]]])cases.push({...base(method,args,false),entries:{},length});
for(const name of ['metadata','basic','mutate','throw','reenter','callOverride','early','readonlyTarget','typeOrder','sliceOverride'])cases.push({kind:'event',name});
for(const key of ['constructor','toString','__proto__','hasOwnProperty'])cases.push({kind:'event',name:'prototypeKey',key});
for(const args of [[],['$undefined'],[null],[3,1,2],['x',1],[false,'$undefined'],[{},1],[[],1]])for(const nonCallable of [false,true])cases.push({kind:'function',args,nonCallable});
const expected=cases.map(r=>{try{return r.kind==='event'?eventScenario(r):r.kind==='function'?functionScenario(r):arrayScenario(r);}catch(e){return {outerError:e.name};}});
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:64*1024*1024}));
assert.equal(actual.length,expected.length);let comparisons=0;
function compare(a,e,p){comparisons++;if(Array.isArray(e)){assert.ok(Array.isArray(a),p);assert.equal(a.length,e.length,p);e.forEach((x,i)=>compare(a[i],x,`${p}[${i}]`));}else if(e!==null&&typeof e==='object'){assert.deepEqual(Object.keys(a).sort(),Object.keys(e).sort(),p);for(const k of Object.keys(e))compare(a[k],e[k],`${p}.${k}`);}else assert.equal(a,e,p);}
for(let i=0;i<cases.length;i++){try{compare(actual[i],expected[i],`case ${i}`);}catch(e){console.error(JSON.stringify(cases[i]));console.error('actual',JSON.stringify(actual[i]));console.error('expected',JSON.stringify(expected[i]));throw e;}}
console.log(`Runtime core parity: ${cases.length} scenarios, ${comparisons} comparisons passed (${lock.commit}).`);
