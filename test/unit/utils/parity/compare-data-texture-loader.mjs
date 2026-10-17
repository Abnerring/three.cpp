// Differential check for the native src/loaders/DataTextureLoader.js port.
//
// Both sides subclass the loader with a parse that returns the same texData document, then call
// createDataTexture and compare the properties the mapping controls. No file transport is involved, so
// the comparison covers the mapping itself rather than the loading plumbing.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-data-texture-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {DataTextureLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/DataTextureLoader.js')).href);
const documents=[
    '{"width":4,"height":2}',
    '{"width":8,"height":8,"wrapS":1001,"wrapT":1000,"magFilter":1006,"minFilter":1008,"anisotropy":4,"flipY":true,"format":1023,"type":1016,"colorSpace":"srgb","generateMipmaps":true}',
    '{"width":1,"height":1,"minFilter":1008,"mipmapCount":1}',
    '{"image":{"width":16,"height":16}}'
];
const raw=execFileSync(path.resolve(probePath),[],{input:documents.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
for(let index=0;index<documents.length;++index){
    const payload=JSON.parse(documents[index]);
    class Stub extends DataTextureLoader{parse(){return payload;}}
    const texture=new Stub().createDataTexture(new ArrayBuffer(0));
    const expected={
        wrapS:texture.wrapS,wrapT:texture.wrapT,magFilter:texture.magFilter,minFilter:texture.minFilter,
        anisotropy:texture.anisotropy,flipY:texture.flipY,format:texture.format,type:texture.type,
        colorSpace:texture.colorSpace,generateMipmaps:texture.generateMipmaps,version:texture.version
    };
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index,documents[index].slice(0,120));
        console.log('  probe :',JSON.stringify(actual).slice(0,260));
        console.log('  oracle:',JSON.stringify(expected).slice(0,260));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`DataTextureLoader parity: ${documents.length} documents`);