// Differential check for the native src/math/MathUtils.js port.
//
// One call per scenario, driven through the pinned module itself, so the helpers are compared on the
// inputs that usually break a hand port: a reversed clamp range, negative euclidean modulo, a zero
// divisor, a zero-length map range and the power-of-two edges.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-math-utils.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {MathUtils}=await import(pathToFileURL(path.join(sourceDir,'src/math/MathUtils.js')).href);
const scenarios=[
    ['clamp',5,-1,1],['clamp',-5,-1,1],['clamp',1,3,0],['clamp',0.5,0,1],
    ['euclideanModulo',5,3],['euclideanModulo',-5,3],['euclideanModulo',5,-3],['euclideanModulo',5,0],
    ['mapLinear',5,0,10,0,100],['mapLinear',-2,0,10,0,100],['mapLinear',5,0,0,0,1],
    ['inverseLerp',0,10,5],['inverseLerp',0,0,5],['inverseLerp',0,10,-5],
    ['lerp',0,10,0.25],['lerp',-10,10,0.5],
    ['damp',0,10,1,0.5],['damp',10,0,2,0.25],
    ['pingpong',5,3],['pingpong',-1,3],['pingpong',7,0],
    ['smoothstep',0.5,0,1],['smoothstep',-1,0,1],['smoothstep',2,0,1],
    ['smootherstep',0.5,0,1],['smootherstep',-0.5,0,1],
    ['degToRad',180],['degToRad',-90],['radToDeg',Math.PI],
    ['isPowerOfTwo',0],['isPowerOfTwo',1],['isPowerOfTwo',2],['isPowerOfTwo',3],['isPowerOfTwo',1024],
    ['ceilPowerOfTwo',5],['ceilPowerOfTwo',0],['ceilPowerOfTwo',4],
    ['floorPowerOfTwo',5],['floorPowerOfTwo',0],['floorPowerOfTwo',4]
];
const encode=value=>{
    if(typeof value!=='number')return value;
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(([name,...args])=>`${name} ${args.join(' ')}`);
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
for(let index=0;index<scenarios.length;++index){
    const [name,...args]=scenarios[index];
    const expected={result:encode(MathUtils[name](...args))};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',name,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${name}(${args.join(', ')})`);
}
console.log(`MathUtils parity: ${scenarios.length} scenarios`);