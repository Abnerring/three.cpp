// Differential check for the native examples/jsm/geometries/TeapotGeometry.js port.
//
// The pinned module is imported from the checkout, the same scenarios are replayed there, and
// every probe line is compared with assert.deepEqual, so no tolerance is used anywhere: the
// stored float32 position/normal/uv components, the index array, the computeBoundingSphere()
// result and the JSON parameter fields all have to match entry for entry. NaN and the signed
// zeroes travel as the shared string encoding of the other probes.
//
// The 32 combinations of bottom/lid/body/fitLid/blinn are replayed at two subdivisions, which
// covers the early return of the all-false case (the pinned loop simply skips every surface and
// still installs empty attributes plus an empty index) as well as the two independent scalings
// (the fitLid XY factor applied to the lid patches 20-27 and the Blinn Z factor applied when
// blinn is false), because the option matrix flips one flag at a time.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-teapot-geometry.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {TeapotGeometry}=await import(pathToFileURL(path.join(sourceDir,'examples/jsm/geometries/TeapotGeometry.js')).href);

const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);

// Both sides report the same attribute shape: the pinned constructor stores Float32Array values,
// so the components are compared as the binary64 conversions of the stored float32 entries.
const attributeJSON=attribute=>attribute===null||attribute===undefined?null:{
    count:attribute.count,itemSize:attribute.itemSize,normalized:attribute.normalized,
    values:encodeArray(attribute.array)};
// The pinned class never assigns `parameters`, so a BufferGeometry#toJSON result carries none of
// the fields a parameterized geometry would add. Comparing the surviving fields keeps a port that
// invents `size`/`segments`/... or a `type` of its own visible as a diff.
const jsonField=geometry=>{
    const data=geometry.toJSON();
    const fields={};
    for(const name of ['size','segments','bottom','lid','body','fitLid','blinn'])
        if(data[name]!==undefined) fields[name]=encode(data[name]);
    fields.type=data.type;
    return fields;
};
const geometryJSON=geometry=>({
    position:attributeJSON(geometry.attributes.position),
    normal:attributeJSON(geometry.attributes.normal),
    uv:attributeJSON(geometry.attributes.uv),
    hasIndex:geometry.index!==null,
    index:geometry.index===null?null:encodeArray(geometry.index.array),
    type:geometry.type,
    json:jsonField(geometry),
    boundingSphere:geometry.boundingSphere===null||geometry.boundingSphere===undefined?null:{
        center:encodeArray([geometry.boundingSphere.center.x,geometry.boundingSphere.center.y,
            geometry.boundingSphere.center.z]),
        radius:encode(geometry.boundingSphere.radius)}});

const scenarios=[];
// A scenario that throws on the pinned side is expected to report the same error message from the
// probe; `label` names the call in failure messages.
const add=(line,compute,label='')=>{
    let expected;
    try{
        expected=compute();
    }catch(error){
        expected={probeError:error.message};
    }
    scenarios.push({line,expected,label});
};
const addTeapot=(size,segments,flags,label)=>{
    const options=flags.map(Boolean);
    add(`teapot ${token(size)} ${token(segments)} ${options.map(flag=>flag?1:0).join(' ')}`,
        ()=>geometryJSON(new TeapotGeometry(size,segments,...options)),label);
};

// The constructor defaults of the native header have to be the upstream parameter list.
add('defaults',()=>geometryJSON(new TeapotGeometry()),'constructor defaults');

// Every combination of bottom/lid/body/fitLid/blinn at a low and a higher subdivision.
for(let mask=0;mask<32;++mask){
    const flags=[0,1,2,3,4].map(bit=>(mask>>bit)&1);
    const label=`flags ${flags.join('')}`;
    addTeapot(50,3,flags,`${label}, segments 3`);
    addTeapot(1,2,flags,`${label}, size 1`);
}

// The default subdivision, fractional values, the Math.floor clamp and the NaN case.
for(const segments of [10,1,2,2.9,3,4,6,6.5,0,-5,Number.NaN])
    addTeapot(50,segments,[1,1,1,1,1],`segments ${token(segments)}`);

// Several sizes, including a zero size whose vertices all coincide (so every triangle is
// degenerate and the index stays empty) and negative or very small/large scales.
for(const size of [0.5,1,3.7,50,1e6,1e-8,0,-50])
    addTeapot(size,4,[1,1,1,1,1],`size ${token(size)}`);

// ---------------------------------------------------------------- run the probe
const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0,components=0,indices=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected,label}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,
        `${label===''?'teapot':label}, scenario ${index}: ${line.slice(0,120)}`);
    for(const name of ['position','normal','uv'])
        if(expected[name]!==null&&expected[name]!==undefined) components+=expected[name].values.length;
    if(expected.index!==null&&expected.index!==undefined) indices+=expected.index.length;
    compared+=1;
}
const emptyIndex=scenarios.filter(scenario=>(scenario.expected.index??[]).length===0).length;
console.log(`Teapot geometry parity: ${compared} scenarios (${components} attribute components, `
    +`${indices} index entries, ${emptyIndex} with an empty index) match the pinned checkout bit for bit; `
    +`${process.version}`);
