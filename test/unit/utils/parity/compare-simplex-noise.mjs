// Differential check for the native examples/jsm/math/SimplexNoise.js port.
//
// The upstream module is imported from the pinned checkout and every scenario is replayed
// there with an instance built from the same deterministic sequence the probe injects, so the
// whole permutation table (p, perm) and the three noise methods are compared with
// assert.deepEqual and no tolerance anywhere.
//
// The upstream constructor is `constructor( r = Math )`. `Math.random()` is not reproducible
// across engines, so both sides build their instances from the generators below: an LCG and a
// Mulberry32 that are implemented identically in C++ and JavaScript, plus the constant sources.
// The pinned `MathUtils.seededRandom()` is itself Mulberry32, seeded with the module's initial
// `_seed = 1234567`, so source 0 is the exact table the upstream module would build from
// `new SimplexNoise( { random: MathUtils } )`.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-simplex-noise.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const moduleURL=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const {SimplexNoise}=await import(moduleURL('examples/jsm/math/SimplexNoise.js'));

// 32-bit LCG, mirrored by `struct Lcg` in SimplexNoiseParityProbe.cpp. Math.imul keeps the
// multiply inside int32 exactly like the uint32_t arithmetic on the C++ side, and the division
// by 2^32 is exact, so both sides hand the constructor the same binary64 values.
function lcg(seed){
    let state=seed>>>0;
    return ()=>{state=(Math.imul(state,1664525)+1013904223)>>>0;return state/4294967296;};
}
// The same Mulberry32 as src/math/MathUtils.js `seededRandom`. three.js pre-seeds its module
// state with 1234567 (`let _seed = 1234567`), so `new SimplexNoise()` of the pinned module —
// whose default argument is `Math` and therefore `Math.random()` — is not the reproducible
// instance; `new SimplexNoise( { random: MathUtils } )` is, and both sides build that one here
// from the same seed. The native port's own default source is `MathUtils::seededRandom()` of
// this project (shared generator state, no seed argument on the constructor), so its exact
// table is covered by the probe invariants and the unit test instead of an upstream table.
const mulberry32=seed=>{
    let state=seed>>>0;
    return ()=>{let t=(state=(state+0x6D2B79F5)>>>0);t=Math.imul(t^(t>>>15),t|1);
        t^=t+Math.imul(t^(t>>>7),t|61);return((t^(t>>>14))>>>0)/4294967296;};
};
// Each entry is a factory, so the upstream instance and the probe instance never share a
// generator (and therefore never advance each other's sequence).
const sources=[
    {kind:'Mulberry32( 1234567 ) = MathUtils.seededRandom()',make:()=>mulberry32(1234567)},
    {kind:'constant 0.5',make:()=>()=>0.5},
    {kind:'constant 0',make:()=>()=>0},
    {kind:'LCG 0x9e3779b9',make:()=>lcg(0x9e3779b9)},
    {kind:'constant 1 - 1e-16',make:()=>()=>0.9999999999999999}];
const instances=sources.map(source=>new SimplexNoise({random:source.make()}));

const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const tokens=values=>values.map(token).join(' ');
// The native port keeps p and perm as std::vector< double > and the three gradient tables as
// std::array rows, all of which JSON renders as plain nested arrays.
const instanceExpected=source=>({
    source:encode(source),
    grad3:encodeArray(instances[source].grad3.map(row=>row.map(encode))),
    grad4:encodeArray(instances[source].grad4.map(row=>row.map(encode))),
    p:encodeArray(instances[source].p),
    perm:encodeArray(instances[source].perm),
    simplex:encodeArray(instances[source].simplex.map(row=>row.map(encode)))});
const noiseExpected=(source,x,y,z,w)=>({
    source:encode(source),
    noise:encode(instances[source].noise(x,y)),
    noise3d:encode(instances[source].noise3d(x,y,z)),
    noise4d:encode(instances[source].noise4d(x,y,z,w))});

const scenarios=[];
const add=(line,expected)=>scenarios.push({line,expected});
const addPoint=(source,x,y,z,w)=>add(`simplex-noise ${source} ${tokens([x,y,z,w])}`,
    noiseExpected(source,x,y,z,w));

// ---------------------------------------------------------------- instances
for(const source of instances.keys())add(`simplex-instance ${source}`,instanceExpected(source));
// The permutation invariants of the source: p is `Math.floor( random() * 256 )` and perm is the
// doubled table. Every instance the two sides build has to satisfy them, and for source 0 the
// same checks also guard the native `new SimplexNoise()` default source in SimplexNoise.tests.cpp.
for(const source of instances.keys()){
    const noise=instances[source];
    assert.equal(noise.p.length,256,`source ${source}: p length`);
    assert.equal(noise.perm.length,512,`source ${source}: perm length`);
    assert.equal(noise.grad3.length,12,`source ${source}: grad3 length`);
    assert.equal(noise.grad4.length,32,`source ${source}: grad4 length`);
    assert.equal(noise.simplex.length,64,`source ${source}: simplex length`);
    for(let i=0;i<256;++i){
        assert.equal(noise.p[i],Math.floor(noise.p[i]),`source ${source}: p[${i}] integrality`);
        assert.ok(noise.p[i]>=0&&noise.p[i]<=255,`source ${source}: p[${i}] range`);
        if(noise.perm[i]!==noise.p[i&255])throw new Error(`source ${source}: perm[${i}]`);
    }
    for(let i=256;i<512;++i)assert.equal(noise.perm[i],noise.p[i&255],`source ${source}: perm[${i}]`);
}

// ---------------------------------------------------------------- 2D branches
// `x0 > y0` / `x0 <= y0`: the exact diagonal, zero, and both sides of it.
const diagonal2d=[[1,0],[1,1],[0,1],[0,0],[1.0000000000000002,1],[0.9999999999999999,1],
    [-1,-0],[-0,-0],[2.5,2.5],[2.5,2.5000000000000004],[1e6,1e6],[1e6,999999.9999999999],
    [0.5,0.5],[-0.5,-0.5],[3,-4],[-4,3],[255.5,255.5],[256.5,256.5]];
for(const source of [0,1])for(const [x,y] of diagonal2d)addPoint(source,x,y,0,0);

// ---------------------------------------------------------------- 4D branch coverage
// All six pairwise comparisons of x0, y0, z0, w0 decide `c`, so the grid moves every coordinate
// through integers, halves and thirds while the LCG block below covers the general case.
const grid2=[-2,-1,-0.5,0,0.25,1,1.5,2,3.5,255,256.5];
for(const x of grid2)for(const y of grid2)addPoint(0,x,y,0.5,-0.5);
const grid3=[-1.5,-0.5,0,0.25,1,2,3.5,255];
for(const x of grid3)for(const y of grid3)for(const z of grid3)addPoint(0,x,y,z,0.5);
const grid4=[-1.5,0,1,255];
for(const x of grid4)for(const y of grid4)for(const z of grid4)for(const w of grid4)
    addPoint(0,x,y,z,w);
// Integer lattice points and their neighbourhoods, where every branch of noise3d is reached.
const lattice=[-2,-1,0,1,2,255,256];
for(const x of lattice)for(const y of lattice)for(const z of [-1,0,1,256])addPoint(0,x,y,z,0.75);
// Signed zero, non-finite and out of range coordinates plus the ToInt32 boundaries.
const edgePoints=[
    [-0,-0,-0,-0],[0,-0,0,-0],[-0,0,-0,0],[0.5,-0,-0.5,-0],[NaN,0,0,0],[0,NaN,0,0],[0,0,NaN,0],
    [0,0,0,NaN],[Infinity,0,0,0],[0,-Infinity,0.5,0],[-Infinity,-Infinity,-Infinity,-Infinity],
    [1e300,0.5,0.5,0.5],[-1e300,0.5,0.5,0.5],[4294967296.5,0.5,0.5,0.5],
    [-4294967296.5,0.5,0.5,0.5],[2147483647.25,-2147483648.75,1e21,-1e21],
    [1e-300,-1e-300,5e-324,-5e-324],[0.9999999999999999,1.0000000000000002,-1.0000000000000002,0.5],
    [8.98846567431158e307,-8.98846567431158e307,1.7976931348623157e308,0.5]];
for(const point of edgePoints)addPoint(0,...point);
for(const point of edgePoints)addPoint(1,...point);
// A wide pseudo-random block through the same generator the instances use for their tables.
// Every source is covered here, so the three noise methods run over large, fractional,
// negative and zero coordinates for all of them.
const sweep=lcg(0x1234abcd);
for(let i=0;i<400;++i){
    const x=(sweep()*512-256)*Math.pow(2,(i%17)-8);
    const y=(sweep()*512-256)*Math.pow(2,(i%13)-6);
    const z=(sweep()*64-32);
    const w=(sweep()*8-4);
    addPoint(i%sources.length,x,y,z,w);
}
for(let i=0;i<300;++i){
    const x=Math.floor(sweep()*1024-512),y=Math.floor(sweep()*1024-512);
    const z=Math.floor(sweep()*64-32),w=Math.floor(sweep()*16-8);
    addPoint(i%sources.length,x,y,z,w);
}
// The remaining sources are exercised over the same condensed grid.
for(const source of [1,2,3,4])
    for(const x of [-3,-0.5,0,0.5,3,256.25])
        for(const y of [-2.5,0,2.5,999.5])addPoint(source,x,y,1.5,-1.5);

// ---------------------------------------------------------------- run the probe
const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    // The probe is a native binary, so its text mode may append a carriage return to every
    // line; that is line-ending encoding, not probe output, and JSON.parse would otherwise keep
    // it inside the "NaN" / "-0" strings. The trailing newline of the last line is dropped.
    .split('\n').map(line=>line.replace(/\r$/,''));
if(output[output.length-1]==='')output.pop();
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,160)}`);
    compared+=1;
}
const kinds={instance:0,noise2d:0,noise3d:0,noise4d:0};
const counts=sources.map(()=>0);
for(const {line} of scenarios){
    const kind=line.slice(0,line.indexOf(' ')===-1?line.length:line.indexOf(' '));
    if(kind==='simplex-instance')kinds.instance+=1;
    else{
        kinds.noise2d+=1;
        kinds.noise3d+=1;
        kinds.noise4d+=1;
        counts[Number(line.split(' ')[1])]+=1;
    }
}
console.log(`SimplexNoise parity: ${compared} scenarios (${kinds.instance} instance tables, `
    +`${kinds.noise2d} noise, ${kinds.noise3d} noise3d, ${kinds.noise4d} noise4d) match the pinned `
    +`checkout bit for bit; sources ${counts.join('/')}; ${process.version}`);
