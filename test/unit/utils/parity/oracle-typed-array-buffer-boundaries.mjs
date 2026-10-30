// This is an actual engine oracle for runtime capabilities absent from the C++ candidate.
import fs from 'node:fs';
const result=[];
const cases=[
 {name:'fill-detached-before-call',method:'fill',before:true,args:[1,0,0]},
 {name:'copyWithin-detached-before-call',method:'copyWithin',before:true,args:[0,0,0]},
 {name:'fill-detach-during-value-empty-range',method:'fill',detachAt:0,args:[1,0,0]},
 {name:'fill-detach-during-start',method:'fill',detachAt:1,args:[1,0,3]},
 {name:'fill-detach-during-end',method:'fill',detachAt:2,args:[1,0,3]},
 {name:'copyWithin-detach-target-positive-count',method:'copyWithin',detachAt:0,args:[0,1,3]},
 {name:'copyWithin-detach-target-zero-count',method:'copyWithin',detachAt:0,args:[0,1,1]},
 {name:'copyWithin-detach-end-zero-count',method:'copyWithin',detachAt:2,args:[0,1,1]},
 {name:'copyWithin-detach-target-start-past-length',method:'copyWithin',detachAt:0,args:[0,8,9]},
];
for(const c of cases){const array=new Uint8Array([1,2,3,4]),events=[];const args=c.args.map((value,i)=>({[Symbol.toPrimitive](hint){events.push([i,hint]);if(c.detachAt===i)structuredClone(array.buffer,{transfer:[array.buffer]});return value;}}));if(c.before)structuredClone(array.buffer,{transfer:[array.buffer]});let outcome;try{outcome={returnedSelf:Uint8Array.prototype[c.method].apply(array,args)===array};}catch(e){outcome={error:e.name};}result.push({input:c,outcome,events,byteLength:array.byteLength,length:array.length});}
if(typeof ArrayBuffer.prototype.resize==='function')for(const tracking of [false,true])for(const method of ['fill','copyWithin'])for(const resizeAt of [0,1,2])for(const newLength of [0,2,6]){const buffer=new ArrayBuffer(4,{maxByteLength:8}),array=tracking?new Uint8Array(buffer):new Uint8Array(buffer,0,4),events=[];array.set([1,2,3,4]);const values=method==='fill'?[7,1,4]:[0,1,4],args=values.map((value,i)=>({[Symbol.toPrimitive](hint){events.push([i,hint]);if(i===resizeAt)buffer.resize(newLength);return value;}}));let outcome;try{outcome={returnedSelf:Uint8Array.prototype[method].apply(array,args)===array};}catch(e){outcome={error:e.name};}result.push({input:{tracking,method,resizeAt,newLength},outcome,events,byteLength:array.byteLength,length:array.length,bytes:Array.from(new Uint8Array(buffer))});}
if(typeof SharedArrayBuffer==='function'){const array=new Float32Array(new SharedArrayBuffer(20));array.set([1,2,3,4,5]);const first=array.fill(7,1,2)===array,second=array.copyWithin(1,0,4)===array;result.push({input:{sharedArrayBuffer:true},returnedSelf:first&&second,values:Array.from(array),status:'single-thread oracle; concurrent unordered byte semantics are outside current C++ runtime'});}
for(const name of ['BigInt64Array','BigUint64Array'])if(typeof globalThis[name]==='function'){const array=new globalThis[name]([1n,2n,3n]),events=[],f=globalThis[name].prototype.fill;let numberError;try{f.call(array,1);}catch(e){numberError=e.name;}f.call(array,{[Symbol.toPrimitive](hint){events.push(hint);return 7n;}},1);result.push({input:{constructor:name},numberError,values:Array.from(array,String),events});}
const output={engine:process.version,scope:'actual builtins only; candidates cannot execute these states until ArrayBuffer/TypedArray/JSValue runtime support is added',scenarios:result};if(process.argv[2])fs.writeFileSync(process.argv[2],JSON.stringify(output,null,2));console.log(`Recorded ${result.length} unsupported-runtime engine scenarios`);
