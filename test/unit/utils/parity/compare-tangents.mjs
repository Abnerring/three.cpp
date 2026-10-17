// Differential check for the native BufferGeometry.computeTangents port.
//
// Both sides parse the same geometry document with their own loader, compute the tangents and compare the
// attribute as float32 bit patterns. The documents cover a shared-vertex quad, a single triangle, a quad
// split by groups, a triangle with collinear uvs - which the algorithm skips - and a document missing the
// uvs, where upstream reports the problem and returns without creating the attribute.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-tangents.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {BufferGeometryLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/BufferGeometryLoader.js')).href);
const quad={
    data:{
        index:{type:'Uint16Array',array:[0,1,2,2,1,3]},
        attributes:{
            position:{type:'Float32Array',array:[0,0,0, 1,0,0, 0,1,0, 1,1,0],itemSize:3},
            normal:{type:'Float32Array',array:[0,0,1, 0,0,1, 0,0,1, 0,0,1],itemSize:3},
            uv:{type:'Float32Array',array:[0,0, 1,0, 0,1, 1,1],itemSize:2}
        }
    }
};
const withGroups=JSON.parse(JSON.stringify(quad));
withGroups.data.groups=[{start:0,count:3},{start:3,count:3,materialIndex:1}];
const degenerate=JSON.parse(JSON.stringify(quad));
degenerate.data.attributes.uv={type:'Float32Array',array:[0,0, 1,0, 2,0, 3,0],itemSize:2};
const missingUv=JSON.parse(JSON.stringify(quad));
delete missingUv.data.attributes.uv;
const documents=[quad,withGroups,degenerate,missingUv];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const bitsOf=array=>{
    const view=new DataView(new ArrayBuffer(4));
    const parts=[];
    for(const value of array){view.setFloat32(0,value);parts.push(view.getUint32(0).toString(16).padStart(8,'0'));}
    return parts.join(' ');
};
const raw=execFileSync(path.resolve(probePath),[],{input:documents.map(d=>JSON.stringify(d)).join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
for(let index=0;index<documents.length;++index){
    const geometry=new BufferGeometryLoader().parse(JSON.parse(JSON.stringify(documents[index])));
    geometry.computeTangents();
    const tangent=geometry.attributes.tangent;
    const expected=tangent===undefined?{present:false}:{present:true,count:tangent.count,itemSize:tangent.itemSize,bits:bitsOf(tangent.array)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index);
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`computeTangents parity: ${documents.length} documents`);