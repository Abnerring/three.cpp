// Differential check for the native JSNumberMath trigonometric entry points:
// Math.tan, Math.asin, Math.atan and Math.atan2, all extracted from the pinned Node/V8
// fdlibm source (Node v26.5.0, deps/v8/src/base/ieee754.cc).
//
// The oracle is the global Math object of the pinned Node release, so no three.js module is
// imported; the source directory argument is only checked for the pinned checkout, which
// keeps this script's command line identical to the other parity probes. Every probe line is
// compared with assert.deepEqual, so no tolerance is used anywhere: one differing bit fails.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
if(process.argv.length!==4)throw new Error('Usage: node compare-math-trig.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
assert.ok(fs.existsSync(path.join(sourceDir,'package.json')),'the pinned three.js checkout must exist');

const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
// Exact neighbours of a finite double, so "just above/below" a branch point is the real next
// representable value instead of a perturbation that rounds back onto the boundary.
const nextUp=value=>{
    if(Number.isNaN(value)||value===Infinity)return value;
    if(Object.is(value,-0))return Number.MIN_VALUE;
    const buffer=new DataView(new ArrayBuffer(8));
    buffer.setFloat64(0,value);
    buffer.setBigUint64(0,buffer.getBigUint64(0)+(value>0?1n:-1n));
    return buffer.getFloat64(0);
};
const nextDown=value=>-nextUp(-value);

const scenarios=[];
const add=(line,expected)=>scenarios.push({line,expected});

// ---------------------------------------------------------------- shared LCG
// The probe implements the same 32-bit state machine over uint32, and Math.imul reproduces
// the signed 32-bit product exactly, so both sides walk one sequence. The `lcg` scenario
// proves that before the sequence continues into the pseudo-random grid below.
const LCG_SEED=0x2545F491;
let lcgState=LCG_SEED;
const lcgNext=()=>{lcgState=(Math.imul(lcgState,1664525)+1013904223)>>>0;return lcgState;};
const lcgDouble=()=>{
    const bits=lcgNext();
    const sign=(bits&1)===1?-1:1;
    const exponent=(bits>>>1)%40-20;
    // 1 + mantissa / 2^32 and 2^exponent are exact; the value needs no rounding.
    return sign*(1+(bits>>>7)/4294967296)*2**exponent;
};
const lcgValues=[];
for(let i=0;i<256;++i)lcgValues.push(lcgDouble());
add('lcg 256',{values:lcgValues.map(encode)});

// ---------------------------------------------------------------- boundary grid
// Every function is probed at 0, both zeros, the unit values, both subnormal scales, the
// huge and the tiny end of the range, and the non-finite inputs.
const boundaryValues=[0,-0,1,-1,2,-2,0.5,-0.5,1.5,-1.5,1e-8,-1e-8,1e8,-1e8,1e300,-1e300,
    5e-324,-5e-324,1e-320,-1e-320,1e308,-1e308,NaN,Infinity,-Infinity];
for(const value of boundaryValues){
    add(`tan ${token(value)}`,{value:encode(Math.tan(value))});
    add(`asin ${token(value)}`,{value:encode(Math.asin(value))});
    add(`atan ${token(value)}`,{value:encode(Math.atan(value))});
}

// tan: the reduction splits at pi/4 and turns into -1/tan at every odd multiple of pi/2, so
// each of those centres is probed with its exact neighbours, together with the arguments
// that force the medium and the large rem_pio2 paths.
const tanValues=[Math.PI/4,-Math.PI/4,Math.PI,-Math.PI,2*Math.PI,-2*Math.PI,
    3*Math.PI/4,-3*Math.PI/4,2**19*Math.PI/2,2**30,2**52,9007199254740992,1e15,1e19,1e100];
for(const multiple of [1,-1,3,-3,5,-5,7,-7,31,-31,33,-33]){
    const center=multiple*Math.PI/2;
    tanValues.push(center,nextUp(center),nextDown(center),nextUp(nextUp(center)),nextDown(nextDown(center)));
}
for(const value of tanValues)add(`tan ${token(value)}`,{value:encode(Math.tan(value))});

// asin: the domain edge |x| = 1, the 2^-27, 0.5, 0.975 and 0.98 branch points of the pinned
// implementation, and the out-of-domain inputs that must yield NaN.
const asinValues=[nextUp(1),nextDown(1),nextUp(-1),nextDown(-1),1+1e-16,-(1+1e-16),1-1e-16,
    -(1-1e-16),1+2**-52,-(1+2**-52),1+2**-51,-(1+2**-51),2,-2,0.975,nextUp(0.975),nextDown(0.975),
    0.98,nextUp(0.98),nextDown(0.98),2**-27,nextUp(2**-27),nextDown(2**-27),-(2**-27),2**-57,2**-60,
    0.7071067811865476,0.9999999999999999,1.0000000000000002];
for(const value of asinValues)add(`asin ${token(value)}`,{value:encode(Math.asin(value))});

// atan: the argument reduction branch points 2^-27, 7/16, 11/16, 19/16, 2.4375 and 2^66,
// each with its exact neighbours on both sides of zero.
const atanValues=[1e300,-1e300,1e308,-1e308,5e-324,-5e-324,1e-320,nextDown(2**66),nextUp(2**66),
    9007199254740992,0.6744,0.4375,7/16,11/16,19/16];
for(const value of [2**-27,0.4375,7/16,11/16,1.1875,19/16,2.4375,2**66]){
    atanValues.push(value,-value,nextUp(value),nextDown(value),nextUp(-value),nextDown(-value));
}
for(const value of atanValues)add(`atan ${token(value)}`,{value:encode(Math.atan(value))});

// atan2: every sign combination of (y, x) over the zeros, the infinities, NaN and a spread of
// magnitudes, as the ECMAScript special cases are defined per sign and per zero.
const atan2Operands=[0,-0,1,-1,2,-2,0.5,-0.5,1e-8,-1e-8,1e8,-1e8,1e300,-1e300,5e-324,-5e-324,
    1e-320,-1e-320,1e308,-1e308,NaN,Infinity,-Infinity];
for(const y of atan2Operands)
    for(const x of atan2Operands)
        add(`atan2 ${token(y)} ${token(x)}`,{value:encode(Math.atan2(y,x))});

// The |y/x| > 2^60 and the 0 > |y|/x > -2^-60 shortcuts, and x === 1 where the pinned
// implementation returns atan(y) directly.
const atan2RatioPairs=[[1e300,1e-300],[-1e300,1e-300],[1e300,-1e-300],[-1e300,-1e-300],
    [1e-300,1e300],[-1e-300,1e300],[1e-300,-1e300],[-1e-300,-1e300],[5e-324,1e308],[-5e-324,1e308],
    [5e-324,-1e308],[-5e-324,-1e308],[1e308,5e-324],[-1e308,5e-324],[1e308,-5e-324],[-1e308,-5e-324],
    [1e-320,1e-320],[-1e-320,1e-320],[1,1e300],[-1,1e300],[1e300,1],[-1e300,1],[1e-8,1],[-1e-8,1],
    [1e8,1],[-1e8,1],[NaN,1],[Infinity,1],[-Infinity,1]];
for(const [y,x] of atan2RatioPairs)add(`atan2 ${token(y)} ${token(x)}`,{value:encode(Math.atan2(y,x))});

// ---------------------------------------------------------------- pseudo-random grid
// Continues the sequence the `lcg` scenario just verified, so the random inputs are the same
// doubles on both sides without travelling as tokens of their own.
const randomValues=[];
for(let i=0;i<260;++i)randomValues.push(lcgDouble());
for(const value of randomValues){
    add(`tan ${token(value)}`,{value:encode(Math.tan(value))});
    add(`atan ${token(value)}`,{value:encode(Math.atan(value))});
    // asin needs |x| <= 1, so the raw value is folded into the domain as well.
    add(`asin ${token(value)}`,{value:encode(Math.asin(value))});
    add(`asin ${token(value/(1+Math.abs(value)))}`,{value:encode(Math.asin(value/(1+Math.abs(value))))});
    add(`asin ${token(value/1048576)}`,{value:encode(Math.asin(value/1048576))});
}
for(let index=0;index+1<randomValues.length;index+=2){
    const y=randomValues[index],x=randomValues[index+1];
    add(`atan2 ${token(y)} ${token(x)}`,{value:encode(Math.atan2(y,x))});
    add(`atan2 ${token(-y)} ${token(-x)}`,{value:encode(Math.atan2(-y,-x))});
    add(`atan2 ${token(y)} ${token(-x)}`,{value:encode(Math.atan2(y,-x))});
}

// ---------------------------------------------------------------- run the probe
const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:256*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,160)}`);
    compared+=1;
}
const counts={tan:0,asin:0,atan:0,atan2:0,lcg:0};
for(const {line} of scenarios){
    const kind=line.slice(0,line.indexOf(' ')===-1?line.length:line.indexOf(' '));
    counts[kind]+=1;
}
console.log(`JSNumberMath trig parity: ${compared} scenarios (${counts.tan} tan, ${counts.asin} asin, `
    +`${counts.atan} atan, ${counts.atan2} atan2, ${counts.lcg} LCG) match the ${process.version} `
    +'global Math bit for bit');
