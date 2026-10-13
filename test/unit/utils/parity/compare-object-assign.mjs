import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
const enc=v=>v===undefined?'undefined':typeof v==='symbol'?{symbol:v.description}:typeof v==='function'?'function':v===null||typeof v!=='object'?v:Reflect.ownKeys(v).map(k=>{const d=Object.getOwnPropertyDescriptor(v,k);return [enc(k),!!d.enumerable,!!d.writable,!!d.configurable,'value'in d?enc(d.value):'accessor']});
function run(mode,targetKind){const target=targetKind===0?{}:targetKind===1?[]:targetKind===2?'xy':targetKind===3?null:undefined,source={},trace=[],symbol=Symbol('token');
Object.defineProperty(source,'9',{value:9,writable:true,enumerable:true,configurable:true});Object.defineProperty(source,'2',{value:2,writable:true,enumerable:true,configurable:true});
for(const [k,v,e]of [['a',11,true],['b',12,true],['hidden',13,false],[symbol,14,true]])Object.defineProperty(source,k,{value:v,writable:true,enumerable:e,configurable:true});
if(mode>=1&&mode<=7)Object.defineProperty(source,'a',{get(){trace.push('get-a');if(mode===1)delete source.b;if(mode===2)Object.defineProperty(source,'b',{enumerable:false});if(mode===3)source.new=15;if(mode===4)Object.defineProperty(source,symbol,{enumerable:false});if(mode===5)source.b=17;if(mode===6)throw RangeError('getter');if(mode===7)source.a=18;return 16;},enumerable:true,configurable:true});
if(target!==null&&typeof target==='object'){
if(mode===8)Object.defineProperty(target,'b',{value:0,writable:false,enumerable:true,configurable:true});if(mode===9)Object.preventExtensions(target);
if(mode===10){const proto={};Object.defineProperty(proto,'a',{set(v){trace.push(v)}});Object.setPrototypeOf(target,proto);}
if(mode===11)Object.defineProperty(target,'b',{set(){throw RangeError('setter')}});}
let result,error='';try{const args=[target,source];if(mode===12)args.push(null,undefined,false,7,Symbol('source'),'ab');result=Object.assign(...args);}catch(e){error=e.name;}
return {mode,targetKind,error,resultIdentity:target!==null&&typeof target==='object'&&result===target,target:enc(target),result:enc(result),trace:enc(trace)};}
const expected=[];for(let kind=0;kind<5;kind++)for(let mode=0;mode<13;mode++)expected.push(run(mode,kind));
const probe=spawnSync(process.argv[2],[],{encoding:'utf8',maxBuffer:32*1024*1024});assert.equal(probe.status,0,probe.stderr);const actual=probe.stdout.trim().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;i++)assert.deepEqual(actual[i],expected[i],`case ${i}`);console.log(`Object.assign parity: ${expected.length} actual Node cases passed (supported native boxing range).`);
