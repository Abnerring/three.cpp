// Differential check for the native src/math/Matrix3.js port.
//
// Elements are compared in storage order, so a single transposed cell fails. The cases lean on what a hand
// port drifts on: the determinant and inverse of a singular matrix, inverting the identity - which produces
// signed zeros - a non-uniform scale through multiplyScalar, and the two conversions from a four by four
// matrix, where the normal matrix is the inverse transpose of the upper left corner.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-matrix3.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Matrix3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Matrix3.js')).href);
const {Matrix4}=await import(pathToFileURL(path.join(sourceDir,'src/math/Matrix4.js')).href);
const identity3=[1,0,0, 0,1,0, 0,0,1];
const singular3=[1,2,3, 2,4,6, 7,8,9];
const general3=[2,1,0, 1,3,1, 0,1,4];
const identity4=[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const scaled4=[2,0,0,0, 0,3,0,0, 0,0,4,0, 0,0,0,1];
const general4=[2,1,0,5, 1,3,1,6, 0,1,4,7, 0,0,0,1];
const scenarios=[
    ['determinant',...identity3],['determinant',...singular3],['determinant',...general3],
    ['invert',...identity3],['invert',...singular3],['invert',...general3],
    ['transpose',...general3],
    ['multiplyScalar',...general3,2],
    ['setFromMatrix4',...identity4],['setFromMatrix4',...general4],
    ['getNormalMatrix',...identity4],['getNormalMatrix',...scaled4],['getNormalMatrix',...general4]
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
const three=args=>{const m=new Matrix3();m.set(...args.slice(0,9));return m;};
const four=args=>{const m=new Matrix4();m.set(...args.slice(0,16));return m;};
const call=(op,args)=>{
    switch(op){
        case 'determinant': return three(args).determinant();
        case 'invert': return three(args).invert();
        case 'transpose': return three(args).transpose();
        case 'multiplyScalar': return three(args).multiplyScalar(args[9]);
        case 'setFromMatrix4': return new Matrix3().setFromMatrix4(four(args));
        case 'getNormalMatrix': return new Matrix3().getNormalMatrix(four(args));
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    const expected=typeof value==='number'?{value:encode(value)}:{elements:Array.from(value.elements).map(encode)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op);
        console.log('  probe :',JSON.stringify(actual).slice(0,240));
        console.log('  oracle:',JSON.stringify(expected).slice(0,240));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Matrix3 parity: ${scenarios.length} scenarios`);