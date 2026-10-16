// Differential check for the native examples/jsm/loaders/XYZLoader.js port.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-xyz-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const source=fs.readFileSync(path.join(sourceDir,'examples/jsm/loaders/XYZLoader.js'),'utf8')
    .replace(/from 'three'/g,`from '${pathToFileURL(path.join(sourceDir,'src/Three.js')).href}'`);
const {XYZLoader}=await import('data:text/javascript;base64,'+Buffer.from(source).toString('base64'));
// The line protocol carries one document per line with "|" for a newline, so the empty document is
// spelled as a single newline: a truly empty final line would not be read back by the probe.
const documents=[
    '1 2 3\n4 5 6\n',
    '# comment\n\n1 2 3\n1 2 3 4\n1 2 3 4 5\n\n7 8 9\n',
    '0 0 0 255 0 0\n1 1 1 0 255 0\n',
    '1e3 .5 1.2.3\nInfinity garbage -Infinity\n',
    '   \t 1\t\t2   3  \n',
    '0.1 0.2 0.3\n',
    '128 128 128 128 128 128\n',
    '1 2 3\r\n',
    '\n'
];
const toLine=document=>document.replace(/\n/g,'|');
const raw=execFileSync(path.resolve(probePath),[],{input:documents.map(toLine).join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
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
    const geometry=new XYZLoader().parse(documents[index]);
    // The pinned geometry exposes its attributes as properties; only the native side reads them
    // through a JSValue handle.
    const attributes=geometry.attributes;
    const color=attributes.color;
    const expected={
        positionCount:attributes.position.count,
        hasColor:color!==undefined&&color!==null,
        position:patterns(attributes.position.array),
        color:color?patterns(color.array):'absent'
    };
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index,JSON.stringify(toLine(documents[index])));
        console.log('  probe :',JSON.stringify(actual).slice(0,300));
        console.log('  oracle:',JSON.stringify(expected).slice(0,300));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`XYZLoader parity: ${documents.length} documents, float32 bits compared`);