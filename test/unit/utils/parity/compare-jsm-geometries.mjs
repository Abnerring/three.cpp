// Differential check for the native addon geometry ports.
//
// The pinned modules are imported from the checkout, the same scenario list is replayed
// there, and every attribute value is compared with assert.deepEqual (no tolerances).
// Scenario text is generated here and echoed by the probe, so both sides always build the
// same geometry.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-jsm-geometries.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const THREE=await import(url('build/three.module.js'));
const ParametricFunctions=await import(url('examples/jsm/geometries/ParametricFunctions.js'));
const {ParametricGeometry}=await import(url('examples/jsm/geometries/ParametricGeometry.js'));
const {BoxLineGeometry}=await import(url('examples/jsm/geometries/BoxLineGeometry.js'));
const {RoundedBoxGeometry}=await import(url('examples/jsm/geometries/RoundedBoxGeometry.js'));
const {Vector3}=THREE;

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const jsonField=(geometry,names)=>{
    const data=geometry.toJSON();
    const fields={};
    for(const name of names) if(data[name]!==undefined) fields[name]=encode(data[name]);
    fields.type=data.type;
    return fields;
};
const attributeJSON=attribute=>attribute===null||attribute===undefined?null:{
    count:attribute.count,itemSize:attribute.itemSize,normalized:attribute.normalized,
    values:encodeArray(attribute.array)};
const geometryJSON=(geometry,names)=>{
    const result={};
    for(const name of names) result[name]=attributeJSON(geometry.getAttribute(name));
    result.hasIndex=geometry.index!==null;
    result.index=geometry.index===null?null:encodeArray(geometry.index.array);
    result.type=geometry.type;
    result.json=jsonField(geometry,['slices','stacks','width','height','depth','segments','radius']);
    return result;
};
const functions=[undefined,ParametricFunctions.plane,ParametricFunctions.klein,
    ParametricFunctions.mobius,ParametricFunctions.mobius3d];
const defaultFunction=(u,v,target)=>target.set(u,v,Math.cos(u)*Math.sin(v));

const scenarios=[];
// ParametricFunctions over a grid, including the klein() parameter swap.
for(const id of [1,2,3,4]){
    for(const u of [0,0.125,0.25,0.5,0.75,1]){
        for(const v of [0,0.25,0.5,1]){
            const target=new Vector3();
            functions[id](u,v,target);
            scenarios.push({line:`F ${id} ${token(u)} ${token(v)}`,
                expected:{point:[encode(target.x),encode(target.y),encode(target.z)]}});
        }
    }
}
// ParametricGeometry, including the default callback and fractional subdivision counts.
for(const [id,slices,stacks] of [[0,8,8],[1,3,2],[2,5,4],[3,4,3],[4,6,2],[0,2.5,3.5]]){
    const func=id===0?defaultFunction:functions[id];
    const geometry=new ParametricGeometry(func,slices,stacks);
    scenarios.push({line:`P ${id} ${token(slices)} ${token(stacks)}`,
        expected:geometryJSON(geometry,['position','normal','uv'])});
}
// BoxLineGeometry with integer and fractional segment counts.
for(const [w,h,d,ws,hs,ds] of [[1,1,1,1,1,1],[2,3,4,2,1,3],[1,1,1,0,1,1],[1.5,2.5,0.5,2.9,1.2,2]]){
    const geometry=new BoxLineGeometry(w,h,d,ws,hs,ds);
    scenarios.push({line:`B ${token(w)} ${token(h)} ${token(d)} ${token(ws)} ${token(hs)} ${token(ds)}`,
        expected:geometryJSON(geometry,['position'])});
}
// RoundedBoxGeometry, including the no-rounding case and a radius clamped by the smallest side.
for(const [w,h,d,segments,radius] of [[1,1,1,2,0.1],[2,1,3,1,0.25],[1,1,1,0,0.1],[2,2,2,2,2],[1,1,1,3,0.05],[1,1,1,2,NaN],[NaN,1,1,2,0.1],[1,1,1,NaN,0.1],[1,1,1,2,Infinity],[1,1,1,2,-1]]){
    const geometry=new RoundedBoxGeometry(w,h,d,segments,radius);
    scenarios.push({line:`R ${token(w)} ${token(h)} ${token(d)} ${token(segments)} ${token(radius)}`,
        expected:geometryJSON(geometry,['position','normal','uv'])});
}

const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0,values=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    const actual=JSON.parse(output[index]);
    assert.deepEqual(actual,expected,`scenario ${index}: ${line.slice(0,140)}`);
    for(const name of ['position','normal','uv']) if(expected[name]) values+=expected[name].values.length;
    if(expected.index) values+=expected.index.length;
    compared+=1;
}
console.log(`Addon geometry parity: ${compared} scenarios (${values} attribute components) across `
    +`ParametricFunctions, ParametricGeometry, BoxLineGeometry and RoundedBoxGeometry match the pinned checkout; ${process.version}`);
