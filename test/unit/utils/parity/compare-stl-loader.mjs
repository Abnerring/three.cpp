// Differential check for the native examples/jsm/loaders/STLLoader.js binary path.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-stl-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const source=fs.readFileSync(path.join(sourceDir,'examples/jsm/loaders/STLLoader.js'),'utf8')
    .replace(/from 'three'/g,`from '${pathToFileURL(path.join(sourceDir,'src/Three.js')).href}'`);
const {STLLoader}=await import('data:text/javascript;base64,'+Buffer.from(source).toString('base64'));
// The documents are built here and sent as base64, so both sides see identical bytes.
const build=(faces,header,packed=[])=>{
    const chunks=[Buffer.from(header,'latin1'),Buffer.alloc(80-header.length)];
    const count=Buffer.alloc(4); count.writeUInt32LE(faces); chunks.push(count);
    for(let face=0;face<faces;++face){
        const record=Buffer.alloc(50);
        record.writeFloatLE(0,0); record.writeFloatLE(0,4); record.writeFloatLE(1,8);
        record.writeFloatLE(0,12); record.writeFloatLE(0,16); record.writeFloatLE(0,20);
        record.writeFloatLE(1,24); record.writeFloatLE(0,28); record.writeFloatLE(0,32);
        record.writeFloatLE(0,36); record.writeFloatLE(1,40); record.writeFloatLE(0,44);
        record.writeUInt16LE(packed[face]??0,48);
        chunks.push(record);
    }
    return Buffer.concat(chunks);
};
const magics=(()=>{
    const header=Buffer.alloc(80);
    header.write('solid COLOR=',0,'latin1');
    header.writeUInt8(200,12); header.writeUInt8(100,13); header.writeUInt8(50,14); header.writeUInt8(255,15);
    return header;
})();
const documents=[
    build(1,'solid test'),
    build(2,'header'),
    Buffer.concat([magics,build(1,'',{length:1}).subarray(80)]),
    build(1,'solid COLOR=',[0x8001]),
    build(0,'solid empty')
];
const patterns=array=>{
    const parts=[];
    for(let index=0;index<array.length;index+=1){
        const view=new DataView(new ArrayBuffer(4));
        view.setFloat32(0,array[index]);
        parts.push(view.getUint32(0).toString(16).padStart(8,'0'));
    }
    return parts.join(' ');
};
const lines=documents.map(document=>document.toString('base64'));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
for(let index=0;index<documents.length;++index){
    const bytes=documents[index];
    const geometry=new STLLoader().parse(bytes.buffer.slice(bytes.byteOffset,bytes.byteOffset+bytes.byteLength));
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
        console.log('MISMATCH document',index,'base64',lines[index].slice(0,40));
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`STLLoader parity: ${documents.length} documents, float32 bits compared`);