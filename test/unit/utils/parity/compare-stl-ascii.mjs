// ASCII coverage for the native examples/jsm/loaders/STLLoader.js port, kept in its own file so the
// binary differential stays untouched. The probe reads base64 lines and decodes them to bytes, so
// ASCII text travels through the same protocol the binary documents use; isBinary then recognises
// the solid prefix and the ASCII half parses it.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-stl-ascii.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const source=fs.readFileSync(path.join(sourceDir,'examples/jsm/loaders/STLLoader.js'),'utf8')
    .replace(/from 'three'/g,`from '${pathToFileURL(path.join(sourceDir,'src/Three.js')).href}'`);
const {STLLoader}=await import('data:text/javascript;base64,'+Buffer.from(source).toString('base64'));
const documents=[
    'solid test\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid test\n',
    'solid a\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid a\n' +
        'solid b\nfacet normal 0 1 0\nouter loop\nvertex 2 0 0\nvertex 3 0 0\nvertex 2 1 0\nendloop\nendfacet\nendsolid b\n',
    // A facet whose lines are neither the XYZ nor the XYZRGB style: both sides ignore them, so the
    // geometry has no vertices at all.
    'solid odd\nfacet normal 0 0 1\nouter loop\nvertex 1 2 3\nvertex 1 2 3 4\nendloop\nendfacet\nendsolid odd\n',
    // A facet with no normal line: upstream keeps the zero vector it started with, which is what the
    // normals array then holds for those vertices.
    'solid nonormal\nfacet\nouter loop\nvertex 0 0 0\nvertex 1 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid nonormal\n'
];
const lines=documents.map(text=>Buffer.from(text,'latin1').toString('base64'));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
const patterns=array=>{
    const parts=[];
    for(let index=0;index<array.length;index+=1){
        const view=new DataView(new ArrayBuffer(4));
        view.setFloat32(0,array[index]);
        parts.push(view.getUint32(0).toString(16).padStart(8,'0'));
    }
    return parts.join(' ');
};
for(let index=0;index<documents.length;++index){
    // The oracle accepts the text itself: parse calls ensureBinary, which maps a string to bytes one
    // code unit at a time, exactly as the port does.
    const geometry=new STLLoader().parse(documents[index]);
    const attributes=geometry.attributes;
    const color=attributes.color;
    const expected={
        hasColors:geometry.hasColors===true,
        alpha:geometry.alpha===undefined?-1:geometry.alpha,
        position:patterns(attributes.position.array),
        normal:patterns(attributes.normal.array),
        color:color?patterns(color.array):'absent'
    };
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index);
        console.log('  text  :',JSON.stringify(documents[index]));
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`STLLoader ASCII parity: ${documents.length} documents, float32 bits compared`);
