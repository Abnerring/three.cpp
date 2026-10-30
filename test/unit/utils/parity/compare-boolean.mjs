import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';
const IntrinsicBoolean = Boolean, boolPrototype = Boolean.prototype;
const intrinsicValueOf = boolPrototype.valueOf;
const booleanValue = v => Reflect.apply(intrinsicValueOf, v, []);
const normal = v => v === undefined ? '#undefined' : typeof v === 'symbol' ? '#symbol' : typeof v === 'function' ? '#function' : v !== null && typeof v === 'object' ? '#object' : typeof v === 'number' && Number.isNaN(v) ? '#NaN' : v === Infinity ? '#Infinity' : v === -Infinity ? '#-Infinity' : Object.is(v,-0) ? '#-0' : v;
function attempt(fn) { try { return normal(fn()); } catch (e) { return e instanceof TypeError ? '#TypeError' : e instanceof RangeError ? '#RangeError' : '#Error'; } }
function decode(v,events) {
  if (v === null || typeof v !== 'object') return v;
  switch (v.tag) {
    case 'undefined': return undefined;
    case 'NaN': return NaN;
    case 'Infinity': return Infinity;
    case '-0': return -0;
    case 'symbol': return Symbol('fixture');
    case 'array': return [];
    case 'boxed': return new IntrinsicBoolean(v.value);
    case 'prototype': return boolPrototype;
    case 'fake': return Object.create(boolPrototype);
    case 'numberBox': return new Number(0);
    case 'object': { const o={}; for(const k of ['valueOf','toString'])Object.defineProperty(o,k,{value(){events.push(k);throw new TypeError('must not coerce object for ToBoolean');},writable:true,enumerable:true,configurable:true});Object.defineProperty(o,Symbol.toPrimitive,{get(){events.push('toPrimitive.get');throw new TypeError('must not read coercion hook');}});return o; }
    case 'proxy': case 'revoked': { const target=v.booleanTarget?new IntrinsicBoolean(false):{}, handler={};for(const k of ['get','getPrototypeOf'])handler[k]=function(){events.push(k);throw new TypeError('unexpected Proxy trap');};const r=Proxy.revocable(target,handler);if(v.tag==='revoked')r.revoke();return r.proxy; }
    default:return {};
  }
}
function describe(o){return Object.getOwnPropertyNames(o).map(k=>{const d=Object.getOwnPropertyDescriptor(o,k);return[k,normal(d.value),!!d.writable,!!d.enumerable,!!d.configurable];});}
function metadata(){const c=IntrinsicBoolean,p=boolPrototype,d=Object.getOwnPropertyDescriptor(globalThis,'Boolean');return{constructor:describe(c),prototype:describe(p),name:c.name,length:c.length,source:Function.prototype.toString.call(c),functionPrototype:Object.getPrototypeOf(c)===Function.prototype,prototypeValue:booleanValue(p),protoIsObjectPrototype:Object.getPrototypeOf(p)===Object.prototype,globalIdentity:globalThis.Boolean===c,globalDescriptor:[d.writable,d.enumerable,d.configurable],valueOfSource:Function.prototype.toString.call(p.valueOf),toStringSource:Function.prototype.toString.call(p.toString),methodsConstructible:[attempt(()=>Reflect.construct(Object,[],p.valueOf))!=='#TypeError',attempt(()=>Reflect.construct(Object,[],p.toString))!=='#TypeError']};}
function snapshot(v){const out={type:v!==null&&(typeof v==='object'||typeof v==='function')?'object':typeof v==='boolean'?'boolean':'other',value:normal(v),truthy:!!v};if(v!==null&&(typeof v==='object'||typeof v==='function')){out.booleanValue=attempt(()=>booleanValue(v));out.own=Object.getOwnPropertyNames(v);out.defaultPrototype=Object.getPrototypeOf(v)===boolPrototype;out.tag=attempt(()=>Object.prototype.toString.call(v));}return out;}
export function oracle(r){
  if(r.kind==='metadata')return metadata();
  if(r.kind==='methodMetadata')return{valueOf:describe(boolPrototype.valueOf),toString:describe(boolPrototype.toString)};
  const events=[],value=decode(r.value,events),ctor=IntrinsicBoolean,prototype=boolPrototype;let result;
  if(r.kind==='convert'){let raw;result=attempt(()=>raw=r.construct?new ctor(value):Reflect.apply(ctor,decode(r.receiver,events),[value]));return{result,snapshot:result==='#TypeError'?null:snapshot(raw),events};}
  if(r.kind==='method'){result=attempt(()=>Reflect.apply(prototype[r.method],value,[123,Symbol('ignored')]));return{result,events};}
  if(r.kind==='newTarget'){
    let target=function Derived(){},custom={marker:true};const slot={};
    if(r.mode==='object')target.prototype=custom;
    else if(r.mode==='null')target.prototype=null;
    else if(r.mode==='number')target.prototype=3;
    else if(r.mode==='bound')target=target.bind(null);
    else if(r.mode==='nonConstructor')target={};
    else if(r.mode==='boundRevoked'){const revocable=Proxy.revocable(target,{});target=revocable.proxy.bind(null);revocable.revoke();}
    else {const handler={get(t,k,receiver){if(k==='prototype'){events.push('prototype.get');if(r.throw)throw new TypeError('prototype getter');if(r.reenter){new IntrinsicBoolean(false);globalThis.gc?.();events.push('reentered');}if(r.revoke)slot.revoke();return r.objectPrototype?custom:null;}return Reflect.get(t,k,receiver);}};if(r.mode==='proxyBound')target=target.bind(null);const revocable=Proxy.revocable(target,handler);target=revocable.proxy;slot.revoke=revocable.revoke;}
    let raw;result=attempt(()=>raw=Reflect.construct(ctor,[value],target));return{result,booleanValue:raw&&typeof raw==='object'?booleanValue(raw):null,defaultPrototype:!!raw&&Object.getPrototypeOf(raw)===prototype,customPrototype:!!raw&&Object.getPrototypeOf(raw)===custom,events};
  }
  if(r.kind==='boxing'){let raw;result=attempt(()=>{switch(r.operation){case'Object':raw=Object(value);break;case'newObject':raw=new Object(value);break;case'valueOf':raw=Object.prototype.valueOf.call(value);break;case'assign':raw=Object.assign(value,{extra:7});break;case'getPrototypeOf':return Object.getPrototypeOf(value)===prototype;case'propertyIsEnumerable':return Object.prototype.propertyIsEnumerable.call(value,'x');case'keys':return Object.keys(value).length;case'getOwnPropertyNames':return Object.getOwnPropertyNames(value).length;case'getOwnPropertyDescriptors':return Object.getOwnPropertyNames(Object.getOwnPropertyDescriptors(value)).length;case'primitiveValueOf':return value.valueOf();case'primitiveToString':return value.toString();case'legacyProto':return value.__proto__===prototype;case'toNumber':return Number(value);case'toString':return String(value);}return raw;});return{result,snapshot:raw!==null&&(typeof raw==='object'||typeof raw==='function')?snapshot(raw):null,extra:raw!==null&&(typeof raw==='object'||typeof raw==='function')?normal(raw.extra):null,events};}
  if(r.kind==='primitiveAccess'){
    const key=r.symbol?Symbol('field'):'field',original=Object.getOwnPropertyDescriptor(prototype,key);
    Object.defineProperty(prototype,key,{configurable:true,get(){events.push(['get',typeof this==='boolean',normal(this)]);return r.result;},set(v){events.push(['set',typeof this==='boolean',normal(this),normal(v)]);}});
    result=attempt(()=>{if(r.set){value[key]=9;return'assigned';}return value[key];});
    if(original)Object.defineProperty(prototype,key,original);else delete prototype[key];return{result,events};
  }
  if(r.kind==='tag'){
    const key=Symbol.toStringTag,original=Object.getOwnPropertyDescriptor(prototype,key);
    Object.defineProperty(prototype,key,{configurable:true,get(){events.push(['tag',typeof this==='object',booleanValue(this)]);return r.tagValue;}});
    result=attempt(()=>Object.prototype.toString.call(value));
    if(original)Object.defineProperty(prototype,key,original);else delete prototype[key];return{result,events};
  }
  if(r.kind==='identity'){let raw;result=attempt(()=>raw=r.operation==='construct'?new ctor(value):r.operation==='valueOf'?Object.prototype.valueOf.call(value):Object(value));return{result,sameInput:raw===value,booleanValue:raw!==null&&typeof raw==='object'?attempt(()=>booleanValue(raw)):null,secondBoxSame:typeof value==='boolean'?raw===Object(value):null,events};}
  if(r.kind==='json'){const changed=r.throwValueOf&&value!==null&&typeof value==='object',original=changed?Object.getOwnPropertyDescriptor(value,'valueOf'):undefined;if(changed)Object.defineProperty(value,'valueOf',{value(){events.push('valueOf');throw new TypeError('JSON BooleanData must not coerce');},writable:true,enumerable:true,configurable:true});result=attempt(()=>JSON.stringify(value));if(changed){if(original)Object.defineProperty(value,'valueOf',original);else delete value.valueOf;}return{result,events};}
  if(r.kind==='descriptorMutation'){result=attempt(()=>{if(r.operation==='prototypeAssign'){ctor.prototype=null;return true;}if(r.operation==='prototypeDelete')return Reflect.deleteProperty(ctor,'prototype');if(r.operation==='methodConstruct')return Reflect.construct(prototype[r.method],[]);});return{result,stablePrototype:ctor.prototype===prototype,events};}
  if(r.kind==='mutations'){
    const gd=Object.getOwnPropertyDescriptor(globalThis,'Boolean'),pd=Object.getOwnPropertyDescriptor(prototype,'constructor');globalThis.Boolean=null;prototype.constructor=null;
    result=ctor(value);const boxed=Object(value),stable=ctor===IntrinsicBoolean&&Object.getPrototypeOf(boxed)===prototype;
    Object.defineProperty(globalThis,'Boolean',gd);Object.defineProperty(prototype,'constructor',pd);return{result,stable,value:booleanValue(boxed),events};
  }
  throw new Error('unknown fixture kind');
}
export function cases(){
 const out=[{kind:'metadata'}];
 const values=[false,true,null,0,1,-1,'','false','0','three.js',{tag:'undefined'},{tag:'NaN'},{tag:'Infinity'},{tag:'-0'},{tag:'symbol'},{tag:'array'},{tag:'boxed',value:false},{tag:'boxed',value:true},{tag:'prototype'},{tag:'fake'},{tag:'numberBox'},{tag:'object'},{tag:'proxy'},{tag:'proxy',booleanTarget:true},{tag:'revoked'},{tag:'revoked',booleanTarget:true}];
 for(const value of values)for(const construct of [false,true])for(const receiver of [null,true,{tag:'revoked'}])out.push({kind:'convert',value,construct,receiver});
 for(const method of ['valueOf','toString'])for(const value of values)out.push({kind:'method',method,value});
 for(const mode of ['object','null','number','bound','proxy','proxyBound','nonConstructor'])for(const value of values)out.push({kind:'newTarget',mode,value});
 for(const mode of ['proxy','proxyBound'])for(const value of [false,true,{tag:'object'}])for(const flags of [{revoke:true},{revoke:true,objectPrototype:true},{throw:true},{reenter:true},{reenter:true,revoke:true},{reenter:true,revoke:true,objectPrototype:true},{objectPrototype:true}])out.push({kind:'newTarget',mode,value,...flags});
 const boxedValues=[false,true,{tag:'boxed',value:false},{tag:'boxed',value:true},{tag:'prototype'},0,5,'','abc',null,{tag:'undefined'}];
 for(const value of boxedValues)for(const operation of ['Object','newObject','valueOf','assign','getPrototypeOf','propertyIsEnumerable','keys','getOwnPropertyNames','getOwnPropertyDescriptors','primitiveValueOf','primitiveToString','legacyProto','toNumber','toString'])out.push({kind:'boxing',value,operation});
 for(const value of [false,true])for(const symbol of [false,true])for(const set of [false,true])for(const result of [null,42,'text'])out.push({kind:'primitiveAccess',value,symbol,set,result});
 for(const value of [false,true,{tag:'boxed',value:false},{tag:'boxed',value:true},{tag:'prototype'}])for(const tagValue of ['CustomBoolean',5,null])out.push({kind:'tag',value,tagValue});
 for(const value of [false,true])out.push({kind:'mutations',value});
 // The original 628 cases stay unchanged as a prefix.
 out.push({kind:'methodMetadata'});
 for(const value of values)out.push({kind:'newTarget',mode:'boundRevoked',value});
 for(const value of [false,true,{tag:'boxed',value:false},{tag:'boxed',value:true},{tag:'prototype'},{tag:'fake'}])for(const throwValueOf of [false,true])out.push({kind:'json',value,throwValueOf});
 for(const operation of ['prototypeAssign','prototypeDelete'])out.push({kind:'descriptorMutation',operation});
 for(const method of ['valueOf','toString'])out.push({kind:'descriptorMutation',operation:'methodConstruct',method});
 for(const value of [false,true,{tag:'boxed',value:false},{tag:'boxed',value:true},{tag:'prototype'},{tag:'fake'}])for(const operation of ['construct','ToObject','valueOf','Object'])out.push({kind:'identity',value,operation});
 return out;
}
function compare(expected,actual,where='$',out=[]){if(expected===null||actual===null||typeof expected!=='object'||typeof actual!=='object'){if(!Object.is(expected,actual))out.push({path:where,expected,actual});return out;}if(Array.isArray(expected)!==Array.isArray(actual)){out.push({path:where,expected,actual});return out;}const ek=Object.keys(expected),ak=Object.keys(actual);if(JSON.stringify(ek)!==JSON.stringify(ak))out.push({path:where+'.keys',expected:ek,actual:ak});for(const k of ek)if(Object.hasOwn(actual,k))compare(expected[k],actual[k],where+'.'+k,out);return out;}
const invoked=process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url), options=new Map();
if(invoked){
 const allowed=new Set(['--probe','--output','--input']),args=process.argv.slice(2);
 for(let i=0;i<args.length;i+=2){const key=args[i],value=args[i+1];if(!allowed.has(key)||options.has(key)||!value||value.startsWith('--'))throw new Error('Usage: compare-boolean.mjs [--probe binary] [--input cases.json] --output new-directory');options.set(key,value);}
 if(!options.has('--output'))throw new Error('--output is required; use a new evidence directory');
}
const option=k=>options.get(k);
if(process.argv[1]&&path.resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 const destination=path.resolve(option('--output'));if(fs.existsSync(destination))throw new Error('Evidence directory already exists: '+destination);fs.mkdirSync(destination,{recursive:true});
 const input=option('--input')?JSON.parse(fs.readFileSync(option('--input'),'utf8')):cases();
 const expected=input.map(oracle);fs.writeFileSync(path.join(destination,'input.json'),JSON.stringify(input,null,2)+'\n');fs.writeFileSync(path.join(destination,'expected.json'),JSON.stringify(expected,null,2)+'\n');
 const binary=option('--probe');let status={mode:binary?'native-differential':'oracle-only',binaryExecuted:false,cases:input.length,node:process.version,engine:process.versions.v8,normalization:'only JSON-unsupported primitive values, object/function placeholders; no algorithm/identity/field suppression',oracle:'ECMAScript native Boolean/Object/Reflect/Proxy'};
 if(binary){const before=createHash('sha256').update(fs.readFileSync(binary)).digest('hex');const child=spawnSync(binary,[],{input:JSON.stringify(input),encoding:'utf8',maxBuffer:128*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});fs.writeFileSync(path.join(destination,'actual.raw.json'),child.stdout??'');fs.writeFileSync(path.join(destination,'stderr.txt'),child.stderr??'');const after=createHash('sha256').update(fs.readFileSync(binary)).digest('hex');status={...status,binary,binaryExecuted:child.pid!==undefined,binaryBeforeSHA256:before,binaryAfterSHA256:after,exitCode:child.status,signal:child.signal,error:child.error?.message};if(child.status===0){const actual=JSON.parse(child.stdout);const differences=compare(expected,actual);fs.writeFileSync(path.join(destination,'actual.json'),JSON.stringify(actual,null,2)+'\n');fs.writeFileSync(path.join(destination,'differences.json'),JSON.stringify(differences,null,2)+'\n');status.fieldsFailed=differences.length;status.passed=expected.filter((e,i)=>compare(e,actual[i]).length===0).length;process.exitCode=differences.length?1:0;}else process.exitCode=1;}
 fs.writeFileSync(path.join(destination,'summary.json'),JSON.stringify(status,null,2)+'\n');console.log(JSON.stringify(status));
}
