import assert from 'node:assert/strict';
import fs from 'node:fs';
import {execFileSync} from 'node:child_process';
const executable=process.argv[2];
if(!executable)throw Error('Usage: node --expose-gc compare-array-shift.mjs <probe>');
if(typeof globalThis.gc!=='function')throw Error('Run the oracle with --expose-gc to execute its collection scenarios');
const encode=v=>v===undefined?'@undefined':typeof v==='function'?'@function':typeof v==='number'?(Number.isNaN(v)?'@NaN':v===Infinity?'@Infinity':v===-Infinity?'@-Infinity':Object.is(v,-0)?'@-0':v):Array.isArray(v)?Array.from(v,encode):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
function snapshot(o){if((typeof o!=='object'||o===null)&&typeof o!=='function')return encode(o);return Object.fromEntries(Object.getOwnPropertyNames(o).map(key=>{const d=Object.getOwnPropertyDescriptor(o,key),entry={enumerable:d.enumerable,configurable:d.configurable};if('value'in d){entry.value=encode(d.value);entry.writable=d.writable}else{entry.get=typeof d.get==='function';entry.set=typeof d.set==='function'}return[key,entry]}));}
function error(fn){try{fn();return ''}catch(e){return e.name}}
function access(target,key,o,events,value,read=0,write=0){
    let entered=false;
    const descriptor={configurable:true,enumerable:true,get(){
        events.push(['get:'+key,this===o]);
        if(read===1)throw RangeError('getter');
        if(read===2)delete o[2];if(read===3)o[4]=99;if(read===4)o.length=0;
        if(read===5&&!entered){entered=true;const result=Array.prototype.shift.call(o);events.push(['inner',result])}
        if(read===6)Object.setPrototypeOf(o,{1:88});
        if(read===7){events.push(['gc']);globalThis.gc?.()}
        return value;
    }};
    if(write!==-1)descriptor.set=function(v){
        events.push(['set:'+key,v,this===o]);
        if(write===1)throw RangeError('setter');
        if(write===2&&!entered){entered=true;const result=Array.prototype.shift.call(o);events.push(['inner',result])}
    };
    Object.defineProperty(target,key,descriptor);
}
function property(o,key,value,writable=true,configurable=true){Object.defineProperty(o,key,{value,writable,enumerable:true,configurable})}
function execute(kind,mode,o,events){let result;const failure=error(()=>result=Array.prototype.shift.call(o,'ignored'));return {kind,mode,error:failure,result:encode(result),props:snapshot(o),events:encode(events)}}
function scenario(mode,array){const o=array?[]:{},events=[];o.length=4;o[0]=-0;o[2]=NaN;o[3]=undefined;
if(mode===1){const p=Object.create(Object.getPrototypeOf(o));p[1]=9;Object.setPrototypeOf(o,p)}
if(mode===2){const p=Object.create(Object.getPrototypeOf(o));delete o[0];access(p,'0',o,events,12);Object.setPrototypeOf(o,p)}
if(mode===3)access(o,'0',o,events,12);
if(mode===4)access(o,'1',o,events,13,2);
if(mode===5)access(o,'1',o,events,13,3);
if(mode===6)access(o,'1',o,events,13,4);
if(mode===7)access(o,'2',o,events,14,1);
if(mode===8)access(o,'0',o,events,12,0,1);
if(mode===9)access(o,'1',o,events,13,0,-1);
if(mode===10)property(o,'1',9,false);
if(mode===11)Object.defineProperty(o,'3',{configurable:false});
if(mode===12)Object.defineProperty(o,'0',{configurable:false});
if(mode===13)Object.defineProperty(o,'length',{writable:false});
if(mode===14)Object.preventExtensions(o);
if(mode===15){const p=Object.create(Object.getPrototypeOf(o));access(p,'1',o,events,13);Object.setPrototypeOf(o,p)}
if(mode===16){const p=Object.create(Object.getPrototypeOf(o));property(p,'1',9,false);Object.setPrototypeOf(o,p)}
if(mode===17)access(o,'length',o,events,4);
if(mode===18)access(o,'length',o,events,4,1);
if(mode===19)access(o,'length',o,events,4,0,1);
if(mode===20)access(o,'0',o,events,12,5);
if(mode===21)access(o,'0',o,events,12,0,2);
if(mode===22)access(o,'0',o,events,12,6);
if(mode===23)access(o,'2',o,events,14,7);
return execute(array?'array':'object',mode,o,events)}
function sparse(mask,array,inherited){const o=array?[]:{};o.length=6;const p=Object.create(Object.getPrototypeOf(o));if(inherited)Object.setPrototypeOf(o,p);for(let i=0;i<6;i++)if(mask&(1<<i))(inherited&&i%2?p:o)[i]=i===0?-0:i===2?NaN:i===3?undefined:i;return execute((array?'sparse-array':'sparse-object')+(inherited?'-inherited':''),mask,o,[])}
function lengthCase(mode){const o={0:-0,1:7,2:8},events=[],lengths=[undefined,null,false,true,0,-0,-1,-Infinity,NaN,0.9,1.9,3.9,' 3.8 ','invalid'];if(mode<lengths.length)o.length=lengths[mode];else if(mode<18)o.length={tag:'length',valueOf(){events.push(['valueOf']);if(mode===15)throw RangeError('length');if(mode===16)o[1]=42;if(mode===17){o.length=0;return 3.8}return 3.8}};else{o.length=mode===18?4294967295:mode===19?9007199254740991:Infinity;access(o,'0',o,events,0,1)}return execute('length',mode,o,events)}
function primitive(mode){return execute('primitive',mode,[undefined,null,false,true,0,-0,42,NaN,'','a','ab','😀'][mode],[])}
function meta(){const fn=Array.prototype.shift;return {kind:'metadata',name:encode(fn.name),length:encode(fn.length),own:snapshot(fn),construct:error(()=>Reflect.construct(fn,[])),prototypeDescriptor:snapshot(Array.prototype).shift}}
const expected=[];
for(const array of [false,true])for(let mode=0;mode<24;mode++)if(!array||mode<17||mode>19)expected.push(scenario(mode,array));
for(const array of [false,true])for(const inherited of [false,true])for(let mask=0;mask<64;mask++)expected.push(sparse(mask,array,inherited));
for(let mode=0;mode<21;mode++)expected.push(lengthCase(mode));
for(let mode=0;mode<12;mode++)expected.push(primitive(mode));
expected.push(meta());
const actual=JSON.parse(execFileSync(executable,{encoding:'utf8',maxBuffer:16*1024*1024}));
let comparisons=0;
function compare(a,b,path){comparisons++;if(a===null||b===null||typeof a!=='object'||typeof b!=='object'){assert.deepEqual(a,b,path);return}assert.deepEqual(Object.keys(a),Object.keys(b),path+' keys');for(const key of Object.keys(b))compare(a[key],b[key],path+'.'+key)}
try{assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`${i}:${expected[i].kind}:${expected[i].mode}`)}catch(error){fs.writeFileSync('array-shift-failure.actual.json',JSON.stringify(actual,null,2));fs.writeFileSync('array-shift-failure.expected.json',JSON.stringify(expected,null,2));throw error}
console.log(`Array shift parity: ${expected.length} scenarios / ${comparisons} comparisons passed (${process.version}).`);
