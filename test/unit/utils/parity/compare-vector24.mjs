// Differential check for the native src/math/Vector2.js and Vector4.js ports.
//
// One call per scenario through the pinned modules, on the inputs a hand port drifts on: normalising a zero
// vector, the angle to itself, min and max with NaN, a reversed clamp range, the angle measure itself - which
// upstream reports in the range zero to pi - and a matrix applied to a four component vector, where w decides
// whether the result is projected.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-vector24.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Vector2}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector2.js')).href);
const {Vector4}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector4.js')).href);
const {Matrix4}=await import(pathToFileURL(path.join(sourceDir,'src/math/Matrix4.js')).href);
const identity=[1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1];
const translate=[1,0,0,1, 0,1,0,2, 0,0,1,0, 0,0,0,1];
const scenarios=[
    ['v2.dot',3,4,1,2],['v2.cross',3,4,1,2],['v2.length',3,4],
    ['v2.normalize',0,0],['v2.normalize',3,4],['v2.angle',1,0],['v2.angle',-1,0],
    ['v2.angleTo',1,0,0,1],['v2.angleTo',0,0,0,0],['v2.distanceTo',1,2,1,2],
    ['v2.min',NaN,5,2,NaN],['v2.max',NaN,5,2,NaN],
    ['v2.clamp',5,-5,-1,-1,1,1],['v2.clamp',0.5,0.5,1,1,-1,-1],
    ['v2.lerp',0,0,10,20,0.25],
    ['v4.dot',1,2,3,4,5,6,7,8],['v4.length',1,2,2,4],
    ['v4.normalize',0,0,0,0],['v4.normalize',1,2,2,4],
    ['v4.min',NaN,2,3,4,1,NaN,3,4],['v4.max',NaN,2,3,4,1,NaN,3,4],
    ['v4.applyMatrix4',1,2,3,1,...identity],
    ['v4.applyMatrix4',1,2,3,1,...translate],
    ['v4.applyMatrix4',1,2,3,2,...translate],
    ['v4.lerp',0,0,0,0,1,2,3,4,0.5]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(parts=>parts.map(v=>Number.isNaN(v)?'NaN':v).join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const call=(op,args)=>{
    const two=new Vector2(args[0],args[1]);
    const four=new Vector4(args[0],args[1],args[2],args[3]);
    switch(op){
        case 'v2.dot': return two.dot(new Vector2(args[2],args[3]));
        case 'v2.cross': return two.cross(new Vector2(args[2],args[3]));
        case 'v2.length': return two.length();
        case 'v2.normalize': return two.normalize();
        case 'v2.angle': return two.angle();
        case 'v2.angleTo': return two.angleTo(new Vector2(args[2],args[3]));
        case 'v2.distanceTo': return two.distanceTo(new Vector2(args[2],args[3]));
        case 'v2.min': return two.min(new Vector2(args[2],args[3]));
        case 'v2.max': return two.max(new Vector2(args[2],args[3]));
        case 'v2.clamp': return two.clamp(new Vector2(args[2],args[3]),new Vector2(args[4],args[5]));
        case 'v2.lerp': return two.lerp(new Vector2(args[2],args[3]),args[4]);
        case 'v4.dot': return four.dot(new Vector4(args[4],args[5],args[6],args[7]));
        case 'v4.length': return four.length();
        case 'v4.normalize': return four.normalize();
        case 'v4.min': return four.min(new Vector4(args[4],args[5],args[6],args[7]));
        case 'v4.max': return four.max(new Vector4(args[4],args[5],args[6],args[7]));
        case 'v4.applyMatrix4': { const m=new Matrix4(); m.set(...args.slice(4,20)); return four.applyMatrix4(m); }
        case 'v4.lerp': return four.lerp(new Vector4(args[4],args[5],args[6],args[7]),args[8]);
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    const expected=typeof value==='number'?{value:encode(value)}
        :op.startsWith('v2.')?{x:encode(value.x),y:encode(value.y)}
        :{x:encode(value.x),y:encode(value.y),z:encode(value.z),w:encode(value.w)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Vector2/Vector4 parity: ${scenarios.length} scenarios`);