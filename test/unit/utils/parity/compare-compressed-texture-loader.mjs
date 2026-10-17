// Differential check for the native src/loaders/CompressedTextureLoader.js port.
//
// Both sides subclass the loader with a parse returning the same texData document and drive load, so the
// transport and the mapping are compared together: fetch is stubbed on the oracle side and FileLoader's
// injectable transport on the native side. Upstream's load is asynchronous, so the callback is awaited
// rather than reading the texture straight after the call.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-compressed-texture-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
if(typeof globalThis.ProgressEvent==='undefined')
    globalThis.ProgressEvent=class ProgressEvent{constructor(type,init={}){this.type=type;Object.assign(this,init);}};
globalThis.fetch=async ()=>new Response(new Uint8Array([1,2,3,4]),{status:200,statusText:'OK',headers:{'content-length':'4'}});
const {CompressedTextureLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/CompressedTextureLoader.js')).href);
const documents=[
    '{"width":64,"height":32,"format":1023,"mipmapCount":3}',
    '{"width":8,"height":8,"format":1022,"mipmapCount":1}',
    '{"width":4,"height":4,"format":1023,"mipmapCount":2}'
];
const raw=execFileSync(path.resolve(probePath),[],{input:documents.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
for(let index=0;index<documents.length;++index){
    const payload=JSON.parse(documents[index]);
    class Stub extends CompressedTextureLoader{parse(){return payload;}}
    const texture=await new Promise(resolve=>{
        const created=new Stub().load('https://example.com/data.bin',()=>resolve(created));
    });
    const expected={minFilter:texture.minFilter,format:texture.format,version:texture.version};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index,documents[index]);
        console.log('  probe :',JSON.stringify(actual).slice(0,220));
        console.log('  oracle:',JSON.stringify(expected).slice(0,220));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`CompressedTextureLoader parity: ${documents.length} documents`);