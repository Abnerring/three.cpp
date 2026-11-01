import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fileURLToPath} from 'node:url';
const modes=['missing','undefined','null','dense','boxedString','callPropertyIgnored','sparse','inherited','string','number','boolean','symbol','negative','nan','fraction','lengthString','lengthUndefined','lengthSymbol','conversion','conversionThrows','primitiveHint','lengthThrows','indexThrows','hugeLength','capacityExceeded','iteratorIgnored','proxy','badTarget','badTargetRevokedList','revokedList','revokedTarget','classTarget','proxyTarget','mutateLength','reentrant','prototypeReplaced','thisUndefined','thisNull','thisNumber','thisObject','bound','callableList','metadata'];
const source=String.raw`
'use strict';
const normalize=v=>v===undefined?'#undefined':typeof v==='symbol'?'#symbol':typeof v==='function'?'#function':typeof v==='number'&&Number.isNaN(v)?'#NaN':v;
const failure=e=>e instanceof Error?'#'+e.name:{thrown:normalize(e)};
const log=[],target=function target(...args){log.push('call');return {thisValue:normalize(this),args:args.map(normalize)}};
let fn=target;const fp=Object.getPrototypeOf(fn),apply=fp.apply,out={};
const getter=(object,key,value)=>Object.defineProperty(object,key,{get:value,enumerable:true,configurable:true});
const read=(key,value,throws=false)=>()=>{log.push(key);if(throws)throw value;return value};
const loggedArray=(length=2)=>{const object={};getter(object,'length',read('length',length));getter(object,'0',read('0',4));getter(object,'1',read('1',5));return object};
function metadata(){
 const descriptor=d=>{const v={enumerable:d.enumerable,configurable:d.configurable};if('value'in d){v.value=normalize(d.value);v.writable=d.writable}return v};
 const result={applyProperty:descriptor(Object.getOwnPropertyDescriptor(fp,'apply')),applyName:descriptor(Object.getOwnPropertyDescriptor(apply,'name')),applyLength:descriptor(Object.getOwnPropertyDescriptor(apply,'length')),applySource:Function.prototype.toString.call(apply),applyConstructible:false};
 try{Reflect.construct(apply,[]);result.applyConstructible=true}catch{}
 const accessors=[],values=[];
 for(const key of ['caller','arguments']){const d=Object.getOwnPropertyDescriptor(fp,key);result[key]=descriptor(d);for(const f of [d.get,d.set]){accessors.push(f);let constructible=false;try{Reflect.construct(f,[]);constructible=true}catch{}values.push({name:descriptor(Object.getOwnPropertyDescriptor(f,'name')),length:descriptor(Object.getOwnPropertyDescriptor(f,'length')),extensible:Object.isExtensible(f),constructible,source:Function.prototype.toString.call(f),keys:Reflect.ownKeys(f)})}}
 result.accessors=values;result.identity=accessors.map(a=>accessors.map(b=>a===b));
 const revoked=Proxy.revocable(fn,{});revoked.revoke();
 result.receivers=[undefined,null,3,'x',{},fp,apply,fn,fn.bind(undefined),new Proxy(fn,{}),revoked.proxy,Function].map(value=>accessors.map(a=>{try{return normalize(Reflect.apply(a,value,[123]))}catch(e){return failure(e)}}));
 const sloppy=Function();result.sloppy=accessors.map(a=>{try{return normalize(Reflect.apply(a,sloppy,[123]))}catch(e){return failure(e)}});result.sloppyOwnKeys=Reflect.ownKeys(sloppy);result.sloppyOwnCaller=Object.hasOwn(sloppy,'caller');result.sloppyOwnArguments=Object.hasOwn(sloppy,'arguments');result.sloppyCallResult=normalize(sloppy());
 const spoofed=function(){'use strict'};spoofed['Function.languageMode']='sloppy-empty';result.sloppyBrandRejected=[spoofed,new Proxy(sloppy,{}),sloppy.bind(undefined)].map(value=>accessors.map(a=>{try{return normalize(Reflect.apply(a,value,[123]))}catch(e){return failure(e)}}));
 for(const a of accessors)a.extra=3;result.accessorsAcceptProperties=accessors[0].extra===3;
 result.directProperties=[];for(const key of ['caller','arguments']){try{result.directProperties.push(normalize(fn[key]))}catch(e){result.directProperties.push(failure(e))}try{fn[key]=123;result.directProperties.push('#undefined')}catch(e){result.directProperties.push(failure(e))}}
 const replacement={marker:1};Object.defineProperty(fp,'caller',{value:replacement,writable:true,enumerable:true,configurable:true});result.replacedCaller=fn.caller===replacement;delete fp.arguments;result.deletedArguments=fn.arguments===undefined;return result;
}
function run(){
 let arrayLike=loggedArray(),thisArg='THIS';
 if(mode==='missing'||mode==='undefined')arrayLike=undefined;else if(mode==='null')arrayLike=null;
 else if(mode==='dense')arrayLike=[1,2,3];else if(mode==='boxedString')arrayLike=new String('ab');else if(mode==='callPropertyIgnored')getter(fn,'call',read('target.call',61,true));else if(mode==='sparse')arrayLike=[,7,,];
 else if(mode==='inherited')arrayLike=Object.assign(Object.create({1:9}),{length:3,0:8});
 else if(mode==='string')arrayLike='abc';else if(mode==='number')arrayLike=2;else if(mode==='boolean')arrayLike=true;else if(mode==='symbol')arrayLike=Symbol('test');
 else if(mode==='negative')arrayLike=loggedArray(-1);else if(mode==='nan')arrayLike=loggedArray(NaN);else if(mode==='fraction')arrayLike=loggedArray(1.9);else if(mode==='lengthString')arrayLike=loggedArray('2.9');
 else if(mode==='lengthUndefined'){arrayLike=loggedArray();getter(arrayLike,'length',read('length',undefined))}else if(mode==='lengthSymbol')arrayLike=loggedArray(Symbol());
 else if(mode==='conversion'||mode==='conversionThrows'||mode==='primitiveHint'){
  const length=mode==='primitiveHint'?{[Symbol.toPrimitive](hint){log.push('convert:'+hint);return 2}}:{valueOf:read('valueOf',mode==='conversionThrows'?29:{},mode==='conversionThrows'),toString:read('toString','2')};arrayLike=loggedArray(length);
 }
 else if(mode==='lengthThrows')getter(arrayLike,'length',read('length',23,true));else if(mode==='indexThrows')getter(arrayLike,'1',read('1',31,true));
 else if(mode==='hugeLength'||mode==='capacityExceeded'){arrayLike=loggedArray(mode==='capacityExceeded'?134217729:9007199254740992);getter(arrayLike,'0',read('0',37,true))}
 else if(mode==='iteratorIgnored')getter(arrayLike,Symbol.iterator,read('iterator',41,true));
 else if(mode==='proxy')arrayLike=new Proxy(arrayLike,{get(target,key,receiver){log.push('get:'+String(key));return Reflect.get(target,key,receiver)},has(){log.push('has');throw true}});
 else if(mode==='badTarget'||mode==='badTargetRevokedList'){fn={};if(mode==='badTargetRevokedList'){const p=Proxy.revocable(arrayLike,{});p.revoke();arrayLike=p.proxy}}
 else if(mode==='revokedList'){const p=Proxy.revocable(arrayLike,{});p.revoke();arrayLike=p.proxy}
 else if(mode==='revokedTarget'){const p=Proxy.revocable(fn,{});p.revoke();fn=p.proxy}
 else if(mode==='classTarget')fn=class C{};
 else if(mode==='proxyTarget')fn=new Proxy(fn,{apply(target,receiver,args){log.push('applyTrap');return Reflect.apply(target,receiver,args)}});
 else if(mode==='mutateLength'){arrayLike={length:2};getter(arrayLike,'0',function(){log.push('0');this.length=0;this[1]=17;return 12})}
 else if(mode==='reentrant')getter(arrayLike,'0',()=>{log.push('0');Reflect.apply(apply,fn,['INNER',[9]]);return 4});
 else if(mode==='prototypeReplaced'){fp.apply=function(){log.push('replacement');return 91};out.dynamic=fn.apply(thisArg,arrayLike)}
 else if(mode==='thisUndefined')thisArg=undefined;else if(mode==='thisNull')thisArg=null;else if(mode==='thisNumber')thisArg=19;else if(mode==='thisObject')thisArg={marker:5};
 else if(mode==='bound')fn=fn.bind('BOUND',8);else if(mode==='callableList'){arrayLike=function arraylike(a,b){};arrayLike[0]=6;arrayLike[1]=7}
 try{out.result=Reflect.apply(apply,fn,mode==='missing'?[]:[thisArg,arrayLike])}catch(e){out.error=failure(e)}out.events=log;return out;
}
mode==='metadata'?metadata():run();
`;
export function oracle(mode){return vm.runInNewContext(source,{mode});}
function boundaryEvidence(){return vm.runInNewContext(String.raw`
const keys=['caller','arguments'],descriptors=keys.map(k=>Object.getOwnPropertyDescriptor(Function.prototype,k));const accessors=descriptors.flatMap(d=>[d.get,d.set]);const poison=(function(){'use strict';return Object.getOwnPropertyDescriptor(arguments,'callee').get})();
const inspect=f=>({name:f.name,length:f.length,extensible:Object.isExtensible(f),ownKeys:Reflect.ownKeys(f),nameDescriptor:Object.getOwnPropertyDescriptor(f,'name'),lengthDescriptor:Object.getOwnPropertyDescriptor(f,'length'),source:Function.prototype.toString.call(f)});
const sloppy=Function(),strict=function(){'use strict'};const result={accessors:accessors.map(inspect),identity:accessors.map(a=>accessors.map(b=>a===b)),strictArgumentsPoison:inspect(poison),poisonIdentity:accessors.map(a=>a===poison),sloppy:[],strict:[],sloppyOwnKeys:Reflect.ownKeys(sloppy),sloppyOwnDescriptors:Reflect.ownKeys(sloppy).map(key=>({key,descriptor:Object.getOwnPropertyDescriptor(sloppy,key)}))};
for(const [name,value]of [['sloppy',sloppy],['strict',strict]])for(const accessor of accessors){try{const v=Reflect.apply(accessor,value,[123]);result[name].push(v===undefined?'#undefined':v)}catch(e){result[name].push('#'+e.name)}}
result;`);}
function diff(expected,actual,prefix=''){const out=[];if(expected&&actual&&typeof expected==='object'&&typeof actual==='object'){for(const k of new Set([...Object.keys(expected),...Object.keys(actual)]))out.push(...diff(expected[k],actual[k],prefix+'/'+k))}else if(!Object.is(expected,actual))out.push({path:prefix,expected,actual});return out;}
const args=process.argv.slice(2),opts=new Map();for(let i=0;i<args.length;i+=2){if(!['--probe','--output'].includes(args[i])||!args[i+1]||opts.has(args[i]))throw Error('Usage: compare-function-apply.mjs [--probe binary] --output new-directory');opts.set(args[i],args[i+1]);}
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 if(!opts.has('--output'))throw Error('--output is required');const output=path.resolve(opts.get('--output'));if(fs.existsSync(output))throw Error('Evidence already exists');fs.mkdirSync(output,{recursive:true});
 const expected=modes.map(mode=>({mode,value:oracle(mode)}));fs.writeFileSync(path.join(output,'expected.json'),JSON.stringify(expected,null,2)+'\n');fs.writeFileSync(path.join(output,'node-legacy-boundaries.json'),JSON.stringify(boundaryEvidence(),null,2)+'\n');
 const summary={mode:opts.has('--probe')?'native-differential':'oracle-only',binaryExecuted:false,cases:modes.length,node:process.version,scope:'Actual apply order and strict/native restricted accessors; zero-argument Function private mode requires root override; general sloppy stack introspection and dynamic compilation remain pending'};
 if(opts.has('--probe')){const binary=path.resolve(opts.get('--probe')),before=createHash('sha256').update(fs.readFileSync(binary)).digest('hex');const actual=[],differences=[];for(const {mode,value}of expected){const r=spawnSync(binary,[mode],{encoding:'utf8',maxBuffer:1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});summary.binaryExecuted||=r.pid!==undefined;let decoded;try{decoded=JSON.parse(r.stdout)}catch{decoded={parseError:true}}actual.push({mode,value:decoded,status:r.status,stderr:r.stderr});differences.push(...diff(value,decoded,mode));if(r.status!==0)differences.push({mode,status:r.status,error:r.error?.message})}
 const lifetime=spawnSync(binary,['--lifetime'],{encoding:'utf8',maxBuffer:1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2)+'\n');fs.writeFileSync(path.join(output,'differences.json'),JSON.stringify(differences,null,2)+'\n');fs.writeFileSync(path.join(output,'lifetime.txt'),String(lifetime.stdout??'')+String(lifetime.stderr??''));Object.assign(summary,{binary,binaryBeforeSHA256:before,binaryAfterSHA256:createHash('sha256').update(fs.readFileSync(binary)).digest('hex'),failedFields:differences.length,lifetimePassed:lifetime.status===0&&/3 passed/.test(lifetime.stdout)});process.exitCode=differences.length||!summary.lifetimePassed?1:0;}
 fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2)+'\n');console.log(JSON.stringify(summary));
}
