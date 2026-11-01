import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
const options={};for(let i=2;i<process.argv.length;++i){const key=process.argv[i];assert(['--probe','--input','--output','--oracle-only'].includes(key),'Unknown or positional argument: '+key);assert(!(key in options),'Duplicate argument: '+key);if(key==='--oracle-only'){options[key]=true;continue;}assert(i+1<process.argv.length&&!process.argv[i+1].startsWith('--'),'Missing value: '+key);options[key]=process.argv[++i];}
assert(options['--output'],'--output is required');assert(Boolean(options['--probe'])!==Boolean(options['--oracle-only']),'Use exactly one of --probe and --oracle-only');assert.equal(typeof globalThis.gc,'function','Use --expose-gc for real GC cases');
const binary=options['--probe']?path.resolve(options['--probe']):null,directory=path.resolve(options['--output']);assert(!fs.existsSync(directory),'Preserve evidence: choose a new output directory');
const nativeMath=Math;
const normal=value=>value===undefined?'#undefined':typeof value==='symbol'?'#symbol':typeof value==='number'?(Number.isNaN(value)?'#NaN':!Number.isFinite(value)?value>0?'#Infinity':'#-Infinity':Object.is(value,-0)?'#-0':value):value;
function decode(value){
 if(value===null||typeof value!=='object')return value;
 switch(value.tag){
  case 'undefined':return undefined;case 'NaN':return NaN;case 'Infinity':return Infinity;case '-Infinity':return -Infinity;case '-0':return -0;case 'symbol':return Symbol('fixture');case 'object':return {};
  case 'boxedNumber':return new Number(decode(value.value));case 'boxedString':return new String(decode(value.value));
 }
 return value;
}
const data=(object,key,value)=>Object.defineProperty(object,key,{value,writable:true,enumerable:false,configurable:true});
function protocolValue(state,definition){
 if(definition.tag!=='primitive'&&definition.tag!=='fallback')return decode(definition);
 const object={},name=definition.name;
 const install=(key,method)=>Object.defineProperty(object,key,{configurable:true,get(){
  tick(name+'.get.'+method);
  if(definition.noncallable===method)return 3;
  const receiver=this;
  return function(...args){
   tick(name+'.call.'+method);state.events.push([name+'.receiver',this===receiver]);
   if(method==='toPrimitive')state.events.push([name+'.hint',args[0]]);
   if(definition.nested)state.events.push([name+'.nested',normal(Reflect.apply(state.function,null,[8,-3]))]);
   if(definition.replaceMethod){data(state.math,state.request.name,function replacement(){return 77;});state.events.push([name+'.replaceMethod']);}
   if(definition.replaceGlobal){const replacement={};data(replacement,state.request.name,function replacement(){return 88;});globalThis.Math=replacement;state.events.push([name+'.replaceGlobal']);}
   if(definition.gc){globalThis.gc();state.events.push([name+'.gc']);}
   if(method==='valueOf'&&definition.valueOfObject)return {};
   return decode(definition.value);
  };
 }});
 function tick(label){state.events.push([label]);if(state.request.throwAt===label)throw new TypeError(label);}
 if(definition.tag==='primitive')install(Symbol.toPrimitive,'toPrimitive');else {install('valueOf','valueOf');install('toString','toString');}
 return definition.proxy?new Proxy(object,{get(target,key,receiver){tick(name+'.proxy.get.'+(typeof key==='symbol'?'toPrimitive':String(key)));return Reflect.get(target,key,receiver);}}):object;
}
const flags=descriptor=>descriptor?[descriptor.writable??false,descriptor.enumerable??false,descriptor.configurable??false]:undefined;
const constructible=value=>{try{Reflect.construct(function(){},[],value);return true;}catch{return false;}};
function metadata(){
 const math=globalThis.Math,out={};
 for(const name of ['floor']){
  const fn=math[name];out[name]={identity:fn===globalThis.Math[name],name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn),constructible:constructible(fn),ownPrototype:Object.hasOwn(fn,'prototype'),functionPrototype:Object.getPrototypeOf(fn)===Function.prototype,nameFlags:flags(Object.getOwnPropertyDescriptor(fn,'name')),lengthFlags:flags(Object.getOwnPropertyDescriptor(fn,'length')),propertyFlags:flags(Object.getOwnPropertyDescriptor(math,name))};
 }
 out.globalMathFlags=flags(Object.getOwnPropertyDescriptor(globalThis,'Math'));out.tag=math[Symbol.toStringTag];return out;
}
function run(request){
 if(request.kind==='metadata')return metadata();
 const math=globalThis.Math,fn=math[request.name],state={request,events:[],math,function:fn},args=request.args.map(value=>value!==null&&typeof value==='object'?protocolValue(state,value):value),out={};
 try{out.result=normal(request.construct?Reflect.construct(fn,args):Reflect.apply(fn,decode(request.receiver),args));}catch(error){out.result='#'+error.name;}
 out.methodSame=math[request.name]===fn;out.globalSame=globalThis.Math===math;out.events=state.events;
 data(math,request.name,fn);globalThis.Math=math;
 if(request.gcAfter)globalThis.gc();return out;
}
const U={tag:'undefined'},N={tag:'NaN'},P={tag:'Infinity'},M={tag:'-Infinity'},Z={tag:'-0'},S={tag:'symbol'},O={tag:'object'},primitive=(name,value,options={})=>({tag:'primitive',name,value,...options}),fallback=(name,value,options={})=>({tag:'fallback',name,value,...options});let cases=[{kind:'metadata'}];
const add=(name,args,other={})=>cases.push({name,args,...other});
const values=[U,null,false,true,'','  ','-0','3.5','0x10','no-number',N,Z,0,1,-1,Number.MIN_VALUE,-Number.MIN_VALUE,Number.MAX_VALUE,-Number.MAX_VALUE,-2.9,-1.9,-0.9,0.9,1.9,2.9,P,M,S,O,[],[null],[1],['2'],[1,2],{tag:'boxedNumber',value:Z},{tag:'boxedString',value:'-3.5'}];
for(const name of ['floor']){
 for(const args of [[],...values.map(value=>[value]),[0,S],[-1,primitive('ignored',2)],[primitive('a',-4),primitive('ignored',2)]])add(name,args);
 for(const value of [Z,N,-3,S,O])for(const options of [{},{nested:true},{replaceMethod:true},{replaceGlobal:true},{gc:true},{proxy:true},{noncallable:'toPrimitive'}])for(const throwAt of ['','a.get.toPrimitive','a.call.toPrimitive'])add(name,[primitive('a',value,options)],{throwAt});
 for(const value of [3,N,S,O])for(const options of [{},{valueOfObject:true},{noncallable:'valueOf'}])for(const throwAt of ['','fallback.get.valueOf','fallback.call.valueOf','fallback.get.toString','fallback.call.toString'])add(name,[fallback('fallback',value,options)],{throwAt});
 for(const receiver of [U,null,0,true,'ignored',O])add(name,[primitive('a',-3)],{receiver});
 for(const args of [[],[primitive('never',2)]])add(name,args,{construct:true});
 add(name,[primitive('gc',-3,{gc:true})],{gcAfter:true});
}
// Both sides of integer boundaries across the binary64 exponent range.
const bits=new DataView(new ArrayBuffer(8));
for(let exponent=-1074;exponent<=1023;exponent+=7){
 const center=2**exponent;
 bits.setFloat64(0,center,false);const raw=bits.getBigUint64(0,false);
 for(const offset of [-1n,0n,1n]){const word=raw+offset;if(word<0n)continue;bits.setBigUint64(0,word,false);const value=bits.getFloat64(0,false);if(Number.isFinite(value))for(const sign of [1,-1])add('floor',[Object.is(sign*value,-0)?Z:sign*value]);}
}
for(const integer of [1,2,3,127,128,255,256,1023,1024,2**31,2**32,2**52,2**53]){
 bits.setFloat64(0,integer,false);const raw=bits.getBigUint64(0,false);
 for(const offset of [-1n,0n,1n]){bits.setBigUint64(0,raw+offset,false);const value=bits.getFloat64(0,false);for(const sign of [1,-1])add('floor',[Object.is(sign*value,-0)?Z:sign*value]);}
}
if(options['--input'])cases=fs.readFileSync(path.resolve(options['--input']),'utf8').trimEnd().split(/\r?\n/).map(JSON.parse);
const input=cases.map(value=>JSON.stringify(value)).join('\n')+'\n',expected=cases.map(run);assert.equal(globalThis.Math,nativeMath,'Oracle restores Math after each case');
fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'input.jsonl'),input);fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases,null,2)+'\n');fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2)+'\n');
const digest=value=>createHash('sha256').update(value).digest('hex'),summary={mode:binary?'native-differential':'oracle-only',binaryExecuted:false,cases:cases.length,node:process.version,v8:process.versions.v8,scope:'Real Math.floor: one ToNumber, binary64 integer neighborhoods, signed zero, infinities, metadata, observable conversion/reentry/mutation/GC. No normalized algorithm, identity, events or descriptors.',pending:['BigInt values and boxed Symbol/cross-realm dependencies are not supplied by this native fixture.','Complete global object surface is not claimed.'],inputSHA256:digest(input),expectedSHA256:digest(fs.readFileSync(path.join(directory,'expected.json')))};
if(binary){summary.binaryBeforeSHA256=digest(fs.readFileSync(binary));const child=spawnSync(binary,[],{input,encoding:'utf8',maxBuffer:64*1024*1024});summary.binaryExecuted=true;summary.exitCode=child.status;summary.signal=child.signal;summary.error=child.error?.message;summary.binaryAfterSHA256=digest(fs.readFileSync(binary));for(const [name,value]of [['native-stdout.jsonl',child.stdout??''],['native-stderr.txt',child.stderr??''],['native-exit.json',JSON.stringify({status:child.status,signal:child.signal,error:child.error?.message},null,2)]])fs.writeFileSync(path.join(directory,name),value);fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');assert.equal(child.status,0,child.stderr||String(child.error));assert.equal(summary.binaryBeforeSHA256,summary.binaryAfterSHA256);const actual=child.stdout.trimEnd().split(/\r?\n/).map(JSON.parse);fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2)+'\n');assert.equal(actual.length,cases.length);const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2)+'\n');summary.differences=differences.length;summary.passed=cases.length-differences.length;fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));}
else fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');console.log(JSON.stringify(summary));
