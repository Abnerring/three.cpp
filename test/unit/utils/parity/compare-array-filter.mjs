import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
const probe=process.argv[2];
if(!probe)throw Error('Usage: node --expose-gc compare-array-filter.mjs <probe>');
if(typeof globalThis.gc!=='function')throw Error('Actual GC checkpoints require --expose-gc');
const encode=v=>v===undefined?'@undefined':v===null?null:typeof v==='function'?'@function':typeof v==='number'?(Number.isNaN(v)?'@NaN':Object.is(v,-0)?'@-0':v):Array.isArray(v)?Array.from({length:v.length},(_,i)=>i in v?encode(v[i]):'@hole'):typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>{const d=Object.getOwnPropertyDescriptor(v,k);return [k,'value' in d?encode(d.value):'@accessor']})):v;
const accessor=(o,key,get)=>Object.defineProperty(o,key,{get,enumerable:true,configurable:true});
function run(mask,isArray,inherited,mode,species,method='filter'){
 const receiver=isArray?[]:{},events=[],thisArg={},target={},p=Object.create(Object.getPrototypeOf(receiver));Object.setPrototypeOf(receiver,p);receiver.length=6;
 for(let i=0;i<6;i++)if(mask&(1<<i))(inherited&&i%2?p:receiver)[i]=i===0?-0:i===2?undefined:i===4?NaN:i;
 let callback=function(value,i,o){events.push(['callback',value,i,this===thisArg,o===receiver]);
  if(i===0){if(mode===3)delete o[2];if(mode===4)o[1]=41;if(mode===5)o.length=1;if(mode===6)o[6]=66;if(mode===7)Object.setPrototypeOf(o,Object.create(null));if(mode===8){const p=Object.create(Object.getPrototypeOf(o));p[1]=88;Object.setPrototypeOf(o,p)}if(mode===11){globalThis.gc();events.push(['gc'])}if(mode===12){const inner=Array.prototype.filter.call(o,(_v,k)=>k===0);events.push(['inner',inner])}}
  if(mode===9&&i===2)throw RangeError('callback');if(mode===1)return i%2===0;if(mode===2)return false;if(mode===10)return {};if(mode===13)return '';return true;
 };
 if(mode===14)callback=17;
 if(!isArray)accessor(receiver,'length',function(){events.push(['length',this===receiver]);if(mode===15)throw RangeError('length');return 6});
 if(species){const constructor={};accessor(receiver,'constructor',function(){events.push(['constructor',this===receiver]);if(species===1)throw RangeError('constructor');if(species===2)return null;if(species===3)return 17;return constructor});
  function Species(length){events.push(['new',length]);if(species===7)throw RangeError('new');if(species===10)receiver[1]=91;return target}
  accessor(constructor,Symbol.species,function(){events.push(['species',this===constructor]);if(species===4)throw RangeError('species');if(species===5)return null;if(species===6)return 17;if(species===11)return undefined;return Species});
  if(species===8)Object.preventExtensions(target);
  if(species===9)Object.defineProperty(target,'1',{value:'locked',configurable:false,writable:false,enumerable:true});
 }
 let result,error='';
 try{if(method==='slice')result=Array.prototype.slice.call(receiver,0,4);else if(method==='splice')result=Array.prototype.splice.call(receiver,0,4);else if(method==='concat')result=Array.prototype.concat.call(receiver,[8,9]);else result=Array.prototype[method].call(receiver,callback,thisArg)}catch(e){error=e.name}
 return {mask,array:isArray,inherited,mode,species,method,error,result:encode(result),target:encode(target),trace:encode(events)};
}
const expected=[];
for(const isArray of [false,true])for(const inherited of [false,true])for(let mask=0;mask<64;mask++)for(let mode=0;mode<3;mode++)expected.push(run(mask,isArray,inherited,mode,0));
for(const isArray of [false,true])for(let mode=3;mode<16;mode++)expected.push(run(63,isArray,true,mode,0));
for(const method of ['filter','map','slice','splice','concat'])for(const isArray of [false,true])for(let species=1;species<12;species++)expected.push(run(47,isArray,false,0,species,method));
expected.push(run(63,true,false,14,4));
const actual=JSON.parse(execFileSync(probe,[],{encoding:'utf8',maxBuffer:64*1024*1024}));
let comparisons=0;
function compare(a,b,p){comparisons++;if(a===null||b===null||typeof a!=='object'||typeof b!=='object'){assert.deepEqual(a,b,p);return}assert.deepEqual(Object.keys(a),Object.keys(b),p+' keys');for(const key of Object.keys(b))compare(a[key],b[key],p+'.'+key)}
try{assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],String(i));}catch(error){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-array-filter-failure-'));fs.writeFileSync(path.join(dir,'actual.json'),JSON.stringify(actual,null,2));fs.writeFileSync(path.join(dir,'expected.json'),JSON.stringify(expected,null,2));console.error(dir);throw error}
console.log(`Array filter/species: ${expected.length} scenarios / ${comparisons} comparisons passed (${process.version}).`);
