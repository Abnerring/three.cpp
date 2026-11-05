import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import {withSchema} from './object-loader-oracle.mjs';

const [upstream,probe,requestedOutput]=process.argv.slice(2),sourceOnly=['--oracle-only','--source-only'].includes(probe);
assert(upstream&&probe,'Usage: compare-buffer-geometry-loader-native.mjs <pinned> <probe|--oracle-only> [fresh-directory]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url),'utf8'));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=relative=>import(pathToFileURL(path.join(upstream,'src',relative)).href);
const [{BufferGeometryLoader},{Loader}]=await Promise.all([load('loaders/BufferGeometryLoader.js'),load('loaders/Loader.js')]);
const ordinary={data:{attributes:{position:{type:'Float32Array',array:[0,0,0,1,2,3],itemSize:3}},index:{type:'Uint16Array',array:[0,1]},groups:[{start:0,count:2,materialIndex:3}],boundingSphere:{center:[1,2,3],radius:4}},name:'triangle',userData:{value:17}};
const ib={data:{arrayBuffers:{words:[1065353216,1073741824,1077936128]},interleavedBuffers:{ib:{uuid:'ib',buffer:'words',type:'Float32Array',stride:3,usage:35048}},attributes:{position:{isInterleavedBufferAttribute:true,data:'ib',itemSize:3,offset:0},normal:{isInterleavedBufferAttribute:true,data:'ib',itemSize:3,offset:0}}}};
const clone=value=>JSON.parse(JSON.stringify(value));
const requests=[
    {case:'metadata',op:'metadata'},
    {case:'empty',op:'parse',json:{data:{attributes:{}}}},
    {case:'ordinary',op:'parse',json:ordinary},
    {case:'instanced',op:'parse',json:{...ordinary,isInstancedBufferGeometry:true}},
    {case:'morph-instanced-flags',op:'parse',json:{data:{attributes:{position:{type:'Float32Array',array:[1,2,3],itemSize:3,isInstancedBufferAttribute:true,meshPerAttribute:7}},morphAttributes:{position:[{type:'Float32Array',array:[1,2,3],itemSize:3,isInstancedBufferAttribute:true}]},morphTargetsRelative:1}}},
    {case:'truthy-groups',op:'parse',json:{data:{attributes:{},groups:null,drawcalls:[{start:'0',count:'3'}]}}},
    {case:'interleaved',op:'parse',json:ib},
    {case:'interleaved-reentry',op:'parse',json:clone(ib),variant:'reenter',nested:{data:{...clone(ib.data),arrayBuffers:{words:[1091567616,1092616192,1093664768]}}}},
    {case:'data-getter-sites',op:'parse',json:{data:{attributes:{}}},variant:'dataGetter'},
    {case:'inherited-attributes',op:'parse',json:{data:{attributes:{}}},variant:'inherited',inherited:{inherited:{type:'Uint8Array',array:[1,2,3],itemSize:3}}},
    {case:'interleaved-usage-throws',op:'parse',json:{data:{...clone(ib.data),attributes:{position:{...ib.data.attributes.position,usage:35048}}}},expectedError:{name:'TypeError'}},
    {case:'unknown-typedarray-throws',op:'parse',json:{data:{attributes:{position:{type:'UnknownArray',array:[1],itemSize:1}}}},expectedError:{name:'TypeError'}}
].map(withSchema);
function metadata(){const ctor=BufferGeometryLoader,p=ctor.prototype,methods={};for(const name of Object.getOwnPropertyNames(p))if(name!=='constructor'){const d=Object.getOwnPropertyDescriptor(p,name),f=d.value;let constructible;try{Reflect.construct(function(){},[],f);constructible=true;}catch{constructible=false;}methods[name]={name:f.name,length:f.length,source:f.toString(),flags:[d.writable,d.enumerable,d.configurable],constructible};}return {name:ctor.name,length:ctor.length,source:ctor.toString(),super:Object.getPrototypeOf(ctor)===Loader,prototypeNames:Object.getOwnPropertyNames(p),methods};}
function snapshot(request,root){
    const seen=new Map(),uuids=new Set();
    const collect=value=>{if(value===null||typeof value!=='object')return;for(const key of Object.keys(value)){if(['_schema','_brands','_structuralBrands'].includes(key))continue;const child=value[key];if(key==='uuid'&&typeof child==='string')uuids.add(child);collect(child);}};collect(request);
    function visit(source){
        if(source===undefined)return {$undefined:true};if(typeof source==='number'&&!Number.isFinite(source))return {$number:Number.isNaN(source)?'NaN':source<0?'-Infinity':'Infinity'};
        if(source===null||!['object','function'].includes(typeof source))return source;
        if(seen.has(source))return {$ref:seen.get(source)};const result={$id:seen.size+1};seen.set(source,result.$id);
        if(Array.isArray(source)){result.$type='Array';result.items=Array.from({length:source.length},(_,i)=>visit(source[i]));return result;}
        for(const name of ['Int8Array','Uint8Array','Uint8ClampedArray','Int16Array','Uint16Array','Int32Array','Uint32Array','Float32Array','Float64Array'])if(source instanceof globalThis[name]){result.$type=name;result.items=Array.from(source,visit);return result;}
        let brand='Object';for(const candidate of request._brands)if(source['is'+candidate]){brand=candidate;break;}
        if(brand==='Object')for(const [a,b,candidate] of request._structuralBrands)if(a in source&&b in source){brand=candidate;break;}
        result.$type=brand;
        for(const key of brand==='Object'?Object.keys(source).sort():request._schema[brand]){
            if(key==='id'||!(key in source))continue;const child=source[key];if(key==='uuid'&&(typeof child!=='string'||!uuids.has(child)))continue;if(brand==='Object'&&typeof child==='function')continue;result[key]=visit(child);
        }return result;
    }return visit(root);
}
// Execute BufferGeometryLoader.parse directly; metadata strings or another
// loader's postprocessing never count as evidence for the successful body.
const expected=[];
for(const request of requests){
    if(request.op==='metadata'){expected.push(metadata());continue;}
    const document=clone(request.json),events=[];
    const loader=new BufferGeometryLoader();
    if(request.variant==='dataGetter'){const data=document.data;Object.defineProperty(document,'data',{configurable:true,enumerable:true,get(){events.push('data');return data;}});}
    if(request.variant==='inherited')Object.setPrototypeOf(document.data.attributes,request.inherited);
    if(request.variant==='reenter')Object.defineProperty(document.data.attributes.normal,'data',{configurable:true,enumerable:true,get(){const nested=loader.parse(request.nested);events.push(['reenter',nested.attributes.position.data.array[0]]);return 'ib';}});
    try{expected.push({value:snapshot(request,loader.parse(document)),events});}
    catch(error){expected.push({error:error!==null&&(typeof error==='object'||typeof error==='function')?{name:error.name}:{name:'ThrownValue',raw:error},events});}
}
for(let i=0;i<requests.length;++i)assert.deepStrictEqual(expected[i].error,requests[i].expectedError,'Unexpected source result '+requests[i].case);
const output=requestedOutput??path.join('D:/Temp/surui','buffer-geometry-loader-parity-'+Date.now());assert(!fs.existsSync(output));fs.mkdirSync(output,{recursive:true});
fs.writeFileSync(path.join(output,'requests.json'),JSON.stringify(requests,null,2));fs.writeFileSync(path.join(output,'expected.json'),JSON.stringify(expected,null,2));
let actual,failures=[];
if(!sourceOnly){actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:32*1024*1024,timeout:300000,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);for(let i=0;i<requests.length;++i)try{assert.deepStrictEqual(actual[i],JSON.parse(JSON.stringify(expected[i])));}catch(error){failures.push({case:requests[i].case,difference:error.message});}}
fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify({cases:requests.length,sourceOnly,failures},null,2));process.stdout.write(JSON.stringify({output,cases:requests.length,failures:failures.length})+'\n');assert.equal(failures.length,0,failures.slice(0,3).map(row=>row.case+'\n'+row.difference).join('\n'));
