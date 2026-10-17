// Differential check for the native src/scenes/Fog.js and src/textures/ExternalTexture.js ports.
//
// Both are thin, so what is compared is the visible state: the colour and the two distances of a fog and of
// its clone, and the flags and image size of an external texture, with and without a source image.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-fog-external.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Fog}=await import(pathToFileURL(path.join(sourceDir,'src/scenes/Fog.js')).href);
const {ExternalTexture}=await import(pathToFileURL(path.join(sourceDir,'src/textures/ExternalTexture.js')).href);
const scenarios=[
    ['fog',0x00ff00,1,100],
    ['fog',0x000000,0,0],
    ['fog',0x336699,2.5,10],
    ['fog.clone',0x00ff00,1,100],
    ['external',null,4,2],
    ['external.default',null,0,0]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(parts=>parts.map(v=>v===null?'0':v).join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const fogState=fog=>({near:encode(fog.near),far:encode(fog.far),isFog:fog.isFog===true,
    red:encode(fog.color.r),green:encode(fog.color.g),blue:encode(fog.color.b)});
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    let expected;
    if(op==='fog')expected=fogState(new Fog(args[0],args[1],args[2]));
    else if(op==='fog.clone')expected=fogState(new Fog(args[0],args[1],args[2]).clone());
    else if(op==='external'){
        // Upstream only records the source texture; it does not attach an image, so the size stays unset.
        const texture=new ExternalTexture();
        expected={isExternalTexture:texture.isExternalTexture===true,
            width:encode(texture.image?texture.image.width:-1),height:encode(texture.image?texture.image.height:-1),
            version:encode(texture.version)};
    } else {
        const texture=new ExternalTexture();
        expected={isExternalTexture:texture.isExternalTexture===true,
            width:encode(texture.image?texture.image.width:-1),height:encode(texture.image?texture.image.height:-1),
            version:encode(texture.version)};
    }
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Fog/ExternalTexture parity: ${scenarios.length} scenarios`);