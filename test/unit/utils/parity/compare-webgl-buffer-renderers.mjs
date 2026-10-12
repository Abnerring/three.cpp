import {closureCases} from './webgl-closure-constructibility.mjs';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-webgl-buffer-renderers.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {WebGLBufferRenderer}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLBufferRenderer.js')));
const {WebGLIndexedBufferRenderer}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLIndexedBufferRenderer.js')));
const numbers=()=>[0,-0,2,2.5,-1,NaN,Infinity,-Infinity,'3','bad',null,undefined,true,false];
const token=v=>v&&(['object','function'].includes(typeof v))?{ref:v.tag===undefined?'object':v.tag}:v;
const encode=v=>v===undefined?'undefined':typeof v==='number'?Number.isNaN(v)?'NaN':!Number.isFinite(v)?v<0?'-Infinity':'Infinity':Object.is(v,-0)?'-0':v
    :Array.isArray(v)?v.map(encode):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const index=(type=5123,bytes=2)=>({type,bytesPerElement:bytes});
const invoke=f=>{try{return encode(f());}catch(error){return {error:error.name};}};
class Harness {
    trace=[];gl={};extension={};extensions={};info={};control={};
    record(name,receiver){const trace=this.trace;return function(...args){trace.push([name,this===receiver,...args.map(token)]);};}
    constructor(indexed){
        for(const name of ['drawArrays','drawArraysInstanced','drawElements','drawElementsInstanced'])this.gl[name]=this.record(name,this.gl);
        for(const name of ['multiDrawArraysWEBGL','multiDrawElementsWEBGL'])this.extension[name]=this.record(name,this.extension);
        const h=this;this.extensions.get=function(name){h.trace.push(['get',this===h.extensions,name]);return h.extension;};this.info.update=this.record('info',this.info);
        this.renderer=new (indexed?WebGLIndexedBufferRenderer:WebGLBufferRenderer)(this.gl,this.extensions,this.info);
        this.control.renderer=this.renderer;this.renderer.setMode(4);if(indexed)this.renderer.setIndex(index());
    }
    convertible(name,value,throws=false,mutate=0){const h=this;return {tag:name,[Symbol.toPrimitive](hint){
        h.trace.push(['convert',name,hint]);const renderer=h.control.renderer;if(mutate){renderer.setMode(1);renderer.setIndex(index(99,8));}
        if(throws)throw RangeError('convert');return value;
    }};}
    getter(target,property,value,mutate=0,throws=false){const h=this;Object.defineProperty(target,property,{configurable:true,enumerable:true,get(){
        h.trace.push(['getter',property,this===target]);if(mutate)h.control.renderer.setMode(mutate);if(throws)throw RangeError('getter');return value;
    }});}
}
function numericCase(indexed,operation,s,c,instances){
    const h=new Harness(indexed),values=numbers();
    const result=invoke(()=>operation===0?h.renderer.render(values[s],values[c]):h.renderer.renderInstances(values[s],values[c],values[instances]));
    return {kind:'numeric',indexed,operation,start:s,count:c,instances,result,trace:encode(h.trace)};
}
function multiCase(indexed,values,draws){
    const h=new Harness(indexed),starts=[0,3,9],fixtures=[[1,2,3],['1','2','3'],[2,undefined,4],[-0,-0,-0],[NaN,3,4],[Infinity,-Infinity,4]],counts=fixtures[values];
    starts.tag='starts';counts.tag='counts';const drawCounts=[0,-0,1,2.5,'0',false,null,undefined,NaN,-2];
    const result=invoke(()=>h.renderer.renderMultiDraw(starts,counts,drawCounts[draws]));
    return {kind:'multi',indexed,values,draws,result,trace:encode(h.trace)};
}
function edgeCase(indexed,scenario){
    const h=new Harness(indexed),starts=[0,3,9],counts=[1,2,3],results=[];starts.tag='starts';counts.tag='counts';
    const command=indexed?'drawElements':'drawArrays',multi=indexed?'multiDrawElementsWEBGL':'multiDrawArraysWEBGL',run=(name,args)=>results.push(invoke(()=>h.renderer[name](...args)));
    if(scenario===0){h.renderer.setMode(undefined);if(indexed)h.renderer.setIndex({});run('render',[]);}
    if(scenario===1){if(indexed){const value=index();h.renderer.setIndex(value);value.type=99;value.bytesPerElement=8;}run('render',[3,6]);}
    if(scenario===2){if(indexed){const value=index(99,8);h.getter(value,'bytesPerElement',8,0,true);run('setIndex',[value]);}run('render',[3,6]);}
    if(scenario===3){if(indexed){const value=index(99,8);h.getter(value,'type',99,0,true);run('setIndex',[value]);}run('render',[3,6]);}
    if(scenario===4){if(indexed){h.renderer.setIndex(index(99,h.convertible('bytes',2)));run('render',[3,6]);}else run('render',[h.convertible('start',3),h.convertible('count',6)]);}
    if(scenario===5){run('render',[h.convertible('start',3,false,indexed?1:0),6]);run('render',[3,6]);}
    if(scenario===6){if(indexed)h.renderer.setIndex(index(5123,h.convertible('bytes',2,false,1)));run('renderInstances',[h.convertible('start',3),6,2]);}
    if(scenario===7){h.getter(h.gl,command,h.gl[command],3);run('render',[h.convertible('start',3),6]);}
    if(scenario===8){h.gl[command]=()=>{h.trace.push(['gpu-throw']);throw RangeError('gpu');};run('render',[3,6]);}
    if(scenario===9){h.gl[command]=undefined;run('render',[h.convertible('start',3),6]);}
    if(scenario===10){h.getter(h.gl,command,undefined,0,true);run('render',[h.convertible('start',3),6]);}
    if(scenario===11){h.getter(h.info,'update',h.info.update,3);run('render',[3,6]);}
    if(scenario===12){h.info.update=()=>{h.trace.push(['info-throw']);throw RangeError('info');};run('render',[3,6]);}
    if(scenario===13){h.info.update=h.record('replacement-info',h.info);run('render',[3,6]);}
    if(scenario===14){const r=new (indexed?WebGLIndexedBufferRenderer:WebGLBufferRenderer)();results.push(invoke(()=>r.renderInstances(h.convertible('start',3),h.convertible('count',6),-0)));}
    if(scenario===15)run('renderInstances',[3,6,h.convertible('instances',0)]);
    if(scenario===16){const original=h.extensions.get;h.getter(h.extensions,'get',function(...args){h.trace.push(['extension-callback']);h.control.renderer.setMode(2);return original.apply(this,args);},1);h.getter(h.extension,multi,h.extension[multi],3);run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===17){h.extensions.get=()=>{h.trace.push(['get-null']);return null;};run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===18){h.extension[multi]=undefined;run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===19){h.extension[multi]=()=>{h.trace.push(['multi-throw']);throw RangeError('multi');};run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===20){for(let i=0;i<3;i++)h.getter(counts,String(i),i+1);run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===21){const drawCount={tag:'drawCount',calls:0,[Symbol.toPrimitive](hint){const calls=this.calls++;h.trace.push(['condition',hint,calls]);return calls<2?3:1;}};run('renderMultiDraw',[starts,counts,drawCount]);}
    if(scenario===22){const drawCount={tag:'drawCount',limit:3,[Symbol.toPrimitive](hint){h.trace.push(['condition',hint,this.limit]);return this.limit;}};Object.defineProperty(counts,'0',{configurable:true,get(){drawCount.limit=1;h.trace.push(['count-0']);return 5;}});run('renderMultiDraw',[starts,counts,drawCount]);}
    if(scenario===23)run('renderMultiDraw',[starts,'123',3]);
    if(scenario===24)run('renderMultiDraw',[starts,null,1]);
    if(scenario===25)run('renderMultiDraw',[starts,undefined,NaN]);
    if(scenario===26){delete counts[1];run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===27){h.extension[multi]=()=>{counts[0]=10;h.control.renderer.setMode(1);h.trace.push(['multi-mutate']);};run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===28){counts[1]=h.convertible('count-1',2,true);run('renderMultiDraw',[starts,counts,3]);}
    if(scenario===29)run('renderMultiDraw',[starts,counts,'0']);
    if(scenario===30){h.renderer.render=function(){h.trace.push(['replacement-render',this===h.renderer]);return 42;};run('render',[3,6]);}
    if(scenario===31){h.renderer.setMode({tag:'mode'});run('render',[3,6]);}
    return {kind:'edge',indexed,scenario,results,trace:encode(h.trace)};
}
function prototypeCase(indexed,scenario){
    const h=new Harness(indexed),pollution=[],saved=new Map(),indexValue=Object.create(null);
    indexValue.type=5125;indexValue.bytesPerElement=4;
    try {
        for(const key of ['mode','type','bytesPerElement']) {
            saved.set(key,Object.getOwnPropertyDescriptor(Object.prototype,key));
            const descriptor=scenario===1?{configurable:true,value:77,writable:false}:{configurable:true,
                get(){pollution.push(['get',key]);return 77;},set(value){pollution.push(['set',key,value]);}};
            Object.defineProperty(Object.prototype,key,descriptor);
        }
        const results=[],result=invoke(()=>{
            h.renderer=new (indexed?WebGLIndexedBufferRenderer:WebGLBufferRenderer)(h.gl,h.extensions,h.info);
            const run=(name,args)=>results.push(invoke(()=>h.renderer[name](...args)));
            run('render',[3,6]);run('setMode',[4]);if(indexed)run('setIndex',[indexValue]);
            run('render',[3,6]);run('setMode',[1]);run('renderInstances',[5,8,2]);
            const starts=[0,3],counts=[6,2];starts.tag='starts';counts.tag='counts';
            run('renderMultiDraw',[starts,counts,2]);
        });
        return {kind:'prototype',indexed,scenario,result,results,trace:encode(h.trace),pollution:encode(pollution)};
    } finally {
        for(const [key,descriptor] of saved)if(descriptor)Object.defineProperty(Object.prototype,key,descriptor);else delete Object.prototype[key];
    }
}
const expected=[];
for(const indexed of [false,true]){
    for(let s=0;s<14;s++)for(let c=0;c<14;c++)expected.push(numericCase(indexed,0,s,c,0));
    for(let s=0;s<4;s++)for(let c=0;c<6;c++)for(let i=0;i<14;i++)expected.push(numericCase(indexed,1,s,c,i));
    for(let v=0;v<6;v++)for(let d=0;d<10;d++)expected.push(multiCase(indexed,v,d));
    for(let i=0;i<32;i++)expected.push(edgeCase(indexed,i));
    for(let i=0;i<2;i++)expected.push(prototypeCase(indexed,i));
}
expected.push(...closureCases('buffer', {WebGLBufferRenderer, WebGLIndexedBufferRenderer}));
expected.push(...closureCases('indexed', {WebGLBufferRenderer, WebGLIndexedBufferRenderer}));
const result=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});if(result.error)throw result.error;assert.equal(result.status,0,result.stderr);
const actual=result.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,where){comparisons++;if(Array.isArray(a)&&Array.isArray(b)){assert.equal(a.length,b.length,where+'.length');for(let i=0;i<a.length;i++)compare(a[i],b[i],`${where}[${i}]`);}
    else if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),where+'.keys');for(const key of Object.keys(b))compare(a[key],b[key],where+'.'+key);}
    else assert.equal(a,b,where);
}
for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`${i}:${expected[i].kind}`);
console.log(`WebGL buffer renderers parity: ${expected.length} cases / ${comparisons} comparisons passed against ${lock.commit}`);
