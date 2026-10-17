// Differential check for the native src/extras/ImageUtils.js port.
//
// The pinned module is imported straight from the checkout; it needs no DOM for this method, because
// its canvas branch is guarded on HTMLImageElement/HTMLCanvasElement/ImageBitmap, none of which exist
// in Node. Both sides therefore take the same image.data branch and the values are compared exactly.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-image-utils.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {ImageUtils}=await import(pathToFileURL(path.join(sourceDir,'src/extras/ImageUtils.js')).href);
const kinds=[
    {kind:1,name:'Uint8',make:values=>new Uint8Array(values)},
    {kind:8,name:'Uint8Clamped',make:values=>new Uint8ClampedArray(values)},
    {kind:6,name:'Float32',make:values=>new Float32Array(values)},
    {kind:7,name:'Float64',make:values=>new Float64Array(values)}
];
// Values chosen to straddle the sRGB curve's knee at 0.04045 (10.3/255) and to reach both ends.
const samples=[0,1,10,11,64,128,200,254,255];
const scenarios=[];
for(const entry of kinds)scenarios.push({...entry,values:samples});
const lines=scenarios.map(entry=>`${entry.kind} ${entry.values.length} ${entry.values.join(' ')}`);
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
for(let index=0;index<scenarios.length;++index){
    const scenario=scenarios[index];
    const converted=ImageUtils.sRGBToLinear({data:scenario.make(scenario.values),width:3,height:1});
    const expected={data:Array.from(converted.data),width:3,height:1};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',scenario.name);
        console.log('  probe :',JSON.stringify(actual).slice(0,300));
        console.log('  oracle:',JSON.stringify(expected).slice(0,300));
    }
    assert.deepEqual(actual,expected,scenario.name);
}
// getDataURL returns the source unchanged wherever upstream would fall through to it.
const withSource=ImageUtils.getDataURL({src:'data:image/png;base64,AAAA'});
assert.equal(withSource,'data:image/png;base64,AAAA');
const plain=ImageUtils.getDataURL({src:'texture.png'});
assert.equal(plain,'texture.png');
// No data means the argument is returned as is.
const untouched={width:1,height:1};
assert.equal(ImageUtils.sRGBToLinear(untouched),untouched);
console.log(`ImageUtils parity: ${scenarios.length} scenarios plus getDataURL and the no-data case`);