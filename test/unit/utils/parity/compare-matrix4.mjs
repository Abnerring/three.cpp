// Differential check for the native src/math/Matrix4.js port.
//
// Elements are compared in storage order, so a single transposed cell fails. The cases lean on what a
// hand port drifts on: the determinant and inverse of a singular matrix, inverting the identity, a
// non-uniform scale through compose and back through decompose, and a negative determinant.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-matrix4.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Matrix4}=await import(pathToFileURL(path.join(sourceDir,'src/math/Matrix4.js')).href);
const {Quaternion}=await import(pathToFileURL(path.join(sourceDir,'src/math/Quaternion.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const identity=[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const singular=[1,2,3,4, 2,4,6,8, 0,0,1,0, 0,0,0,1];
const general=[2,0,0,1, 0,3,0,2, 0,0,4,3, 1,2,3,1];
const scaled=[2,0,0,0, 0,2,0,0, 0,0,2,0, 0,0,0,1];
const reflected=[-1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const scenarios=[
    ['determinant',...identity],
    ['determinant',...singular],
    ['determinant',...general],
    ['determinant',...reflected],
    ['invert',...identity],
    ['invert',...singular],
    ['invert',...general],
    ['invert',...scaled],
    ['transpose',...general],
    ['multiplyScalar',...general,2],
    ['compose',1,2,3, 0,0,0,1, 1,1,1],
    ['compose',1,2,3, 0.5,0.5,0.5,0.5, 2,3,4],
    ['decompose',...general],
    ['decompose',...scaled],
    ['decompose',...reflected]
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
const fromArgs=args=>{
    const m=new Matrix4();
    for(let row=0;row<4;++row)
        for(let column=0;column<4;++column)
            m.elements[column*4+row]=args[row*4+column]??0;
    return m;
};
const call=(op,args)=>{
    switch(op){
        case 'determinant': return fromArgs(args).determinant();
        case 'invert': return fromArgs(args).invert();
        case 'transpose': return fromArgs(args).transpose();
        case 'multiplyScalar': return fromArgs(args).multiplyScalar(args[16]);
        case 'compose':{
            const m=new Matrix4();
            m.compose(new Vector3(args[0],args[1],args[2]),new Quaternion(args[3],args[4],args[5],args[6]),new Vector3(args[7],args[8],args[9]));
            return m;
        }
        case 'decompose':{
            const position=new Vector3(),scale=new Vector3(),quaternion=new Quaternion();
            fromArgs(args).decompose(position,quaternion,scale);
            return {position,quaternion,scale};
        }
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    let expected;
    if(typeof value==='number') expected={value:encode(value)};
    else if(value.elements) expected={elements:Array.from(value.elements).map(encode)};
    else expected={
        px:encode(value.position.x),py:encode(value.position.y),pz:encode(value.position.z),
        qx:encode(value.quaternion.x),qy:encode(value.quaternion.y),qz:encode(value.quaternion.z),qw:encode(value.quaternion.w),
        sx:encode(value.scale.x),sy:encode(value.scale.y),sz:encode(value.scale.z)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.slice(0,16).join(' '));
        console.log('  probe :',JSON.stringify(actual).slice(0,300));
        console.log('  oracle:',JSON.stringify(expected).slice(0,300));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Matrix4 parity: ${scenarios.length} scenarios`);