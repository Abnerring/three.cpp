// Differential check for the native Color sRGB conversions and the MathUtils helpers that
// the pinned modules implement with Math.pow/exp/log/log2.
//
// The pinned classes are imported from the checkout, the same scenarios are replayed, and
// every result is compared with assert.deepEqual (no tolerances).
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-color-math.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const THREE=await import(url('build/three.module.js'));
const {Color}=THREE;
const MathUtils=await import(url('src/math/MathUtils.js'));

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);

const scenarios=[];
const add=(line,expected)=>scenarios.push({line,expected});
// The grid is shared by every conversion kind, so the non-finite entries below make the sRGB transfer functions face NaN and +/-Infinity as well: the pinned branches compare against the 0.04045 / 0.0031308 thresholds, and a NaN comparison is false, so NaN takes the power branch and stays NaN.
const channels=[0,-0,1e-8,-1e-8,0.0001,0.04045,0.0031308,0.5,1,2,-0.5,3.5,1e6,-1e6,1e300,NaN,Infinity,-Infinity];
// Color sRGB conversions: copySRGBToLinear / copyLinearToSRGB / convertSRGBToLinear /
// convertLinearToSRGB. The pinned r187 Color has no gamma-factor variants.
const kinds=[
    {id:0,name:'copySRGBToLinear',run:source=>{
        const target=new Color();target.copySRGBToLinear(source);return [target.r,target.g,target.b];}},
    {id:1,name:'copyLinearToSRGB',run:source=>{
        const target=new Color();target.copyLinearToSRGB(source);return [target.r,target.g,target.b];}},
    {id:2,name:'convertSRGBToLinear',run:source=>{
        const target=new Color().copy(source);target.convertSRGBToLinear();return [target.r,target.g,target.b];}},
    {id:3,name:'convertLinearToSRGB',run:source=>{
        const target=new Color().copy(source);target.convertLinearToSRGB();return [target.r,target.g,target.b];}}
];
for(const kind of kinds){
    for(let index=0;index<channels.length;++index){
        const r=channels[index];
        const g=channels[(index+5)%channels.length];
        const b=channels[(index+11)%channels.length];
        const source=new Color(r,g,b);
        add(`G ${kind.id} ${token(r)} ${token(g)} ${token(b)}`,{value:encodeArray(kind.run(source))});
    }
}
// MathUtils.damp and the power-of-two helpers.
for(const [x,y,lambda,dt] of [[0,1,1,1],[0,1,0.5,0.016],[1,0,10,0.5],[-1,1,2.5,0.25],[5,5,3,1],
    [0,0,0,0],[1,2,100,0.001],[0,1,1e-8,1e8],[1e6,-1e6,2,3],[NaN,1,1,1],[0,Infinity,1,1]]){
    add(`D ${token(x)} ${token(y)} ${token(lambda)} ${token(dt)}`,
        {value:encode(MathUtils.damp(x,y,lambda,dt))});
}
const powers=[0,-0,1,2,3,4,5,8,1024,0.5,0.25,1.5,3.7,1e6,1e-8,-1,-2,-0.5,NaN,Infinity,-Infinity,1e300,5e-324,1023,1025];
for(const value of powers){
    add(`P 0 ${token(value)}`,{value:MathUtils.isPowerOfTwo(value)});
    add(`P 1 ${token(value)}`,{value:encode(MathUtils.floorPowerOfTwo(value))});
    add(`P 2 ${token(value)}`,{value:encode(MathUtils.ceilPowerOfTwo(value))});
}

const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:256*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,140)}`);
}
const conversionCount=scenarios.filter(scenario=>scenario.line.startsWith('G ')).length;
console.log(`Color/MathUtils parity: ${scenarios.length} scenarios (${conversionCount} sRGB conversions,`
    +` damp and the power-of-two helpers) match the pinned checkout bit for bit; ${process.version}`);
