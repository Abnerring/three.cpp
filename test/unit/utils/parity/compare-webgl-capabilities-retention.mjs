import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4||typeof global.gc!=='function')throw Error('Usage: node --expose-gc compare-webgl-capabilities-retention.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]);
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const file='src/renderers/webgl/WebGLCapabilities.js';
const source=execFileSync('git',['-C',upstream,'show',`HEAD:${file}`]);
assert.equal(createHash('sha256').update(source).digest('hex'),'fc21d0cc71190f66bf7ce7a38b0a3b74c0f1dba5f1f5d94fd7a7f4bce39fc67a');
assert.equal(fs.readFileSync(path.join(upstream,file),'utf8').replaceAll('\r\n','\n'),source.toString('utf8'));
const {WebGLCapabilities}=await import(pathToFileURL(path.join(upstream,file)));
const methods=['getMaxAnisotropy','getMaxPrecision','textureFormatReadable','textureTypeReadable'];
function prepare(method,instance){
 const gl={getParameter(name){return name==='anisotropy'?{cached:true}:1;}};
 const extensions={has(){return true;},get(){return {MAX_TEXTURE_MAX_ANISOTROPY_EXT:'anisotropy'};}};
 const utils={convert(v){return v;}},parameters={precision:'lowp'};
 const cap=WebGLCapabilities(gl,extensions,parameters,utils),cached=cap.getMaxAnisotropy();
 const fn=cap[method],keep=instance?Reflect.construct(function Empty(){},[],fn):fn;
 return {method,mode:instance?'new-target-prototype':'direct',keep,refs:Object.fromEntries(Object.entries({gl,extensions,utils,parameters,cached}).map(([k,v])=>[k,new WeakRef(v)]))};
}
const held=methods.flatMap(m=>[prepare(m,false),prepare(m,true)]);
const collect=async()=>{for(let i=0;i<12;++i){await new Promise(resolve=>setImmediate(resolve));global.gc();}};
const alive=value=>Object.fromEntries(Object.entries(value.refs).map(([k,ref])=>[k,ref.deref()!==undefined]));
await collect();
const expected=held.map(value=>({method:value.method,mode:value.mode,alive:alive(value)}));
for(const value of held)value.keep=null;
await collect();
expected.forEach((value,i)=>{value.afterDrop=alive(held[i]);});
const actual=JSON.parse(execFileSync(process.argv[3],[],{encoding:'utf8'}));
try{assert.deepEqual(actual,expected);}catch(error){console.error(JSON.stringify({actual,expected},null,2));throw error;}
console.log('WebGLCapabilities retention: 8 actual pinned-source GC cases passed (direct methods and newTarget prototype constructor edges).');
