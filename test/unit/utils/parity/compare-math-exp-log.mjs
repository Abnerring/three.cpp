// Differential check for the native JSNumberMath exponential/logarithm entry points:
// Math.exp, Math.log and Math.log2, all extracted from the pinned Node/V8 fdlibm source
// (Node v26.5.0, deps/v8/src/base/ieee754.cc).
//
// The oracle is the global Math object of the pinned Node release, so no three.js module is
// imported; the source directory argument is only checked for the pinned checkout, which
// keeps this script's command line identical to the other parity probes. Every probe line is
// compared with assert.deepEqual, so no tolerance is used anywhere: one differing bit fails.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
if(process.argv.length!==4)throw new Error('Usage: node compare-math-exp-log.mjs <threejs-source-dir> <probe.exe>');
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
// The exact double with the given raw bit pattern, so the branch constants the pinned
// implementation tests as high words (0x3FE6147A, 0x3FE6B851, the 0x6A09C normalization edge,
// the subnormal floor) are probed on both sides of the edge rather than at a rounded decimal.
const fromBits=pattern=>{
    const buffer=new DataView(new ArrayBuffer(8));
    buffer.setBigUint64(0,pattern);
    return buffer.getFloat64(0);
};
const fromHighWord=(highWord,lowWord=0)=>fromBits((BigInt(highWord)<<32n)|BigInt(lowWord));

const scenarios=[];
const add=(line,expected)=>scenarios.push({line,expected});

// ---------------------------------------------------------------- shared LCG
// The probe implements the same 32-bit state machine over uint32, and Math.imul reproduces
// the signed 32-bit product exactly, so both sides walk one sequence. The `lcg` and `bits`
// scenarios prove that before the sequence continues into the pseudo-random grids below.
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
// Two states become the high and the low word of a double, matching std::bit_cast on the
// probe side, so raw bit patterns (subnormals, infinities, NaN payloads) are covered too.
const lcgBits=()=>{
    const pattern=(BigInt(lcgNext())<<32n)|BigInt(lcgNext());
    return fromBits(pattern);
};
const lcgValues=[];
for(let i=0;i<256;++i)lcgValues.push(lcgDouble());
add('lcg 256',{values:lcgValues.map(encode)});
// The probe consumes its LCG state in scenario order, so the raw bit patterns of `bits` are
// drawn immediately after the `lcg` ones: those are the only two scenarios that advance the
// state, and every grid below sends its inputs as tokens.
const randomBitValues=[];
for(let i=0;i<96;++i)randomBitValues.push(lcgBits());
add('bits 96',{values:randomBitValues.map(encode)});
// Drawn after both verified batches, so these inputs only ever travel as tokens.
const randomValues=[];
for(let i=0;i<260;++i)randomValues.push(lcgDouble());

// ---------------------------------------------------------------- boundary grid
// Every function is probed at 0, both zeros, the unit values, one, the values around 1, e, the
// subnormal scales, the huge end of the range, and the non-finite inputs. log/log2 of the
// negative entries must be NaN, log/log2 of +-0 must be -Infinity and exp(-0) must be 1.
const boundaryValues=[0,-0,1,-1,2,10,Math.E,0.5,1.5,1e-8,-1e-8,5e-324,-5e-324,1e-320,-1e-320,
    1e300,-1e300,1e308,-1e308,NaN,Infinity,-Infinity,0.9999999999999999,1.0000000000000002];
for(const value of boundaryValues){
    add(`exp ${token(value)}`,{value:encode(Math.exp(value))});
    add(`log ${token(value)}`,{value:encode(Math.log(value))});
    add(`log2 ${token(value)}`,{value:encode(Math.log2(value))});
}

// exp: the overflow threshold 7.09782712893383973096e+02 and the underflow threshold
// -7.45133219101941108420e+02, the 0.5*ln2 and 1.5*ln2 argument-reduction edges, the 2**-28
// inexact shortcut, the exp(1) special case, and the two scaling branches at k = -1021 and
// k = 1024 (the latter only reachable at the overflow threshold itself). Each edge is probed
// with its exact neighbours.
const O_THRESHOLD=709.782712893383973096;
const U_THRESHOLD=-745.133219101941108420;
const expValues=[1,2,Math.E,Math.LN2,-Math.LN2,Math.LN2/2,1.5*Math.LN2,-(2**-28),2**-28,2**-29,
    2**-57,709,709.5,710,-709.5,-745,-746,-707.5,-708,-400,-400.5,1024*Math.LN2,1023*Math.LN2,
    -1021*Math.LN2,-1022*Math.LN2,-1023*Math.LN2];
for(const value of [O_THRESHOLD,U_THRESHOLD,Math.LN2/2,1.5*Math.LN2,2**-28,-(2**-28),
    1024*Math.LN2,-1021*Math.LN2,-1022*Math.LN2]){
    expValues.push(value,nextUp(value),nextDown(value),nextUp(nextUp(value)),nextDown(nextDown(value)));
}
for(const value of expValues)add(`exp ${token(value)}`,{value:encode(Math.exp(value))});

// log: the 2**-20 fast path around 1, the sqrt(2)/sqrt(1/2) normalization edge where
// (hx + 0x95F64) crosses 0x100000, the 0x6147A/0x6B851 polynomial-split edges, the 2**54
// subnormal scale, and the largest-subnormal / smallest-normal boundary.
const logValues=[nextUp(1),nextDown(1),1-2**-20,1+2**-20,nextDown(1-2**-20),nextUp(1+2**-20),
    Math.SQRT1_2,Math.SQRT2,2**-1022,5e-324,2**1023,Number.MAX_VALUE,nextDown(Number.MAX_VALUE),
    nextUp(Number.MAX_VALUE),2**54,2**-54,fromHighWord(0x000FFFFF,0xFFFFFFFF),fromHighWord(0x00100000)];
for(const value of [1-2**-20,1+2**-20,fromHighWord(0x3FE6A09C),fromHighWord(0x3FF6A09C),
    fromHighWord(0x3FE6147A),fromHighWord(0x3FE6B851),fromHighWord(0x3FF6147A),
    fromHighWord(0x3FF6B851),fromHighWord(0x000FFFFF,0xFFFFFFFF),fromHighWord(0x00100000)]){
    logValues.push(value,nextUp(value),nextDown(value));
}
for(const value of logValues)add(`log ${token(value)}`,{value:encode(Math.log(value))});

// log2: the exact log2(1) = +0 shortcut, the sqrt(2)/sqrt(1/2) edges where the upstream
// ivln2hi/ivln2lo split has to absorb the cancellation, the subnormal 2**54 scaling and every
// power-of-two exponent.
const log2Values=[1,nextUp(1),nextDown(1),0,-0,1-2**-20,1+2**-20,Math.SQRT2,Math.SQRT1_2,2,4,
    1024,2**52,2**-52,2**-1074,2**1023,Number.MAX_VALUE,1e300,fromHighWord(0x000FFFFF,0xFFFFFFFF),
    fromHighWord(0x00100000)];
for(const value of [1,Math.SQRT2,Math.SQRT1_2,fromHighWord(0x3FE6A09C),fromHighWord(0x3FF6A09C),
    fromHighWord(0x000FFFFF,0xFFFFFFFF),fromHighWord(0x00100000)]){
    log2Values.push(nextUp(value),nextDown(value),nextUp(nextUp(value)),nextDown(nextDown(value)));
}
for(const value of log2Values)add(`log2 ${token(value)}`,{value:encode(Math.log2(value))});

// ---------------------------------------------------------------- pseudo-random grid
// Continues the sequence the `lcg` and `bits` scenarios just verified, so the random inputs are
// the same doubles on both sides without travelling as tokens of their own.
for(const value of randomValues){
    add(`exp ${token(value)}`,{value:encode(Math.exp(value))});
    add(`exp ${token(value/16)}`,{value:encode(Math.exp(value/16))});
    add(`exp ${token(value/1024)}`,{value:encode(Math.exp(value/1024))});
    add(`log ${token(value)}`,{value:encode(Math.log(value))});
    // log/log2 need a positive argument, so the raw value is folded into the domain as well.
    add(`log ${token(Math.abs(value))}`,{value:encode(Math.log(Math.abs(value)))});
    add(`log ${token(Math.abs(value)+1)}`,{value:encode(Math.log(Math.abs(value)+1))});
    add(`log2 ${token(value)}`,{value:encode(Math.log2(value))});
    add(`log2 ${token(Math.abs(value))}`,{value:encode(Math.log2(Math.abs(value)))});
    add(`log2 ${token(Math.abs(value)+1)}`,{value:encode(Math.log2(Math.abs(value)+1))});
}
for(const value of randomBitValues){
    add(`exp ${token(value)}`,{value:encode(Math.exp(value))});
    add(`log ${token(value)}`,{value:encode(Math.log(value))});
    add(`log2 ${token(value)}`,{value:encode(Math.log2(value))});
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
const counts={exp:0,log:0,log2:0,lcg:0,bits:0};
for(const {line} of scenarios){
    const kind=line.slice(0,line.indexOf(' ')===-1?line.length:line.indexOf(' '));
    counts[kind]+=1;
}
console.log(`JSNumberMath exp/log parity: ${compared} scenarios (${counts.exp} exp, ${counts.log} log, `
    +`${counts.log2} log2, ${counts.lcg} LCG, ${counts.bits} bit patterns) match the ${process.version} `
    +'global Math bit for bit');
