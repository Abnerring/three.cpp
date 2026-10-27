import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=process.argv[3]&&process.argv[3]!=='--oracle'?path.resolve(process.argv[3]):null,directory=path.resolve(process.argv[4]??path.join(process.env.TEMP,'uniform-group-node-parity'));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const U={tag:'undefined'};
const cases=[{entry:'wrapper',args:['direct']},{entry:'binding',args:[]},{entry:'uniformGroup',args:['plain']},{entry:'sharedUniformGroup',args:['shared']},{entry:'wrapper',args:['explicit',true,4,'custom']},{entry:'uniformGroup',args:['defaults',U,U]},{entry:'sharedUniformGroup',args:['null',null,null]},{entry:'wrapper',args:['observed'],observed:true},{entry:'binding',args:['snapshot'],observed:true,enumAfter:true},{entry:'wrapper',args:['private'],isolate:true},{entry:'wrapper',args:['prototype'],poisonConstructor:true},...['FRAME','RENDER','OBJECT'].map(throwAt=>({entry:'wrapper',args:['throw'],observed:true,throwAt}))];
fs.mkdirSync(directory,{recursive:true});
// Each case starts a real new process, so module declarations and all three preset IDs are observed.
const source=`import fs from 'node:fs';const r=JSON.parse(fs.readFileSync(0,'utf8')),events=[],out={};
const types=(await import(${JSON.stringify(pathToFileURL(path.join(upstream,'src/nodes/core/constants.js')).href)})).NodeUpdateType;
if(r.observed)for(const key of ['FRAME','RENDER','OBJECT'])Object.defineProperty(types,key,{get(){events.push(key);if(key===r.throwAt)throw new TypeError('enum getter');return 'observed-'+key;},enumerable:true,configurable:true});
function normal(v){if(v===undefined)return '#undefined';if(typeof v==='function')return '#function';if(v===null||typeof v!=='object')return v;const o=Array.isArray(v)?new Array(v.length):{};for(const k of Object.keys(v))o[k]=normal(v[k]);return o;}
const decode=v=>v&&v.tag==='undefined'?undefined:v;
function constructible(v){try{Reflect.construct(function(){},[],v);return true;}catch{return false;}}
try{const m=await import(${JSON.stringify(pathToFileURL(path.join(upstream,'src/nodes/core/UniformGroupNode.js')).href)}),args=r.args.map(decode),node=r.entry==='wrapper'||r.entry==='binding'?new m.default(...args):m[r.entry](...args);out.keys=Object.keys(m);out.constructor={name:m.default.name,length:m.default.length,constructible:constructible(m.default),defaultIdentity:m.default===node.constructor};out.groups={};for(const k of ['frameGroup','renderGroup','objectGroup'])out.groups[k]=normal(m[k]);out.node=normal(node);
if(r.enumAfter){for(const key of ['FRAME','RENDER','OBJECT'])Object.defineProperty(types,key,{value:'changed',writable:true,enumerable:true,configurable:true});out.snapshot=[m.frameGroup.updateType,m.renderGroup.updateType,m.objectGroup.updateType];}
if(r.isolate){const facade={...m},ctor=m.default,frame=m.frameGroup;facade.default=null;facade.uniformGroup=null;facade.frameGroup=null;out.privateIdentity=m.default===ctor&&m.frameGroup===frame;out.afterFacade=normal(new m.default('afterFacade'));}
if(r.poisonConstructor){m.default.prototype.constructor=()=>{throw new TypeError('mutable prototype constructor');};out.afterPrototype=normal(new m.default('afterPrototype'));}
}catch(e){out.error='#'+e.name;}out.events=events;process.stdout.write(JSON.stringify(out));`;
const oracle=path.join(directory,'oracle.mjs');fs.writeFileSync(oracle,source);fs.writeFileSync(path.join(directory,'input.json'),JSON.stringify(cases,null,2));
const expected=cases.map(r=>JSON.parse(execFileSync(process.execPath,[oracle],{input:JSON.stringify(r),encoding:'utf8',maxBuffer:8*1024*1024})));fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));
if(!binary){console.log(`Prepared ${cases.length} real fresh-process UniformGroupNode scenarios`);process.exit(0);}
const actual=cases.map(r=>JSON.parse(execFileSync(binary,[],{input:JSON.stringify(r),encoding:'utf8',maxBuffer:8*1024*1024})));fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));const differences=[];for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}fs.writeFileSync(path.join(directory,'differences.json'),JSON.stringify(differences,null,2));assert.equal(differences.length,0,JSON.stringify(differences.slice(0,2),null,2));console.log(`UniformGroupNode: ${cases.length} real fresh-process scenarios passed (absolute IDs and enum getter order)`);
