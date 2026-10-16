import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {Cache}=await import(pathToFileURL(path.join(upstream,'src/loaders/Cache.js')));
const cases=[];
for(const op of ['add','get','remove','clear'])for(const enabled of [false,true,0,1,null,''])
for(const url of ['blob:x','https://host/file','invalid','BLOB:x'])for(const index of [-1,0,2,null,'x'])
for(const thrown of ['', 'enabled','files','setFiles','dictGet','dictSet','sliceGet','sliceCall','indexOfGet','indexOfCall','toStringGet','toStringCall'])cases.push({op,enabled,url,index,throw:thrown});
const encode=v=>v===undefined?'#undefined':typeof v==='function'?'#function':v;
function run(r){
    const trace=[],state={enabled:r.enabled,stored:12};function event(s){trace.push(s);if(r.throw===s)throw new TypeError(s);}
    const dictionary={};Object.defineProperty(dictionary,'slot',{get(){event('dictGet');return state.stored;},set(v){event('dictSet');state.stored=v;},enumerable:true,configurable:true});state.files=dictionary;
    const target={get enabled(){event('enabled');return state.enabled;},get files(){event('files');return state.files;},set files(v){event('setFiles');state.files=v;}};
    const key={};for(const name of ['slice','indexOf','toString'])Object.defineProperty(key,name,{get(){event(name+'Get');return function(...a){event(name+'Call');if(name==='slice'){trace.push(encode(a[0]));return r.url;}if(name==='indexOf')return r.index;return 'slot';};},enumerable:true,configurable:true});
    let result;try{result=Cache[r.op].call(target,key,39);}catch(e){result=e.name;}
    return [encode(result),trace,state.stored,state.files===dictionary,!!Object.getOwnPropertyDescriptor(dictionary,'slot')];
}
const expected=cases.map(run),actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:64*1024*1024}));
assert.equal(actual.length,expected.length);for(let i=0;i<cases.length;i++)assert.deepEqual(actual[i],expected[i],`${i} ${JSON.stringify(cases[i])}`);
console.log(`Cache dynamic protocol parity: ${cases.length} scenarios`);
const native=JSON.parse(execFileSync(binary,['gc'],{encoding:'utf8'}));
const snapshots=[];let weak,old;
function put(){const payload={};payload.self=payload;weak=new WeakRef(payload);Cache.add('a',payload);}
async function jobs(){await new Promise(setImmediate);await new Promise(setImmediate);global.gc();await new Promise(setImmediate);global.gc();}
async function observe(){await jobs();snapshots.push(weak.deref()!==undefined);}
Cache.clear();Cache.enabled=true;put();await observe();
Cache.remove('a');await observe();
put();old=Cache.files;Cache.clear();await observe();
old=undefined;await observe();
put();Cache.clear();await observe();
assert.deepEqual(native,snapshots);assert.deepEqual(snapshots,[true,false,true,false,false]);
console.log(`Cache actual Node/native GC parity: ${snapshots.length} snapshots`);
