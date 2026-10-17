// Differential check for the native src/loaders/BufferGeometryLoader.js port.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-buffer-geometry-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
// Imported straight from the checkout: this module and everything it imports are relative within
// src, so the bare "three" specifier rewrite that other comparisons need does not apply.
const {BufferGeometryLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/BufferGeometryLoader.js')).href);
const documents=[
    '{"data":{"attributes":{"position":{"type":"Float32Array","array":[0,1,2,3,4,5],"itemSize":3}}}}',
    '{"data":{"attributes":{"position":{"type":"Float32Array","array":[0,0,0,1,0,0,0,1,0],"itemSize":3}},"index":{"type":"Uint16Array","array":[0,1,2]}}}',
    '{"data":{"attributes":{"position":{"type":"Float32Array","array":[0,0,0],"itemSize":3}},"morphAttributes":{"position":[{"type":"Float32Array","array":[1,0,0],"itemSize":3}]},"morphTargetsRelative":true,"groups":[{"start":0,"count":1,"materialIndex":2}]}}',
    // The shared-interleaved document from the unit test: two attributes naming one buffer.
    '{"isInstancedBufferGeometry":true,"instanceCount":7,"data":{"attributes":{"position":{"isInterleavedBufferAttribute":true,"data":"ib","itemSize":3,"offset":0},"uv":{"isInterleavedBufferAttribute":true,"data":"ib","itemSize":2,"offset":1}},"interleavedBuffers":{"ib":{"uuid":"ib","buffer":"ab","type":"Float32Array","stride":3,"usage":35048}},"arrayBuffers":{"ab":[1065353216,1073741824,1077936128]}}}'
];
const bits=array=>{
    const parts=[];
    for(let index=0;index<array.length;index+=1){
        const view=new DataView(new ArrayBuffer(4));
        view.setFloat32(0,array[index]);
        parts.push(view.getUint32(0).toString(16).padStart(8,'0'));
    }
    return parts.join(' ');
};
const raw=execFileSync(path.resolve(probePath),[],{input:documents.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
for(let index=0;index<documents.length;++index){
    const geometry=new BufferGeometryLoader().parse(JSON.parse(documents[index]));
    const attributes=geometry.attributes;
    const expected={
        attributes:Object.keys(attributes).map(name=>({name,itemSize:attributes[name].itemSize,count:attributes[name].count,bits:bits(attributes[name].array)})),
        hasIndex:geometry.index!==undefined&&geometry.index!==null,
        morph:geometry.morphAttributes.position===undefined?0:geometry.morphAttributes.position.length,
        groups:geometry.groups===undefined?0:geometry.groups.length,
        morphRelative:geometry.morphTargetsRelative===true
    };
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index);
        console.log('  json  :',documents[index].slice(0,160));
        console.log('  probe :',JSON.stringify(actual).slice(0,260));
        console.log('  oracle:',JSON.stringify(expected).slice(0,260));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`BufferGeometryLoader parity: ${documents.length} documents, float32 bits compared`);
