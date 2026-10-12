import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node --expose-gc compare-interleaved-buffers.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]),lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {InterleavedBuffer,InstancedInterleavedBuffer}=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const constructors=[Int8Array,Uint8Array,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array,Uint8ClampedArray];
const number=value=>value===undefined?'#undefined':typeof value!=='number'?value:Number.isNaN(value)?'#NaN':value===Infinity?'#Infinity':value===-Infinity?'#-Infinity':Object.is(value,-0)?'#-0':value;
const values=array=>Array.from(array,number);
function state(value){
    const result={keys:Object.keys(value),data:value.array===undefined?'#undefined':values(value.array)};
    for(const key of ['stride','count','usage','version','meshPerAttribute','isInterleavedBuffer','isInstancedInterleavedBuffer','needsUpdate'])result[key]=number(value[key]);
    result.ranges=value.updateRanges;return result;
}
function make(kind,size=12){const data=new constructors[kind](size);for(let i=0;i<size;++i)data[i]=i%3===0?-i*2.25:i*1.5+.25;data.buffer._uuid='buffer';return data;}
const attempt=callback=>{try{return callback();}catch(error){return error.name;}};
const getter=(target,name,log,value)=>Object.defineProperty(target,name,{configurable:true,enumerable:true,get(){log.push(name);return value;}});
function ordinaryCase(kind,instanced){
    const data=make(kind),source=instanced?new InstancedInterleavedBuffer(data,3,2):new InterleavedBuffer(data,3);source.uuid='source';const result={initial:state(source)};
    const rangeStart={},marker={x:3};source.addUpdateRange(rangeStart,marker);const ranges=source.updateRanges,range=ranges[0];
    result.rangeIdentity=[range.start===rangeStart,range.count===marker];source.clearUpdateRanges();result.rangesSame=ranges===source.updateRanges;
    for(const value of [true,1,'true',false,undefined,true])source.needsUpdate=value;
    source.version='5';source.needsUpdate=true;source.setUsage(35048);
    const target=instanced?new InstancedInterleavedBuffer(make(kind,4),1,5):new InterleavedBuffer(make(kind,4),1);
    target.uuid='target';target.version=4;target.addUpdateRange(1,2);function callback(){this.uploaded=true;}
    target.onUpload(callback);target.copy(source);result.copy=state(target);
    result.copyIdentity=[target.array!==data,target.array.buffer!==data.buffer,target.onUploadCallback===callback,target.uuid==='target'];
    target.onUploadCallback();result.callbackReceiver=target.uploaded;
    const metadata={},cloned=source.clone(metadata);result.clone=state(cloned);
    result.cloneIdentity=[cloned!==source,cloned.array!==data,cloned.array.buffer!==data.buffer,cloned.onUploadCallback===InterleavedBuffer.prototype.onUploadCallback,cloned.uuid!==source.uuid];
    const jsonMeta={},json=source.toJSON(jsonMeta);result.json=json;result.jsonKeys=Object.keys(json);result.meta=jsonMeta;result.final=state(source);return result;
}
function viewsCase(kind){
    const full=make(kind,16),a=full.subarray(2,10),b=full.subarray(2,10),c=full.subarray(3,11),meta={};
    const first=new InterleavedBuffer(a,2),second=new InterleavedBuffer(b,4),third=new InterleavedBuffer(c,2);
    const ca=first.clone(meta),cb=second.clone(meta),cc=third.clone(meta),result={};
    result.identity=[a!==b,a.buffer===b.buffer,ca.array!==cb.array,ca.array.buffer===cb.array.buffer,ca.array.buffer!==cc.array.buffer];
    result.cacheKeys=Object.keys(meta.arrayBuffers);ca.array[1]=42;result.data=[values(ca.array),values(cb.array),values(cc.array),values(a)];
    result.offsets=[ca.array.byteOffset,cb.array.byteOffset,cc.array.byteOffset];const jsonMeta={};first.uuid='first';second.uuid='second';third.uuid='third';
    result.json=[first.toJSON(jsonMeta),second.toJSON(jsonMeta),third.toJSON(jsonMeta)];result.meta=jsonMeta;return result;
}
function countCase(stride){const source=new InterleavedBuffer(make(6),stride),result={initial:state(source)};source.stride=7;result.countUnchanged=number(source.count);result.cloneCount=number(source.clone({}).count);return result;}
function copyingCase(){
    const result={},data=new Float32Array([1,2,3,4,5,6]),source=new InterleavedBuffer(data,3);
    source.copyAt(1/3,source,0);result.overlap=values(data);source.stride=2.5;
    source.copyAt(0,new InterleavedBuffer(new Float32Array([8,9,10,11,12,13]),2),.5);result.differentStride=values(data);
    source.copyAt(-.4,source,0);result.negative=values(data);source.set(data.subarray(0,4),1);result.setOverlap=values(data);
    result.setRangeError=attempt(()=>source.set([1,2],6));result.afterError=values(data);source.set([undefined,null,true,'3.5'],0);result.setConversions=values(data);return result;
}
function errorsCase(){
    const result={},absent=new InterleavedBuffer();result.default=state(absent);result.null=attempt(()=>new InterleavedBuffer(null,2));
    result.noMetaClone=attempt(()=>absent.clone());result.noMetaJSON=attempt(()=>absent.toJSON());result.undefinedArrayClone=attempt(()=>absent.clone({}));
    const unaligned=new InterleavedBuffer(new Uint8Array([1,2,3]),1),meta={};unaligned.array.buffer._uuid='small';result.unalignedJSON=attempt(()=>unaligned.toJSON(meta));result.unalignedMeta=meta;
    const source=new InterleavedBuffer(make(6),3);source.uuid='source';const mixed={};source.clone(mixed);result.jsonAfterClone=source.toJSON(mixed);
    const reverse={};source.toJSON(reverse);result.cloneAfterJSON=state(source.clone(reverse));source.array.buffer._uuid='__proto__';const polluted={};
    result.prototypeKey=state(source.clone(polluted));result.prototypeCache=Object.keys(polluted.arrayBuffers);
    const instance=new InstancedInterleavedBuffer(make(6),3,{});instance.copy(source);result.instanceFromBase=state(instance);instance.uuid='instance';instance.array.buffer._uuid='instance-buffer';result.instanceJSON=instance.toJSON({});return result;
}
function callbackCase(){
    let log=[];const source={},result={},array=make(6);getter(source,'array',log,array);getter(source,'count',log,4);getter(source,'stride',log,3);getter(source,'usage',log,35048);
    const target=new InterleavedBuffer();target.copy(source);result.copyLog=log;
    log=[];const data=make(6),buffer=new InterleavedBuffer(data,3);buffer.uuid='source';const meta={},cache={};getter(meta,'arrayBuffers',log,cache);buffer.clone(meta);result.cloneMetaLog=log;
    log=[];const ranges={};getter(buffer,'updateRanges',log,ranges);ranges.push=function(range){log.push(this===ranges,range);};buffer.addUpdateRange(2,7);result.rangeLog=log;
    log=[];const custom=new InterleavedBuffer(make(6),3);
    class Custom {constructor(array,stride){log.push('constructor');const value=new InterleavedBuffer(array,stride);value.custom=true;value.setUsage=function(usage){log.push('setUsage');this.usage=usage;return this;};return value;}}
    getter(custom,'constructor',log,Custom);getter(custom,'stride',log,2);getter(custom,'usage',log,35048);const clone=custom.clone({});result.customConstructorLog=log;result.custom=clone.custom;result.customCount=clone.count;
    log=[];const changing={reads:0},array2=make(6,6);Object.defineProperty(changing,'array',{get(){const n=this.reads++;log.push(n);return n===0?array:array2;}});
    changing.count=99;changing.stride=1;changing.usage=1;target.copy(changing);result.changingLog=log;result.changingData=values(target.array);result.changingCount=target.count;return result;
}
function gcCase(){
    const root={};{const data=make(6),source=new InterleavedBuffer(data,3);source.uuid='source';source.self=source;source.onUploadCallback=()=>source;root.source=source;}
    global.gc?.();const result={callbackCycle:root.source.onUploadCallback()===root.source};delete root.source;
    for(let i=0;i<20;++i){const array=make(6),value=new InterleavedBuffer(array,3);value.self=value;array.owner=value;array.buffer.view=array;}
    global.gc?.();result.cyclesCollected=true;const retained=new Float32Array([1,2,3,4]);retained.label='kept';retained.buffer._uuid='kept-buffer';global.gc?.();result.nativeRetained=[retained.label,retained.buffer._uuid];return result;
}
function bytesCase(){
    const bits=new Uint32Array([0x7fc12345,0xffcabcde,0x80000000,0x3f800000]),floats=new Float32Array(bits.buffer),integers=bits;floats.buffer._uuid='bits';
    const source=new InterleavedBuffer(floats,2),other=new InterleavedBuffer(integers,2),meta={},first=source.clone(meta),second=other.clone(meta),copy=new InterleavedBuffer();copy.copy(source);
    const result={sharedDifferentTypes:first.array.buffer===second.array.buffer,cloneBits:values(new Uint32Array(first.array.buffer)),copyBits:values(new Uint32Array(copy.array.buffer))};
    source.uuid='source';const jsonMeta={};result.json=source.toJSON(jsonMeta);result.meta=jsonMeta;
    const different=new Int16Array(floats.buffer,2,5),sub=new InterleavedBuffer(different,2),submeta={},cloned=sub.clone(submeta);
    result.subarray=values(cloned.array);result.subarrayKey=Object.keys(submeta.arrayBuffers);return result;
}
const coercion=(log,label,value)=>({valueOf(){log.push(label);return value;}});
function orderCase(){
    const result={};for(let mode=0;mode<3;++mode){const log=[],array=make(6),buffer=array.buffer;
        getter(array,'buffer',log,buffer);getter(array,'byteOffset',log,mode===0?0:4);getter(array,'byteLength',log,mode===0?48:16);getter(array,'constructor',log,Float32Array);
        const source=new InterleavedBuffer(array,3);source.uuid='source';const meta={};if(mode===2)source.toJSON(meta);else source.clone(meta);result[`keyLog:${mode}`]=log;}
    for(let mode=0;mode<4;++mode){const log=[],input={};getter(input,'length',log,2);getter(input,'0',log,coercion(log,'element0',7));getter(input,'1',log,coercion(log,'element1',8));
        const target=new InterleavedBuffer(new Float32Array([1,2,3]),1),offset=coercion(log,'offset',mode===0?0:mode===1?-1:mode===2?3:Infinity);
        const status=attempt(()=>{target.set(input,offset);return 'ok';});result[`set:${mode}`]=[status,log,values(target.array)];}
    const log=[],source={};getter(source,'length',log,3);getter(source,'0',log,9);Object.defineProperty(source,'1',{get(){log.push('1');throw new TypeError('read failed');}});
    const target=new InterleavedBuffer(new Float32Array([1,2,3]),1),status=attempt(()=>{target.set(source);return 'ok';});result.partialSet=[status,log,values(target.array)];return result;
}
const expected={};for(let kind=0;kind<9;++kind){expected[`base:${kind}`]=ordinaryCase(kind,false);expected[`instance:${kind}`]=ordinaryCase(kind,true);expected[`views:${kind}`]=viewsCase(kind);}
[0,1,3,2.5,'2',null,undefined,-2,true].forEach((stride,i)=>expected[`stride:${i}`]=countCase(stride));
function superCase(){
    const log=[],source={meshPerAttribute:7},result={},value=new InstancedInterleavedBuffer(make(6),3,2),original=Object.getPrototypeOf(InstancedInterleavedBuffer.prototype),replacement=Object.create(original);
    for(const name of ['copy','clone','toJSON'])Object.defineProperty(replacement,name,{configurable:true,get(){log.push([name,this===value]);return function(arg){log.push([`call:${name}`,this===value,arg.meshPerAttribute]);return {custom:name};};}});
    Object.setPrototypeOf(InstancedInterleavedBuffer.prototype,replacement);
    value.copy(source);result.copyMeshPerAttribute=value.meshPerAttribute;result.clone=value.clone(source);result.json=value.toJSON(source);result.log=log;
    Object.setPrototypeOf(InstancedInterleavedBuffer.prototype,original);return result;
}
Object.assign(expected,{copying:copyingCase(),errors:errorsCase(),callbacks:callbackCase(),bytes:bytesCase(),order:orderCase(),super:superCase(),gc:gcCase()});
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),{encoding:'utf8',maxBuffer:16*1024*1024}));
let comparisons=0;
function compare(actual,expected,where='root'){
    ++comparisons;if(expected&&typeof expected==='object'){
        assert.deepEqual(Object.keys(actual),Object.keys(expected),`${where}: key order`);
        for(const key of Object.keys(expected))compare(actual[key],expected[key],`${where}.${key}`);
    }else assert.deepEqual(actual,expected,where);
}
compare(actual,JSON.parse(JSON.stringify(expected)));console.log(`InterleavedBuffer parity: ${Object.keys(expected).length} scenarios, ${comparisons} comparisons`);
