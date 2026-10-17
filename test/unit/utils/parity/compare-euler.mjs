// Differential check for the native src/math/Euler.js port.
//
// The scenarios cover the two conversions reorder goes through, and reorder itself across the order
// permutations, including the singular gimbal-lock branch and a signed zero that must survive.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-euler.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Euler}=await import(pathToFileURL(path.join(sourceDir,'src/math/Euler.js')).href);
const {Quaternion}=await import(pathToFileURL(path.join(sourceDir,'src/math/Quaternion.js')).href);
const {Matrix4}=await import(pathToFileURL(path.join(sourceDir,'src/math/Matrix4.js')).href);
const identity=[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const rotated=[0,0,-1,0, 0,1,0,0, 1,0,0,0, 0,0,0,1];
const halfPI=Math.PI/2;
const scenarios=[
    ['euler.reorder','XYZ','YXZ',0.3,0.4,0.5],
    ['euler.reorder','XYZ','ZYX',0.3,0.4,0.5],
    ['euler.reorder','YXZ','XYZ',0.3,0.4,0.5],
    ['euler.reorder','ZXY','YXZ',-0.3,0.4,-0.5],
    ['euler.reorder','XYZ','XYZ',0,0,0],
    ['euler.setFromQuaternion','XYZ',0,0,0,1],
    ['euler.setFromQuaternion','YXZ',0,0,0,-1],
    ['euler.setFromQuaternion','XYZ',0.5,0.5,0.5,0.5],
    ['euler.setFromQuaternion','XYZ',0,halfPI,0,1],
    ['euler.setFromQuaternion','ZXY',0.2,-0.7,0.1,0.3],
    ['euler.setFromRotationMatrix','XYZ',...identity],
    ['euler.setFromRotationMatrix','XYZ',...rotated],
    ['euler.setFromRotationMatrix','YXZ',...rotated]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const toToken=value=>Number.isNaN(value)?'NaN':String(value);
const lines=scenarios.map(parts=>parts.map(toToken).join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const result=e=>({x:encode(e.x),y:encode(e.y),z:encode(e.z),order:e.order});
for(let index=0;index<scenarios.length;++index){
    const [op,a,b,c,d,e]=scenarios[index];
    let expected;
    if(op==='euler.reorder'){
        const euler=new Euler(c,d,e,a);
        expected=result(euler.reorder(b));
    }else if(op==='euler.setFromQuaternion'){
        const euler=new Euler(0,0,0,a);
        expected=result(euler.setFromQuaternion(new Quaternion(b,c,d,e)));
    }else{
        const matrix=new Matrix4();
        matrix.set(...scenarios[index].slice(2,18));
        const euler=new Euler(0,0,0,a);
        expected=result(euler.setFromRotationMatrix(matrix));
    }
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,JSON.stringify(scenarios[index].slice(0,3)));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Euler parity: ${scenarios.length} scenarios`);