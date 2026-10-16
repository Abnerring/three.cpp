// Differential check for the native examples/jsm/math/MeshSurfaceSampler.js port.
//
// The pinned class is imported from the checkout, the same scenarios are replayed there with an
// identically injected random source, and the float32 distribution plus every sampled
// position/normal/colour/uv is compared with assert.deepEqual (no tolerances).
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-mesh-surface-sampler.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const {MeshSurfaceSampler}=await import(url('examples/jsm/math/MeshSurfaceSampler.js'));
const THREE=await import(url('build/three.module.js'));
const {BoxGeometry,SphereGeometry,Mesh,BufferGeometry,Vector2,Vector3,Color,Float32BufferAttribute}=THREE;

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);

// Mirrors addColor() of the probe: integer maths only, so both sides build identical data.
function addColor(geometry){
    const count=geometry.getAttribute('position').count;
    const values=[];
    for(let i=0;i<count;++i){
        values.push((i%7)/7,((i*3)%5)/5,((i*5)%11)/11);
    }
    geometry.setAttribute('color',new Float32BufferAttribute(values,3));
}
function buildMesh(kind){
    if(kind===5) return new Mesh(); // empty geometry: no position attribute
    let geometry;
    if(kind===0||kind===2||kind===6){
        geometry=new BoxGeometry(1,1,1);
        if(kind===2) addColor(geometry);
        if(kind===6) addZeroColor(geometry);
    }else if(kind===1||kind===3||kind===7){
        geometry=new BoxGeometry(1,1,1).toNonIndexed();
        if(kind===3) addColor(geometry);
        if(kind===7) addZeroColor(geometry);
    }else if(kind===4){
        geometry=new SphereGeometry(1,8,6);
    }else{
        throw new Error('unknown mesh kind');
    }
    const mesh=new Mesh(geometry);
    mesh.updateMatrixWorld(true);
    return mesh;
}
// Kinds 6 and 7 carry an all-zero colour attribute so a zero-total weight distribution runs.
function addZeroColor(geometry){
    const count=geometry.getAttribute('position').count;
    geometry.setAttribute('color',new Float32BufferAttribute(new Array(count*3).fill(0),3));
}

// A fixed pseudo-random pull (LCG) reused for every scenario, plus a few edge values.
const draws=[];
let seed=123456789;
for(let i=0;i<64;++i){
    seed=(seed*1103515245+12345)%2147483648;
    draws.push(seed/2147483648);
}
const edgeDraws=[0,0.5,1,-0,-1,1e-300,0.9999999999999999,...draws];

const meshKinds=[0,1,2,3,4];
const scenarios=[];
for(const meshKind of meshKinds){
    for(const weightAttribute of ['-','color']){
        for(const sampleCount of [0,1,3,17]){
            const values=meshKind%2===0?edgeDraws:draws;
            scenarios.push({meshKind,weightAttribute,sampleCount,values});
        }
    }
}
scenarios.push({meshKind:5,weightAttribute:'-',sampleCount:2,values:draws});
// Boundary and degenerate draws: an exact 0 and an exact 1 draw (the pinned binary search then
// walks off the end of the distribution), a draw sequence of a single value, and draws above 1
// as well as a negative draw. These pin the search's edge behaviour instead of the happy path.
for(const meshKind of [0,4]){
    for(const values of [[0],[1],[0.5],[-1],[2],[1e-320],[0.9999999999999999,0,-1,2]]){
        scenarios.push({meshKind,weightAttribute:'-',sampleCount:3,values});
        scenarios.push({meshKind,weightAttribute:'color',sampleCount:3,values});
    }
}
// A weight attribute whose total is zero: every cumulative entry is 0, so the search compares
// against 0 for any draw and the face index comes from the boundary branch.
for(const meshKind of [6,7]){
    scenarios.push({meshKind,weightAttribute:'color',sampleCount:4,values:[0,0.5,1,-1]});
}

const lines=scenarios.map(({meshKind,weightAttribute,sampleCount,values})=>
    `M ${meshKind} ${weightAttribute} ${sampleCount} ${values.map(token).join(' ')}`);
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:256*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');

const expectedFor=({meshKind,weightAttribute,sampleCount,values})=>{
    if(meshKind===5){
        // The pinned class reads the length of an empty distribution and then throws.
        return {error:'TypeError'};
    }
    const mesh=buildMesh(meshKind);
    const sampler=new MeshSurfaceSampler(mesh);
    sampler.setWeightAttribute(weightAttribute==='-'?null:weightAttribute);
    sampler.build();
    let cursor=0;
    sampler.setRandomGenerator(()=>values[cursor++%values.length]);
    const samples=[];
    for(let i=0;i<sampleCount;++i){
        const position=new Vector3(), normal=new Vector3(), color=new Color(), uv=new Vector2();
        sampler.sample(position,normal,color,uv);
        samples.push({position:encodeArray([position.x,position.y,position.z]),
            normal:encodeArray([normal.x,normal.y,normal.z]),
            color:encodeArray([color.r,color.g,color.b]),uv:encodeArray([uv.x,uv.y])});
    }
    return {distribution:encodeArray(sampler.distribution),samples};
};

let samples=0,faces=0;
for(let index=0;index<scenarios.length;++index){
    const scenario=scenarios[index];
    const actual=JSON.parse(output[index]);
    const expected=expectedFor(scenario);
    if(expected.error){
        assert.equal(actual.error,expected.error,`scenario ${index}: expected the pinned error`);
        assert.match(actual.message,/undefined/,`scenario ${index}: error message`);
        continue;
    }
    assert.deepEqual(actual,expected,`scenario ${index}: mesh ${scenario.meshKind} weight ${scenario.weightAttribute}`);
    samples+=expected.samples.length;
    faces+=expected.distribution.length;
}
console.log(`MeshSurfaceSampler parity: ${scenarios.length} scenarios, ${faces} float32 distribution`
    +` entries and ${samples} samples (position/normal/colour/uv) match the pinned checkout bit for bit;`
    +` ${process.version}`);
