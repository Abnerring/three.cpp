import {closureCases} from './webgl-closure-constructibility.mjs';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync,spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-webgl-properties.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
const relativeLock=new URL('../../../../upstream.json',import.meta.url);
const lock=JSON.parse(fs.readFileSync(fs.existsSync(relativeLock)?relativeLock:path.resolve('upstream.json')));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {WebGLProperties}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLProperties.js')));
const encode=v=>v===undefined?'undefined':v===null?null:typeof v==='function'?'function':typeof v==='number'
    ?Number.isNaN(v)?'NaN':!Number.isFinite(v)?v<0?'-Infinity':'Infinity':Object.is(v,-0)?'-0':v
    :Array.isArray(v)?v.map(encode):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(key=>{
        const d=Object.getOwnPropertyDescriptor(v,key);return [key,Object.hasOwn(d,'value')?encode(d.value):'accessor'];
    })):v;
const scalars=()=>[undefined,null,false,true,0,-0,1.5,-2,NaN,Infinity,-Infinity,'','3','x'];
const key=type=>type===1?[]:type===2?function(){}:{};
const invoke=fn=>{try{return encode(fn());}catch(error){return {error:error.name};}};
const converter=(trace,result,action=0,p=undefined,object=undefined)=>({[Symbol.toPrimitive](hint){
    trace.push(['convert',hint]);if(action===1)throw RangeError('conversion');if(action===2)return {};
    if(action===3)p.dispose();if(action===4)p.remove(object);return result;
}});
function basicCase(type){
    const p=new WebGLProperties(),object=key(type),other=key(type),results=[encode(p.has(object))],first=p.get(object),second=p.get(other);
    results.push(first===p.get(object),first!==second,Object.getPrototypeOf(first)===Object.prototype);
    const value={};results.push(invoke(()=>p.update(object,'value',value)),first.value===value,invoke(()=>p.remove(object)),encode(p.has(object)));
    const third=p.get(object);results.push(third!==first,invoke(()=>p.dispose()),encode(p.has(object)));first.after=42;results.push(encode(first.after));
    const detached=p.get;results.push(detached.call(null,object)===p.get(object));p.dispose.call(false);results.push(encode(p.has(object)));
    return {kind:'basic',type,results};
}
function primitiveCase(index){const p=new WebGLProperties(),value=scalars()[index];return {kind:'primitive',index,
    has:invoke(()=>p.has(value)),get:invoke(()=>p.get(value)),remove:invoke(()=>p.remove(value)),update:invoke(()=>p.update(value,'x',42))};}
function scalarCase(k,v){const p=new WebGLProperties(),object=key(0),target=p.get(object),values=scalars(),result=invoke(()=>p.update(object,values[k],values[v]));
    return {kind:'scalar',key:k,value:v,result,target:encode(target)};}
function specialCase(k,v){const p=new WebGLProperties(),object=key(0),target=p.get(object),keys=['__proto__','constructor','toString','01','2','length'],result=invoke(()=>p.update(object,keys[k],scalars()[v]));
    return {kind:'special',key:k,value:v,result,target:encode(target),nullPrototype:Object.getPrototypeOf(target)===null};}
function coercionCase(scenario){
    const p=new WebGLProperties(),object=key(0),trace=[];let target=undefined,property=undefined;
    if(scenario!==0)target=p.get(object);property=converter(trace,'x',scenario===2?1:scenario===3?2:scenario===9?3:scenario===10?4:0,p,object);
    if(scenario>=4&&scenario<=8){property={};const toString=function(){trace.push(['toString']);if(scenario===8)throw RangeError('toString');if(scenario===6)return {};return 'x';};
        Object.defineProperty(property,'toString',{configurable:true,get(){trace.push(['get-toString']);if(scenario===7)throw RangeError('getter');return scenario===5?17:toString;}});
        Object.defineProperty(property,'valueOf',{get(){trace.push(['get-valueOf']);return function(){trace.push(['valueOf']);return 'x';};}});
    }
    if(scenario===11)Object.defineProperty(target,'x',{value:1,writable:false,enumerable:true,configurable:true});
    if(scenario===12){const prototype={};Object.defineProperty(prototype,'x',{configurable:true,set(value){trace.push(['set',this===target,value]);}});Object.setPrototypeOf(target,prototype);}
    if(scenario===13||scenario===14)Object.defineProperty(target,'x',{enumerable:true,configurable:true,get(){trace.push(['unexpected-get']);return 1;},
        ...(scenario===13?{set(value){trace.push(['set',this===target,value]);}}:{})});
    if(scenario===15)Object.preventExtensions(target);
    const saved=Object.getOwnPropertyDescriptor(WeakMap.prototype,'get');
    try {
        if(scenario>=16&&scenario<=18)Object.defineProperty(WeakMap.prototype,'get',{value(){return scenario===16?null:scenario===17?17:false;},writable:true,configurable:true});
        const result=invoke(()=>p.update(object,property,42));return {kind:'coercion',scenario,result,target:encode(target),has:encode(p.has(object)),trace:encode(trace)};
    } finally {Object.defineProperty(WeakMap.prototype,'get',saved);}
}
function dynamicCase(scenario){
    const p=new WebGLProperties(),object=key(0),trace=[],registry=[],target=p.get(object),saved=new Map(),names=['get','set','delete','has'];target.before=1;
    const mapId=map=>{const index=registry.indexOf(map);if(index!==-1)return index+1;registry.push(map);return registry.length;};
    if(scenario<=5||scenario>=16||scenario===13)p.remove(object);
    try {
        names.forEach((name,method)=>{
            const descriptor=Object.getOwnPropertyDescriptor(WeakMap.prototype,name),original=WeakMap.prototype[name];saved.set(name,descriptor);
            const replacement=function(...args){trace.push(['call',name,mapId(this),this instanceof WeakMap]);
                if((scenario===0&&method===0)||(scenario===2&&method===1)||(scenario===6&&method===2)||(scenario===8&&method===3)||(scenario===13&&method===0))p.dispose();
                if((scenario===4&&method===0)||(scenario===10&&method===2))throw RangeError('method');
                if(scenario>=16&&method===0)return scenario===16?null:scenario===17?false:scenario===18?0:scenario===19?'':scenario===20?{}:undefined;
                return original.apply(this,args);
            };
            const getter=(scenario===1&&method===0)||(scenario===3&&method===1)||(scenario===5&&method===0)||(scenario===7&&method===2)||(scenario===9&&method===3)||(scenario===11&&method===2)||(scenario===12&&method===0)||(scenario===13&&method===1);
            Object.defineProperty(WeakMap.prototype,name,getter?{configurable:true,get(){trace.push(['getter',name,mapId(this),this instanceof WeakMap]);
                if(scenario===5||scenario===11)throw RangeError('getter');if(scenario===12)return 17;p.dispose();return replacement;
            }}:{configurable:true,writable:true,value:scenario===14&&method===0?17:replacement});
        });
        const result=invoke(()=>[6,7,10,11].includes(scenario)?p.remove(object):[8,9].includes(scenario)?p.has(object):scenario===15?p.update(object,'after',42):p.get(object));
        for(const [name,d] of saved)Object.defineProperty(WeakMap.prototype,name,d);
        return {kind:'dynamic',scenario,result,old:encode(target),has:encode(p.has(object)),trace:encode(trace)};
    } finally {for(const [name,d] of saved)Object.defineProperty(WeakMap.prototype,name,d);}
}
function pollutionCase(scenario){
    const trace=[],saved=new Map();try {
        for(const name of ['has','get','remove','update','dispose','properties']) {
            saved.set(name,Object.getOwnPropertyDescriptor(Object.prototype,name));
            Object.defineProperty(Object.prototype,name,scenario===1?{__proto__:null,configurable:true,value:77,writable:false}:{__proto__:null,configurable:true,
                get(){trace.push(['get',name]);return 77;},set(value){trace.push(['set',name,value]);}});
        }
        let state=null;const descriptors=[],result=invoke(()=>{
            const p=new WebGLProperties();for(const name of Object.keys(p)){const d=Object.getOwnPropertyDescriptor(p,name);descriptors.push([name,!!d.writable,!!d.enumerable,!!d.configurable]);}
            const object=key(0),target=p.get(object),update=invoke(()=>p.update(object,'get',42));
            state={update,target:encode(target),has:encode(p.has(object)),plainPrototype:Object.getPrototypeOf(target)===Object.prototype};
        });return {kind:'pollution',scenario,result,descriptors,state,trace:encode(trace)};
    } finally {for(const [name,d] of saved)if(d)Object.defineProperty(Object.prototype,name,d);else delete Object.prototype[name];}
}
const expected=[];
for(let i=0;i<3;i++)expected.push(basicCase(i));for(let i=0;i<14;i++)expected.push(primitiveCase(i));
for(let k=0;k<14;k++)for(let v=0;v<14;v++)expected.push(scalarCase(k,v));
for(let k=0;k<6;k++)for(let v=0;v<14;v++)expected.push(specialCase(k,v));
for(let i=0;i<19;i++)expected.push(coercionCase(i));for(let i=0;i<22;i++)expected.push(dynamicCase(i));for(let i=0;i<2;i++)expected.push(pollutionCase(i));
expected.push(...closureCases('properties', {WebGLProperties}));
const result=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});if(result.error)throw result.error;assert.equal(result.status,0,result.stderr);
const actual=result.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));assert.equal(actual.length,expected.length);
let comparisons=0;function compare(a,b,where){comparisons++;
    if(Array.isArray(a)&&Array.isArray(b)){assert.equal(a.length,b.length,where+'.length');for(let i=0;i<a.length;i++)compare(a[i],b[i],`${where}[${i}]`);}
    else if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),where+'.keys');for(const key of Object.keys(b))compare(a[key],b[key],where+'.'+key);}
    else assert.equal(a,b,where);
}
for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`${i}:${expected[i].kind}`);
console.log(`WebGLProperties parity: ${expected.length} cases / ${comparisons} comparisons passed against ${lock.commit}`);
