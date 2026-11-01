import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync,execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const allowed=new Set(['--probe','--output','--input','--upstream']),options=new Map(),args=process.argv.slice(2);
for(let i=0;i<args.length;i+=2){const key=args[i],value=args[i+1];if(!allowed.has(key)||options.has(key)||!value||value.startsWith('--'))throw new Error('Usage: node --expose-gc compare-math-inverse-trig.mjs --output new-directory [--probe binary] [--input cases.json] [--upstream checkout]');options.set(key,value);}
assert(options.has('--output'),'--output is required');assert.equal(process.version,'v26.5.0','Pinned Node oracle');assert.equal(typeof globalThis.gc,'function','Use --expose-gc');
const directory=path.resolve(options.get('--output')),binary=options.has('--probe')?path.resolve(options.get('--probe')):null;
assert(!fs.existsSync(directory),'Preserve prior evidence: output directory already exists');
if(binary)assert(fs.statSync(binary).isFile(),'--probe must name an existing binary');
if(options.has('--input'))assert(fs.statSync(options.get('--input')).isFile(),'--input must name an existing JSON array');
if(options.has('--upstream'))assert.equal(execFileSync('git',['-C',path.resolve(options.get('--upstream')),'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
process.env.TEMP='D:/Temp/surui';process.env.TMP='D:/Temp/surui';
const nativeMath=Math;
const normal=value=>typeof value==='number'?{type:'number',value:Number.isNaN(value)?'NaN':!Number.isFinite(value)?value>0?'Infinity':'-Infinity':Object.is(value,-0)?'-0':value}:value===undefined?'#undefined':typeof value==='symbol'?'#symbol':value;
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
 for(const name of ['acos','asin','atan2']){
  const fn=math[name];out[name]={source:Function.prototype.toString.call(fn),identity:fn===globalThis.Math[name],name:fn.name,length:fn.length,constructible:constructible(fn),ownPrototype:Object.hasOwn(fn,'prototype'),functionPrototype:Object.getPrototypeOf(fn)===Function.prototype,nameFlags:flags(Object.getOwnPropertyDescriptor(fn,'name')),lengthFlags:flags(Object.getOwnPropertyDescriptor(fn,'length')),propertyFlags:flags(Object.getOwnPropertyDescriptor(math,name))};
 }
 out.PI=normal(math.PI);out.PIFlags=flags(Object.getOwnPropertyDescriptor(math,'PI'));out.globalMathFlags=flags(Object.getOwnPropertyDescriptor(globalThis,'Math'));out.tag=math[Symbol.toStringTag];return out;
}
function run(request){
 if(request.kind==='metadata')return metadata();
 if(request.kind==='pi')return piCase(request);if(request.kind==='binding')return bindingCase(request);
 const math=globalThis.Math,fn=math[request.name],state={request,events:[],math,function:fn},args=request.args.map(value=>value!==null&&typeof value==='object'?protocolValue(state,value):value),out={};
 try{out.result=normal(request.construct?Reflect.construct(fn,args):Reflect.apply(fn,decode(request.receiver),args));}catch(error){out.result='#'+error.name;}
 out.methodSame=math[request.name]===fn;out.globalSame=globalThis.Math===math;out.events=state.events;
 data(math,request.name,fn);globalThis.Math=math;
 if(request.gcAfter)globalThis.gc();return out;
}
function piCase(request){const math=globalThis.Math,state={events:[]},out={};try{switch(request.operation){
 case 'strictSet':out.result=(math.PI=17);break;
 case 'reflectSet':out.result=Reflect.set(math,'PI',17);break;
 case 'reflectDelete':out.result=Reflect.deleteProperty(math,'PI');break;
 default:{const d=request.operation==='defineSame'?{value:math.PI}:request.operation==='defineValue'?{value:17}:request.operation==='defineWritable'?{writable:true}:request.operation==='defineEnumerable'?{enumerable:true}:request.operation==='defineConfigurable'?{configurable:true}:request.operation==='defineAccessor'?{get(){state.events.push(['get.PI']);return 17;}}:null;assert(d,'Unknown PI fixture operation');out.result=Object.defineProperty(math,'PI',d)===math;}
 }}catch(e){out.result='#'+e.name;}out.result=normal(out.result);out.value=normal(math.PI);out.flags=flags(Object.getOwnPropertyDescriptor(math,'PI'));out.events=state.events;return out;}
function bindingCase(request){const math=globalThis.Math,fn=math[request.name],state={events:[]},out={};if(request.mode==='deleted'){out.delete=Reflect.deleteProperty(globalThis,'Math');try{out.lookup=Math;}catch(e){out.lookup='#'+e.name;}}else{Object.defineProperty(globalThis,'Math',{configurable:true,get(){state.events.push(['get.Math']);return math;}});out.lookupSame=Math===math;}
 out.savedResult=normal(Reflect.apply(fn,null,[.25,-.5]));globalThis.gc();out.afterGC=normal(Reflect.apply(fn,null,[.25,-.5]));data(globalThis,'Math',math);out.mathSame=Math===math;out.functionSame=Math[request.name]===fn;out.events=state.events;return out;}
const U={tag:'undefined'},N={tag:'NaN'},P={tag:'Infinity'},M={tag:'-Infinity'},Z={tag:'-0'},S={tag:'symbol'},O={tag:'object'},primitive=(name,value,options={})=>({tag:'primitive',name,value,...options}),fallback=(name,value,options={})=>({tag:'fallback',name,value,...options});
function generatedCases(){const cases=[{kind:'metadata'}],add=(name,args,other={})=>cases.push({name,args,...other});
 const wire=x=>Number.isNaN(x)?N:Object.is(x,-0)?Z:x===Infinity?P:x===-Infinity?M:x;
 const nextUp=x=>{if(x===0)return Number.MIN_VALUE;const d=new DataView(new ArrayBuffer(8));d.setFloat64(0,x);d.setBigUint64(0,d.getBigUint64(0)+(x>0?1n:-1n));return d.getFloat64(0);},nextDown=x=>-nextUp(-x);
 const boundaries=[0,-0,Number.MIN_VALUE,-Number.MIN_VALUE,2**-1022,-(2**-1022),Number.MAX_VALUE,-Number.MAX_VALUE,NaN,Infinity,-Infinity,1,-1,.5,-.5,2,-2,1e-8,-1e-8];
 for(const point of [-1,-.975,-.5,-(2**-27),0,2**-27,.5,.975,1])boundaries.push(nextDown(point),point,nextUp(point));
 const values=[U,null,false,true,'','  ','-0','0.25','0x10','no-number',S,O,{tag:'boxedNumber',value:Z},{tag:'boxedString',value:'-.5'},...boundaries.map(wire)];
 for(const name of ['acos','asin']){add(name,[]);for(const value of values)add(name,[value]);for(const value of [U,N,Z,P,M,S,.25])add(name,[value,primitive('ignored',S),S]);}
 const grid=boundaries.map(wire);for(const y of grid)for(const x of grid)add('atan2',[y,x]);
 for(const args of [[],[U],[.25],[Z],[P],[S],[N,primitive('x',.5)],[N,primitive('x',S)],[S,primitive('never',.5)],[.25,-.5,primitive('ignored',S),S]])add('atan2',args);
 let seed=0x6d2b79f5;const next=()=>{seed=(nativeMath.imul(seed,1664525)+1013904223)>>>0;return seed/4294967296;};for(let i=0;i<128;++i){const x=next()*2-1,y=next()*2-1;add('acos',[x]);add('asin',[x]);add('atan2',[y,x]);}
 for(const name of ['acos','asin','atan2']){
  for(const position of name==='atan2'?[0,1]:[0])for(const value of [.25,N,Z,S,O])for(const options of [{},{nested:true},{replaceMethod:true},{replaceGlobal:true},{gc:true},{proxy:true},{noncallable:'toPrimitive'}])for(const throwAt of ['', 'a.get.toPrimitive','a.call.toPrimitive']){const args=name==='atan2'?[.25,-.5]:[.25];args[position]=primitive('a',value,options);if(name==='atan2')args[1-position]=primitive(position?'y':'x',-.5);args.push(primitive('ignored',S));add(name,args,{throwAt});}
  for(const position of name==='atan2'?[0,1]:[0])for(const value of ['.25',N,S,O])for(const options of [{},{valueOfObject:true},{noncallable:'valueOf'}])for(const throwAt of ['','fallback.get.valueOf','fallback.call.valueOf','fallback.get.toString','fallback.call.toString']){const args=name==='atan2'?[.25,-.5]:[.25];args[position]=fallback('fallback',value,options);args.push(primitive('ignored',S));add(name,args,{throwAt});}
  for(const receiver of [U,null,0,false,'ignored',O])add(name,[primitive('a',.25),primitive('b',-.5)],{receiver});
  for(const args of [[],[primitive('never',.5),primitive('alsoNever',.25)]])add(name,args,{construct:true});
  for(const mode of ['deleted','getter'])cases.push({kind:'binding',name,mode});
  add(name,[primitive('gc',.25,{gc:true}),-.5],{gcAfter:true});
 }
 for(const operation of ['strictSet','reflectSet','reflectDelete','defineSame','defineValue','defineWritable','defineEnumerable','defineConfigurable','defineAccessor'])cases.push({kind:'pi',operation});cases.push({kind:'metadata'});return cases;
}
const cases=options.has('--input')?JSON.parse(fs.readFileSync(options.get('--input'),'utf8')):generatedCases();assert(Array.isArray(cases),'--input must be a JSON array');assert(cases.length>0,'Empty input is not a successful comparison');
for(const r of cases){assert(r&&typeof r==='object'&&!Array.isArray(r),'Each case must be an object');assert(['metadata','pi','binding'].includes(r.kind)||(['acos','asin','atan2'].includes(r.name)&&Array.isArray(r.args)),'Unsupported case shape');}
fs.mkdirSync(directory,{recursive:true});
const digest=data=>createHash('sha256').update(data).digest('hex'),write=(name,value)=>fs.writeFileSync(path.join(directory,name),JSON.stringify(value,null,2)+'\n');
const input=cases.map(value=>JSON.stringify(value)).join('\n')+'\n';write('input.json',cases);fs.writeFileSync(path.join(directory,'input.jsonl'),input);
const summary={status:'oracle-running',cases:cases.length,node:process.version,v8:process.versions.v8,scriptSha256:digest(fs.readFileSync(fileURLToPath(import.meta.url))),inputSha256:digest(input),scope:['actual global Math.acos/asin/atan2','Math.PI immutable own data descriptor'],encoding:'JSValue trace Number encoding: exact finite binary64 round-trip; NaN, Infinity, -Infinity, -0 tags; no epsilon. NaN payload/sign are not an ECMAScript assertion.',nativeExecuted:false,categories:{metadata:cases.filter(x=>x.kind==='metadata').length,pi:cases.filter(x=>x.kind==='pi').length,binding:cases.filter(x=>x.kind==='binding').length,construct:cases.filter(x=>x.construct).length,acos:cases.filter(x=>x.name==='acos'&&!x.kind).length,asin:cases.filter(x=>x.name==='asin'&&!x.kind).length,atan2:cases.filter(x=>x.name==='atan2'&&!x.kind).length},boundaries:['No BigInt JSValue or foreign-realm borrowing','Only these four Math additions; complete Math is not claimed','Native execution and realm lifetime validation require parent integration build']};
try{
 const expected=cases.map(run);assert.equal(globalThis.Math,nativeMath);write('expected.json',expected);summary.expectedSha256=digest(fs.readFileSync(path.join(directory,'expected.json')));
 // Independently assert oracle invariants so fixture changes cannot silently erase the required protocol.
 if(!options.has('--input')){const meta=expected[0];assert.deepEqual(expected.at(-1),meta,'All fixture mutations must restore the original Math binding/functions/PI');for(const [name,length] of [['acos',1],['asin',1],['atan2',2]]){assert.equal(meta[name].length,length);assert.equal(meta[name].source,`function ${name}() { [native code] }`);assert.equal(meta[name].constructible,false);}assert.deepEqual(meta.PIFlags,[false,false,false]);assert.equal(meta.PI.value,nativeMath.PI);assert(cases.some((r,i)=>r.name==='atan2'&&r.args?.[0]?.tag==='NaN'&&r.args?.[1]?.value?.tag==='symbol'&&expected[i].result==='#TypeError'));assert(cases.filter((r,i)=>r.construct&&expected[i].events.length===0).length===6);}
 if(binary){
  summary.nativeRequested=true;summary.binary=binary;summary.binaryBeforeSha256=digest(fs.readFileSync(binary));
  const child=spawnSync(binary,[],{input,encoding:'utf8',maxBuffer:256*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});
  summary.nativeExecuted=child.pid!==undefined;fs.writeFileSync(path.join(directory,'native-stdout.jsonl'),child.stdout??'');fs.writeFileSync(path.join(directory,'native-stderr.log'),child.stderr??'');
  try{summary.binaryAfterSha256=digest(fs.readFileSync(binary));}catch(error){summary.binaryAfterSha256=null;summary.binaryAfterHashError=String(error);}summary.nativeExit={status:child.status,signal:child.signal,error:child.error?.message??null,pid:child.pid??null};write('native-exit.json',summary.nativeExit);
  let actual=null;const differences=[];try{actual=(child.stdout??'').trimEnd().split(/\r?\n/).filter(Boolean).map(JSON.parse);}catch(error){differences.push({kind:'invalid-native-output',error:String(error)});}write('actual.json',actual);
  if(actual){if(actual.length!==expected.length)differences.push({kind:'count',expected:expected.length,actual:actual.length});for(let index=0;index<expected.length;++index)try{assert.deepEqual(actual[index],expected[index]);}catch{differences.push({index,input:cases[index],expected:expected[index],actual:actual[index]??null});}}
  write('differences.json',differences);summary.differences=differences.length;summary.status=child.status!==0?'native-process-failed':summary.binaryBeforeSha256!==summary.binaryAfterSha256?'binary-changed':differences.length?'native-differences':'native-passed';
  assert.equal(child.status,0,child.stderr||String(child.error));assert.equal(summary.binaryBeforeSha256,summary.binaryAfterSha256,'Binary changed during run');assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));
 }else summary.status='oracle-only-native-not-built';
}catch(error){summary.error=String(error);if(summary.status==='oracle-running')summary.status='oracle-failed';throw error;}finally{write('summary.json',summary);console.log(JSON.stringify(summary));}
