// Differential check for the native src/math/Quaternion.js port.
//
// One call per scenario through the pinned module, on the branches a hand port drifts on: normalising a
// zero quaternion, the angle to itself, inverting a non-unit quaternion, and slerp across the antipodal
// pair, where three interpolates by negating rather than dividing by a vanishing sine.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-quaternion.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Quaternion}=await import(pathToFileURL(path.join(sourceDir,'src/math/Quaternion.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const scenarios=[
    ['dot',0,0,0,1,0,0,0,1],
    ['length',0,0,0,2],
    ['length',1,2,3,4],
    ['normalize',0,0,0,0],
    ['normalize',1,2,3,4],
    ['multiply',0,0,0,1,0,0,0,1],
    ['multiply',0.5,0.5,0.5,0.5,0.5,0.5,0.5,0.5],
    ['invert',0,0,0,1],
    ['invert',1,2,3,4],
    ['conjugate',1,2,3,4],
    ['angleTo',0,0,0,1,0,0,0,1],
    ['angleTo',0,0,0,1,0,0,0,-1],
    ['slerp',0,0,0,1,0,0,0,-1,0.5],
    ['slerp',0,0,0,1,0,0,0,1,0.5],
    ['slerp',0,0,0,1,0.5,0.5,0.5,0.5,0.25],
    ['setFromAxisAngle',0,1,0,Math.PI/2],
    ['setFromAxisAngle',0,0,0,1]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const num=value=>Number.isNaN(value)?'NaN':value;
const lines=scenarios.map(([op,...args])=>`${op} ${args.map(num).join(' ')}`);
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const call=(op,args)=>{
    const first=new Quaternion(args[0],args[1],args[2],args[3]);
    const second=new Quaternion(args[4],args[5],args[6],args[7]);
    switch(op){
        case 'dot': return first.dot(second);
        case 'length': return first.length();
        case 'normalize': return first.normalize();
        case 'multiply': return first.multiply(second);
        case 'invert': return first.invert();
        case 'conjugate': return first.conjugate();
        case 'angleTo': return first.angleTo(second);
        case 'slerp': return first.slerp(second,args[8]);
        case 'setFromAxisAngle': return new Quaternion().setFromAxisAngle(new Vector3(args[0],args[1],args[2]),args[3]);
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    const expected=typeof value==='number'
        ? {value:encode(value)}
        : {x:encode(value.x),y:encode(value.y),z:encode(value.z),w:encode(value.w)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op}(${args.join(', ')})`);
}
console.log(`Quaternion parity: ${scenarios.length} scenarios`);