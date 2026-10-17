// Differential check for the native src/math/Vector3.js port.
//
// One call per scenario, driven through the pinned module, on the inputs a hand port drifts on:
// normalising a zero vector, angle and distance to itself, min and max with NaN, a zero-length
// projection plane, and the spherical conversion with a zero radius.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-vector3.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const {Quaternion}=await import(pathToFileURL(path.join(sourceDir,'src/math/Quaternion.js')).href);
const scenarios=[
    ['cross',1,2,3,4,5,6],
    ['dot',1,2,3,4,5,6],
    ['length',3,4,0],
    ['normalize',0,0,0],
    ['normalize',3,4,0],
    ['lerp',0,0,0,10,20,30,0.25],
    ['angleTo',1,0,0,0,1,0],
    ['angleTo',0,0,0,0,0,0],
    ['distanceTo',1,2,3,1,2,3],
    ['clamp',5,-5,10,-1,-1,-1,1,1,1],
    ['min',NaN,2,3,1,NaN,5],
    ['max',NaN,2,3,1,NaN,5],
    ['applyQuaternion',1,0,0,0,0,0,1],
    ['applyQuaternion',1,2,3,0.5,0.5,0.5,0.5],
    ['projectOnPlane',1,2,3,0,0,0],
    ['projectOnPlane',1,2,3,0,1,0],
    ['reflect',1,-1,0,0,1,0],
    ['setFromSphericalCoords',0,0,0],
    ['setFromSphericalCoords',2,Math.PI/3,Math.PI/4]
];
const encode=value=>{
    if(typeof value!=='number')return value;
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(([op,...args])=>`${op} ${args.map(v=>Number.isNaN(v)?'NaN':v).join(' ')}`);
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const call=(op,args)=>{
    const v=new Vector3(args[0],args[1],args[2]);
    switch(op){
        case 'cross': return new Vector3().crossVectors(v,new Vector3(args[3],args[4],args[5]));
        case 'dot': return v.dot(new Vector3(args[3],args[4],args[5]));
        case 'length': return v.length();
        case 'normalize': return v.normalize();
        case 'lerp': return v.lerp(new Vector3(args[3],args[4],args[5]),args[6]);
        case 'angleTo': return v.angleTo(new Vector3(args[3],args[4],args[5]));
        case 'distanceTo': return v.distanceTo(new Vector3(args[3],args[4],args[5]));
        case 'clamp': return v.clamp(new Vector3(args[3],args[4],args[5]),new Vector3(args[6],args[7],args[8]));
        case 'min': return v.min(new Vector3(args[3],args[4],args[5]));
        case 'max': return v.max(new Vector3(args[3],args[4],args[5]));
        case 'applyQuaternion': return v.applyQuaternion(new Quaternion(args[3],args[4],args[5],args[6]));
        case 'projectOnPlane': return v.projectOnPlane(new Vector3(args[3],args[4],args[5]));
        case 'reflect': return v.reflect(new Vector3(args[3],args[4],args[5]));
        case 'setFromSphericalCoords': return new Vector3().setFromSphericalCoords(args[0],args[1],args[2]);
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    const expected=typeof value==='number'
        ? {value:encode(value)}
        : {x:encode(value.x),y:encode(value.y),z:encode(value.z)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op}(${args.join(', ')})`);
}
console.log(`Vector3 parity: ${scenarios.length} scenarios`);