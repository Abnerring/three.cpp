import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';

const [binaryArgument,directoryArgument]=process.argv.slice(2);
assert(binaryArgument&&directoryArgument,'Usage: node --expose-gc compare-math-min-abs.mjs <binary|--oracle> <new-output-directory>');
assert.equal(typeof globalThis.gc,'function','Use --expose-gc for real callback GC cases');
const binary=binaryArgument==='--oracle'?null:path.resolve(binaryArgument),directory=path.resolve(directoryArgument);
assert(!fs.existsSync(directory),'Preserve earlier evidence: choose a new output directory');
fs.mkdirSync(directory,{recursive:true});
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
 for(const name of ['min','abs','max','ceil','round','sqrt','sin','cos','random']){
  const fn=math[name];out[name]={identity:fn===globalThis.Math[name],name:fn.name,length:fn.length,constructible:constructible(fn),ownPrototype:Object.hasOwn(fn,'prototype'),functionPrototype:Object.getPrototypeOf(fn)===Function.prototype,nameFlags:flags(Object.getOwnPropertyDescriptor(fn,'name')),lengthFlags:flags(Object.getOwnPropertyDescriptor(fn,'length')),propertyFlags:flags(Object.getOwnPropertyDescriptor(math,name))};
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
const U={tag:'undefined'},N={tag:'NaN'},P={tag:'Infinity'},M={tag:'-Infinity'},Z={tag:'-0'},S={tag:'symbol'},O={tag:'object'},primitive=(name,value,options={})=>({tag:'primitive',name,value,...options}),fallback=(name,value,options={})=>({tag:'fallback',name,value,...options}),cases=[{kind:'metadata'}];
function add(name,args,other={}){cases.push({name,args,...other});}
const values=[U,null,false,true,'','  ','-0','3.5','0x10','no-number',N,Z,0,1,-1,Number.MIN_VALUE,-Number.MIN_VALUE,Number.MAX_VALUE,-Number.MAX_VALUE,P,M,S,{tag:'boxedNumber',value:Z},{tag:'boxedString',value:'-3.5'}];
for(const name of ['min','max']){
 for(const args of [[],...values.map(value=>[value]),[0,Z],[Z,0],[Z,Z],[P,M],[N,4],[4,N],[N,S],[P,S],[M,S],[S,primitive('later',3)],[1,2,-3,4,5]])add(name,args);
 for(const left of [Z,0,1,-1,N,P,M])for(const right of [Z,0,1,-1,N,P,M])add(name,[left,right]);
 for(const args of [[primitive('a',N),primitive('b',2),primitive('c',3)],[N,primitive('a',2),primitive('b',3)],[primitive('a',1),primitive('b',2),primitive('c',3)],[primitive('a',N),primitive('b',S),primitive('c',3)]])for(const throwAt of ['',...['a','b','c'].flatMap(name=>[name+'.get.toPrimitive',name+'.call.toPrimitive'])])add(name,args,{throwAt});
 for(const value of [N,Z,-4])for(const options of [{nested:true},{replaceMethod:true},{replaceGlobal:true},{gc:true},{proxy:true},{noncallable:'toPrimitive'}])add(name,[primitive('a',value,options),primitive('b',2)]);
 for(const value of [3,N,S,O])for(const options of [{},{valueOfObject:true},{noncallable:'valueOf'}])add(name,[fallback('fallback',value,options),primitive('later',2)]);
}
for(const args of [[],...values.map(value=>[value]),[0,S],[-1,primitive('ignored',2)],[primitive('a',-4),primitive('ignored',2)]])add('abs',args);
for(const value of [Z,N,-3,S,O])for(const options of [{},{nested:true},{replaceMethod:true},{replaceGlobal:true},{gc:true},{proxy:true},{noncallable:'toPrimitive'}])for(const throwAt of ['','a.get.toPrimitive','a.call.toPrimitive'])add('abs',[primitive('a',value,options)],{throwAt});
for(const value of [3,N,S,O])for(const options of [{},{valueOfObject:true},{noncallable:'valueOf'}])for(const throwAt of ['','fallback.get.valueOf','fallback.call.valueOf','fallback.get.toString','fallback.call.toString'])add('abs',[fallback('fallback',value,options)],{throwAt});
for(const name of ['min','max','abs'])for(const receiver of [U,null,0,true,'ignored',O])add(name,[primitive('a',-3)],{receiver});
for(const name of ['min','max','abs'])for(const args of [[],[primitive('never',2)]])add(name,args,{construct:true});
for(const name of ['ceil','round','sqrt','sin','cos'])for(const value of [Z,0,1,-1,N,P,M])add(name,[value]);
for(const name of ['min','abs'])add(name,[primitive('gc',-3,{gc:true})],{gcAfter:true});

const input=cases.map(value=>JSON.stringify(value)).join('\n')+'\n',expected=cases.map(run);
assert.equal(globalThis.Math,nativeMath,'Oracle restores global Math after every case');
fs.writeFileSync(path.join(directory,'input.jsonl'),input);fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2)+'\n');
const digest=value=>createHash('sha256').update(value).digest('hex');
const summary={status:binary?'pending-native':'oracle-only-native-not-built',cases:cases.length,nodeVersion:process.version,v8Version:process.versions.v8,scope:['Math.min','Math.abs','preserved Math.max and unary methods'],inputSha256:digest(input),expectedSha256:digest(fs.readFileSync(path.join(directory,'expected.json'))),categories:{metadata:1,construct:cases.filter(value=>value.construct).length,gc:cases.filter(value=>value.gcAfter||value.args?.some(arg=>arg?.gc)).length,primitiveProtocol:cases.filter(value=>value.args?.some(arg=>arg?.tag==='primitive')).length,fallbackProtocol:cases.filter(value=>value.args?.some(arg=>arg?.tag==='fallback')).length}};
if(binary){
 summary.binarySha256=digest(fs.readFileSync(binary));
 const child=spawnSync(binary,[],{input,encoding:'utf8',maxBuffer:64*1024*1024});fs.writeFileSync(path.join(directory,'native-stdout.jsonl'),child.stdout??'');fs.writeFileSync(path.join(directory,'native-stderr.log'),child.stderr??'');summary.nativeExitCode=child.status;
 assert.equal(child.status,0,child.stderr??String(child.error));
 const actual=child.stdout.trimEnd().split(/\r?\n/).map(JSON.parse),differences=[];assert.equal(actual.length,cases.length);
 for(let index=0;index<cases.length;++index)try{assert.deepEqual(actual[index],expected[index]);}catch{differences.push({index,input:cases[index],expected:expected[index],actual:actual[index]});}
 fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2)+'\n');fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2)+'\n');summary.differences=differences.length;summary.status=differences.length?'native-differences':'native-passed';
 fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');assert.equal(differences.length,0,JSON.stringify(differences.slice(0,3),null,2));
}else fs.writeFileSync(path.join(directory,'summary.json'),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify(summary));
