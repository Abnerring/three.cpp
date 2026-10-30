// Differential check for the native src/math/Frustum.js port.
//
// The frustum is built from the same sixteen numbers on both sides, so the comparison covers the plane
// extraction as well as the queries that use it. An identity projection gives the unit box, which makes
// the expected answers readable, and a translated one moves it.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-frustum.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Frustum}=await import(pathToFileURL(path.join(sourceDir,'src/math/Frustum.js')).href);
const {Matrix4}=await import(pathToFileURL(path.join(sourceDir,'src/math/Matrix4.js')).href);
const {Box3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Box3.js')).href);
const {Sphere}=await import(pathToFileURL(path.join(sourceDir,'src/math/Sphere.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const identity=[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const translated=[1,0,0,5, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const scaled=[2,0,0,0, 0,2,0,0, 0,0,2,0, 0,0,0,1];
const scenarios=[
    ['planes',...identity],
    ['planes',...scaled],
    ['containsPoint',...identity, 0,0,0],
    ['containsPoint',...identity, 5,0,0],
    ['containsPoint',...identity, 0,0,-5],
    ['intersectsBox',...identity, -1,-1,-1, 1,1,1],
    ['intersectsBox',...identity, 0.5,0.5,0.5, 2,2,2],
    ['intersectsBox',...identity, 10,10,10, 11,11,11],
    ['intersectsSphere',...identity, 0,0,0, 1],
    ['intersectsSphere',...identity, 0,0,0, 0.2],
    ['intersectsSphere',...identity, 10,0,0, 1],
    ['containsPoint',...translated, 5,0,0],
    ['containsPoint',...translated, 0,0,0],
    ['intersectsBox',...scaled, -1,-1,-1, 1,1,1]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(parts=>parts.join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const frustumOf=args=>{
    const matrix=new Matrix4();
    matrix.set(...args.slice(0,16));
    return new Frustum().setFromProjectionMatrix(matrix);
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const frustum=frustumOf(args);
    let expected;
    if(op==='containsPoint')expected={value:frustum.containsPoint(new Vector3(args[16],args[17],args[18]))};
    else if(op==='intersectsBox')expected={value:frustum.intersectsBox(new Box3(new Vector3(args[16],args[17],args[18]),new Vector3(args[19],args[20],args[21])))};
    else if(op==='intersectsSphere')expected={value:frustum.intersectsSphere(new Sphere(new Vector3(args[16],args[17],args[18]),args[19]))};
    else expected={constants:frustum.planes.map(plane=>encode(plane.constant))};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.slice(0,16).join(' '));
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Frustum parity: ${scenarios.length} scenarios`);