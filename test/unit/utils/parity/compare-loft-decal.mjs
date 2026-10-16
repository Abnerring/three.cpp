// Differential check for the native LoftGeometry and DecalGeometry ports.
//
// The pinned modules are imported from the checkout, the same scenario list is replayed
// there, and attributes, index, type and the JSON parameter fields are compared with
// assert.deepEqual (no tolerances).
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-loft-decal.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const THREE=await import(url('build/three.module.js'));
const {LoftGeometry}=await import(url('examples/jsm/geometries/LoftGeometry.js'));
const {DecalGeometry}=await import(url('examples/jsm/geometries/DecalGeometry.js'));
const {Vector3,Euler,Mesh,BoxGeometry,BufferGeometry,Float32BufferAttribute}=THREE;

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const attributeJSON=attribute=>attribute===null||attribute===undefined?null:{
    count:attribute.count,itemSize:attribute.itemSize,values:encodeArray(attribute.array)};
const geometryJSON=(geometry,names)=>{
    const result={};
    for(const name of names) result[name]=attributeJSON(geometry.getAttribute(name));
    result.hasIndex=geometry.index!==null;
    result.index=geometry.index===null?null:encodeArray(geometry.index.array);
    result.type=geometry.type;
    return result;
};
const loftJSON=geometry=>{
    const result=geometryJSON(geometry,['position','uv','normal']);
    const json=geometry.toJSON();
    result.json={closed:json.closed,capStart:json.capStart,capEnd:json.capEnd,
        sectionRows:json.sections.length,type:json.type};
    return result;
};
const triangleSoup=()=>{
    const geometry=new BufferGeometry();
    geometry.setAttribute('position',new Float32BufferAttribute(new Float32Array([
        -0.75,-0.75,0.25, 0.75,-0.75,0.25, 0.75,0.75,0.25,
        -0.75,-0.75,0.25, 0.75,0.75,0.25, -0.75,0.75,0.25]),3));
    const mesh=new Mesh(geometry);
    mesh.updateMatrixWorld(true);
    return mesh;
};

const scenarios=[];
// LoftGeometry: a square tube, a triangle tube, tapering, degenerate and cap variants.
function ring(z,radius,count,offset=0){
    return Array.from({length:count},(_,i)=>{
        const angle=2*Math.PI*i/count+offset;
        return new Vector3(Math.cos(angle)*radius,Math.sin(angle)*radius,z);
    });
}
const loftCases=[
    {sections:[ring(0,1,4),ring(1,1,4)],options:{}},
    {sections:[ring(0,1,4),ring(1,0.5,4),ring(2,2,4)],options:{closed:true}},
    {sections:[ring(0,1,4),ring(1,1,4)],options:{closed:false}},
    {sections:[ring(0,1,4),ring(1,1,4)],options:{capStart:true}},
    {sections:[ring(0,1,4),ring(1,1,4)],options:{capEnd:true}},
    {sections:[ring(0,1,4),ring(1,1,4)],options:{capStart:true,capEnd:true,closed:false}},
    {sections:[ring(0,1,3),ring(1,0.25,3),ring(2,2,3)],options:{capStart:true,capEnd:true}},
    {sections:[ring(0,1,5,0.3),ring(1,1.5,5,0.3)],options:{capEnd:true}},
    {sections:[ring(0,0,4),ring(1,1,4)],options:{capStart:true}},
    {sections:[ring(0,1,4),ring(1,1,4)],options:{closed:false,capStart:true,capEnd:true}},
    {sections:[ring(0,1,4)],options:{}},                       // single section: console.error path
    {sections:[ring(0,1,4),ring(1,1,3)],options:{}},           // mismatched columns: console.error path
    {sections:[],options:{}},
    // Degenerate section shapes: a single point per section (closed sections then repeat it, so
    // every edge has zero length and the arc-length uvs divide by the total), a two-point section
    // with caps (the contour triangulation gets a line), and sections whose consecutive points
    // coincide so the per-point distances are zero.
    {sections:[[new Vector3(0,0,0)],[new Vector3(0,0,1)]],options:{}},
    {sections:[[new Vector3(0,0,0)],[new Vector3(0,0,1)]],options:{capStart:true,capEnd:true}},
    {sections:[[new Vector3(0,0,0)],[new Vector3(0,0,1)]],options:{closed:false}},
    {sections:[[new Vector3(1,0,0),new Vector3(0,1,0)],[new Vector3(1,0,1),new Vector3(0,1,1)]],
        options:{capStart:true,capEnd:true}},
    {sections:[[new Vector3(0,0,0),new Vector3(0,0,0)],[new Vector3(0,0,1),new Vector3(0,0,1)]],options:{}},
    {sections:[ring(0,1,4),ring(0,1,4),ring(1,1,4)],options:{capStart:true}},
    // Non-finite coordinates: the pinned class does not validate them, so NaN and Infinity flow
    // into the arc-length uvs, the cap normals and the computed vertex normals.
    {sections:[[new Vector3(0,0,0),new Vector3(1,0,0),new Vector3(0,NaN,0)],[new Vector3(0,0,1),new Vector3(1,0,1),new Vector3(0,1,1)]],options:{}},
    {sections:[[new Vector3(0,0,0),new Vector3(1,0,0),new Vector3(0,1,0)],[new Vector3(0,0,Infinity),new Vector3(1,0,1),new Vector3(0,1,1)]],options:{}},
    {sections:[[new Vector3(Infinity,0,0),new Vector3(-Infinity,0,0),new Vector3(0,1,0)],[new Vector3(0,0,1),new Vector3(1,0,1),new Vector3(0,1,1)]],options:{capStart:true,capEnd:true}},
    {sections:[[new Vector3(NaN,NaN,NaN)],[new Vector3(NaN,NaN,NaN)]],options:{}},
    {sections:[[new Vector3(0,0,0),new Vector3(1,0,0),new Vector3(0,1,0)],[new Vector3(0,0,1),new Vector3(1,0,1),new Vector3(NaN,NaN,NaN)]],options:{capEnd:true}}
];
// A construction that the pinned module itself rejects is compared as the error name and
// message, so the degenerate contour crash is part of the contract.
function capture(action){
    try{return action();}catch(error){return {error:error.constructor.name,message:error.message};}
}
for(const {sections,options} of loftCases){
    const geometry=capture(()=>new LoftGeometry(sections,options));
    // Each row carries its own point count so ragged sections reach the module guard.
    const body=sections.map(section=>`${section.length} `
        +section.flatMap(point=>[token(point.x),token(point.y),token(point.z)]).join(' ')).join(' ');
    scenarios.push({line:`L ${sections.length} ${options.closed===false?0:1}`
        +` ${options.capStart?1:0} ${options.capEnd?1:0}${body?' '+body:''}`,
        expected:geometry.error?geometry:loftJSON(geometry)});
}
// DecalGeometry: box mesh, translated box mesh and a triangle soup without normals.
const decalCases=[
    ['D',0,0,0, 0,0,0, 1,1,1],
    ['D',0.5,0.5,0.5, 0,0,0, 1,1,1],
    ['D',0,0,0.6, 0,0,0, 2,2,2],
    ['D',0.2,-0.3,0.4, 0.4,0.75,-0.3, 1.5,0.75,2],
    ['D',0,0,0, 0.9,0,0, 0.05,0.05,0.05],
    ['D',3,3,3, 0,0,0, 1,1,1],
    ['T',0.25,0.5,-0.25, 0.25,0.5,-0.25, 0.3,0.2,0.1, 1,1,1],
    ['T',1,0,0, 0.4,0.4,0.4, 0,0,0, 1.6,1.6,1.6],
    ['E',0,0,0, 0,0,0, 1,1,1],
    ['E',0,0,0.2, 0,0,0, 0.5,0.5,0.5],
    // Degenerate and extreme projector sizes: a zero size divides by zero in the uv step, a
    // negative size flips the uv orientation and a huge size engulfs the whole mesh.
    ['D',0,0,0, 0,0,0, 0,1,1],
    ['D',0,0,0, 0,0,0, 1,0,1],
    ['D',0,0,0, 0,0,0, 1,1,0],
    ['D',0,0,0, 0.3,0.2,0.1, -1,1,1],
    ['D',0,0,0, 0,0,0, 1e6,1e6,1e6],
    ['E',0,0,0.2, 0,0,0, 0,0.5,0.5],
    ['T',0.25,0.5,-0.25, 0.25,0.5,-0.25, 0.3,0.2,0.1, 0,1,1]
];
for(const [kind,...values] of decalCases){
    let mesh;
    if(kind==='E'){mesh=triangleSoup();}
    else{mesh=new Mesh(new BoxGeometry(1,1,1));}
    if(kind==='T'){mesh.position.set(values[0],values[1],values[2]);values.splice(0,3);}
    mesh.updateMatrixWorld(true);
    const [px,py,pz,ox,oy,oz,sx,sy,sz]=values;
    const geometry=new DecalGeometry(mesh,new Vector3(px,py,pz),new Euler(ox,oy,oz),new Vector3(sx,sy,sz));
    const extra=mesh.position.lengthSq()===0?[]:[mesh.position.x,mesh.position.y,mesh.position.z];
    scenarios.push({line:kind==='T'
            ?`T ${extra.map(token).join(' ')} ${values.map(token).join(' ')}`
            :`${kind} ${values.map(token).join(' ')}`,
        expected:geometryJSON(geometry,['position','uv','normal'])});
}

const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0,components=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,140)}`);
    for(const name of ['position','uv','normal']) if(expected[name]) components+=expected[name].values.length;
    compared+=1;
}
console.log(`Loft/Decal parity: ${compared} scenarios (${components} attribute components) across LoftGeometry`
    +` (tube, taper, caps, closed/open, error paths) and DecalGeometry (box, translated box, triangle soup`
    +` without normals) match the pinned checkout; ${process.version}`);
