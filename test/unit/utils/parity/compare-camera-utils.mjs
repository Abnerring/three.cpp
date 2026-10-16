// Differential check for the native examples/jsm/utils/CameraUtils.js port.
//
// The pinned module is imported from the checkout, the same scenarios are replayed there and the
// camera state is compared with assert.deepEqual (no tolerances).
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-camera-utils.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
// CameraUtils.js imports 'three' (the bundle); the comparison reads the same instance so that
// the camera objects it builds and the module's duck-typed reads share one shape.
const THREE=await import(url('build/three.module.js'));
const {frameCorners}=await import(url('examples/jsm/utils/CameraUtils.js'));
const {PerspectiveCamera,Vector3}=THREE;
const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);

const scenarios=[
    {eye:[0,0,5], near:0.1, far:2000, aspect:1, estimate:false, corners:[[-1,-1,0],[1,-1,0],[-1,1,0]]},
    {eye:[0,0,5], near:0.1, far:2000, aspect:1.5, estimate:true, corners:[[-1,-1,0],[1,-1,0],[-1,1,0]]},
    {eye:[0.5,0.25,4], near:0.5, far:100, aspect:0.75, estimate:true, corners:[[-2,-1,0.5],[2,-1,0.5],[-2,3,1.5]]},
    {eye:[-3,2,7], near:0.01, far:5000, aspect:2, estimate:false, corners:[[0,0,0],[4,0,1],[0,3,-1]]},
    {eye:[1,1,1], near:1, far:2, aspect:1, estimate:true, corners:[[0,0,0],[1,0,0],[0,1,0]]},
    {eye:[0,0,0], near:0.1, far:10, aspect:1, estimate:false, corners:[[-1,-1,-5],[1,-1,-5],[-1,1,-5]]}
];
const lines=scenarios.map(s=>`F ${s.eye.map(token).join(' ')} ${token(s.near)} ${token(s.far)} ${token(s.aspect)} `
    +`${s.corners.flat().map(token).join(' ')} ${s.estimate?1:0}`);
const output=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8'}).trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
for(let index=0;index<scenarios.length;++index){
    const {eye,near,far,aspect,estimate,corners}=scenarios[index];
    const camera=new PerspectiveCamera(50,aspect,near,far);
    camera.position.set(...eye);
    frameCorners(camera,new Vector3(...corners[0]),new Vector3(...corners[1]),new Vector3(...corners[2]),estimate);
    const expected={fov:encode(camera.fov),
        quaternion:encodeArray([camera.quaternion.x,camera.quaternion.y,camera.quaternion.z,camera.quaternion.w]),
        projectionMatrix:encodeArray(camera.projectionMatrix.elements),
        projectionMatrixInverse:encodeArray(camera.projectionMatrixInverse.elements)};
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${lines[index].slice(0,120)}`);
}
console.log(`CameraUtils parity: ${scenarios.length} scenarios (fov, quaternion, the off-axis projection`
    +` matrix and its inverse) match the pinned checkout bit for bit; ${process.version}`);