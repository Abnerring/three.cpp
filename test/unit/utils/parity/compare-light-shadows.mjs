// Differential check for the native src/lights/DirectionalLightShadow.js and PointLightShadow.js ports.
//
// What these classes decide is the camera they build and the defaults they inherit, so the comparison is the
// camera parameters plus the inherited shadow fields, for each class and for its clone.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-light-shadows.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {DirectionalLightShadow}=await import(pathToFileURL(path.join(sourceDir,'src/lights/DirectionalLightShadow.js')).href);
const {PointLightShadow}=await import(pathToFileURL(path.join(sourceDir,'src/lights/PointLightShadow.js')).href);
const cases=['directional','point','directional.clone','point.clone'];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const state=(shadow,directional)=>{
    const camera=shadow.camera;
    const cameraState=directional
        ?{left:encode(camera.left),right:encode(camera.right),top:encode(camera.top),bottom:encode(camera.bottom),
           near:encode(camera.near),far:encode(camera.far)}
        :{fov:encode(camera.fov),aspect:encode(camera.aspect),near:encode(camera.near),far:encode(camera.far)};
    return {directional,bias:encode(shadow.bias),normalBias:encode(shadow.normalBias),radius:encode(shadow.radius),
        blurSamples:encode(shadow.blurSamples),intensity:encode(shadow.intensity),
        mapSizeX:encode(shadow.mapSize.x),mapSizeY:encode(shadow.mapSize.y),camera:cameraState};
};
const raw=execFileSync(path.resolve(probePath),[],{input:cases.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,cases.length,'one probe line per case');
for(let index=0;index<cases.length;++index){
    const name=cases[index];
    const directional=name.startsWith('directional');
    let shadow=directional?new DirectionalLightShadow():new PointLightShadow();
    if(name.endsWith('.clone'))shadow=shadow.clone();
    const expected=state(shadow,directional);
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',name);
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,name);
}
console.log(`Light shadows parity: ${cases.length} cases`);