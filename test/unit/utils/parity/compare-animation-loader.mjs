// Differential check for the native src/loaders/AnimationLoader.js port.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-animation-loader.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
// Imported straight from the checkout: this module and its transitive imports are all relative
// within src, so nothing needs the data URL rewrite that the bare "three" specifier requires.
const {AnimationLoader}=await import(pathToFileURL(path.join(sourceDir,'src/loaders/AnimationLoader.js')).href);
// Each line is one serialized clip list. Clips carry no tracks here: parsing a track needs
// KeyframeTrack.parse, which is a separate unit, and empty tracks already exercise the loop and the
// geometry of the returned array.
const documents=[
    '[]',
    '[{"name":"a","duration":1.5,"tracks":[]}]',
    '[{"name":"b","duration":2,"tracks":[]},{"name":"c","duration":0.25,"tracks":[]}]',
    '[{"name":"nan","duration":1e999,"tracks":[]}]'
];
const durationBits=value=>{
    const view=new DataView(new ArrayBuffer(8));
    view.setFloat64(0,value);
    return view.getBigUint64(0).toString(16).padStart(16,'0');
};
const raw=execFileSync(path.resolve(probePath),[],{input:documents.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,documents.length,'one probe line per document');
for(let index=0;index<documents.length;++index){
    const clips=new AnimationLoader().parse(JSON.parse(documents[index]));
    const expected={count:clips.length,clips:clips.map(clip=>({name:clip.name,tracks:clip.tracks.length,durationBits:durationBits(clip.duration)}))};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH document',index,documents[index]);
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`document ${index}`);
}
console.log(`AnimationLoader parity: ${documents.length} documents, float64 duration bits compared`);