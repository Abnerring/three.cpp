import {closureCases} from './webgl-closure-constructibility.mjs';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-webgl-info.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {WebGLInfo}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLInfo.js')));
const {setConsoleFunction}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
const modes=[4,1,3,2,0],numbers=()=>[0,-0,1,3,5.5,-2,NaN,Infinity,-Infinity,'6','bad',null,undefined,true,false];
const glObject=()=>({TRIANGLES:4,LINES:1,LINE_STRIP:3,LINE_LOOP:2,POINTS:0});
const encode=v=>v===undefined?'undefined':typeof v==='function'?'function':typeof v==='number'
    ?Number.isNaN(v)?'NaN':!Number.isFinite(v)?v<0?'-Infinity':'Infinity':Object.is(v,-0)?'-0':v
    :Array.isArray(v)?v.map(encode):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
const convertible=(trace,name,result,throws=false,mutate=undefined)=>({[Symbol.toPrimitive](hint){
    trace.push([name,hint]);if(mutate){mutate.calls=100;mutate.triangles=100;mutate.lines=100;mutate.points=100;}
    if(throws)throw RangeError('conversion');return result;
}});
const installConsole=(trace,throws=false)=>setConsoleFunction(function(...args){trace.push(['console',this===undefined,...args]);if(throws)throw RangeError('console');});
const invoke=f=>{try{return encode(f());}catch(error){return {error:error.name};}};
const state=(info,captured)=>({keys:Object.keys(info),memory:encode(info.memory),render:encode(info.render),captured:encode(captured),programs:encode(info.programs),autoReset:encode(info.autoReset)});
function numericCase(mode,c,i){
    const trace=[];installConsole(trace);const info=new WebGLInfo(glObject()),r=info.render,values=numbers();r.frame=7;info.memory.textures=3;
    const result=invoke(()=>info.update(values[c],mode,values[i])),updated=state(info,r),reset=invoke(()=>info.reset());
    return {kind:'numeric',mode,count:c,instances:i,result,updated,reset,after:state(info,r),trace:encode(trace)};
}
function counterCase(mode,index){
    const trace=[];installConsole(trace);const info=new WebGLInfo(glObject()),r=info.render,values=numbers();r.calls=values[index];
    for(const field of ['triangles','lines','points'])r[field]=values[index];
    const result=invoke(()=>info.update(6,mode,2));return {kind:'counter',mode,index,result,state:state(info,r),trace:encode(trace)};
}
function coercionCase(mode,reentry){
    const trace=[];installConsole(trace);const info=new WebGLInfo(glObject()),r=info.render,field=mode===4?'triangles':mode===0?'points':'lines';
    r.calls=convertible(trace,'calls','2');r[field]=convertible(trace,'previous','sum:');
    const result=invoke(()=>info.update(convertible(trace,'count',6,false,reentry?r:undefined),mode,convertible(trace,'instances',2)));
    return {kind:'coercion',mode,reentry,result,state:state(info,r),trace:encode(trace)};
}
function glCase(index){
    const trace=[];installConsole(trace);const gl=glObject(),mode={tag:'mode'},names=['TRIANGLES','LINES','LINE_STRIP','LINE_LOOP','POINTS'];
    names.forEach((name,i)=>Object.defineProperty(gl,name,{configurable:true,get(){trace.push(['gl',name,this===gl]);return i===index?mode:'other';}}));
    const info=new WebGLInfo(gl),r=info.render,result=invoke(()=>info.update(6,mode,2));
    return {kind:'gl',index,result,state:state(info,r),trace:encode(trace)};
}
function closureCase(){
    const trace=[];installConsole(trace);const info=new WebGLInfo(glObject()),r=info.render;r.frame=17;
    info.render=null;info.memory='memory';info.programs=['program'];info.autoReset='manual';
    const update=invoke(()=>info.update.call(null,9,4,2)),updated=state(info,r),reset=invoke(()=>info.reset()),copy=info;copy.programs=18;
    return {kind:'closure',update,updated,reset,alias:copy===info,after:state(info,r),trace:encode(trace)};
}
function resetCase(index){
    const trace=[];installConsole(trace);const info=new WebGLInfo(glObject()),r=info.render;r.frame=13;
    ['calls','triangles','points','lines'].forEach((name,i)=>Object.defineProperty(r,name,{configurable:true,set(value){trace.push(['set',name,value]);if(i===index)throw TypeError('reset');}}));
    const result=invoke(()=>info.reset());return {kind:'reset',index,result,state:state(info,r),trace:encode(trace)};
}
function exceptionCase(index){
    const trace=[];installConsole(trace,index===7);let gl=glObject();
    if(index===0)gl=undefined;if(index===1)gl=null;if(index===2)gl={};
    if(index===3)Object.defineProperty(gl,'TRIANGLES',{get(){throw RangeError('gl');}});
    const info=new WebGLInfo(gl),r=info.render;let count=6,mode=4,instances=2;
    if(index===2)mode=undefined;if(index===4)count=convertible(trace,'count',6,true);if(index===5)instances=convertible(trace,'instances',2,true);
    if(index===6||index===7||index===8)mode='4';if(index===8)setConsoleFunction(1);
    if(index===9)r.calls=convertible(trace,'calls',0,true);if(index===10)r.triangles=convertible(trace,'previous',0,true);
    if(index===11){mode={isStackTrace:true,getError(){throw RangeError('stackTrace');}};setConsoleFunction(null);}
    if(index===12){mode={};Object.defineProperty(mode,'isStackTrace',{get(){throw RangeError('stackTrace');}});setConsoleFunction(false);}
    const result=invoke(()=>info.update(count,mode,instances));return {kind:'exception',index,result,state:state(info,r),trace:encode(trace)};
}
function prototypeCase(scenario){
    const trace=[],saved=new Map(),gl=glObject();installConsole(trace);
    try {
        for(const key of ['geometries','textures','frame','calls','triangles','points','lines','memory','render','programs','autoReset','reset','update']) {
            saved.set(key,Object.getOwnPropertyDescriptor(Object.prototype,key));
            Object.defineProperty(Object.prototype,key,scenario===1?{configurable:true,value:77,writable:false}:{configurable:true,
                get(){trace.push(['get',key]);return 77;},set(value){trace.push(['set',key,value]);}});
        }
        let initial=null,updated=null,after=null,deleted=null;const descriptors=[],results=[];
        const result=invoke(()=>{
            const info=new WebGLInfo(gl),r=info.render;initial=state(info,r);
            for(const object of [info,info.memory,r])for(const key of Object.keys(object)) {
                const d=Object.getOwnPropertyDescriptor(object,key);
                descriptors.push([key,Object.hasOwn(d,'value'),!!d.writable,!!d.enumerable,!!d.configurable,
                    Object.hasOwn(d,'get'),Object.hasOwn(d,'set'),Object.getPrototypeOf(object)===Object.prototype]);
            }
            results.push(invoke(()=>info.update(6,4,2)));updated=state(info,r);
            results.push(invoke(()=>info.reset()));info.programs=9;after=state(info,r);
            delete r.triangles;results.push(invoke(()=>info.update(6,4,1)));deleted=state(info,r);
        });
        return {kind:'prototype',scenario,result,initial,descriptors,results,updated,after,deleted,trace:encode(trace)};
    } finally {
        for(const [key,d] of saved)if(d)Object.defineProperty(Object.prototype,key,d);else delete Object.prototype[key];
    }
}
const expected=[];
for(const mode of modes)for(let c=0;c<numbers().length;c++)for(let i=0;i<numbers().length;i++)expected.push(numericCase(mode,c,i));
for(const mode of modes)for(let i=0;i<15;i++)expected.push(counterCase(mode,i));
for(const mode of modes)for(const reentry of [false,true])expected.push(coercionCase(mode,reentry));
for(let i=0;i<6;i++)expected.push(glCase(i));expected.push(closureCase());
for(let i=0;i<5;i++)expected.push(resetCase(i));for(let i=0;i<13;i++)expected.push(exceptionCase(i));setConsoleFunction(null);
for(let i=0;i<2;i++)expected.push(prototypeCase(i));setConsoleFunction(null);
setConsoleFunction(()=>{});
expected.push(...closureCases('info', {WebGLInfo}));
setConsoleFunction(null);
const result=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});
if(result.error)throw result.error;assert.equal(result.status,0,result.stderr);
const actual=result.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,where){
    comparisons++;
    if(Array.isArray(a)&&Array.isArray(b)){assert.equal(a.length,b.length,where+'.length');for(let i=0;i<a.length;i++)compare(a[i],b[i],`${where}[${i}]`);}
    else if(a&&b&&typeof a==='object'&&typeof b==='object'){
        assert.deepEqual(Object.keys(a),Object.keys(b),where+'.keys');for(const key of Object.keys(b))compare(a[key],b[key],where+'.'+key);
    }else assert.equal(a,b,where);
}
for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`${i}:${expected[i].kind}`);
console.log(`WebGLInfo parity: ${expected.length} cases / ${comparisons} comparisons passed against ${lock.commit}`);
