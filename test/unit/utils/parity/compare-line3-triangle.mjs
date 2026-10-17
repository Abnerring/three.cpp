// Differential check for the native src/math/Line3.js and Triangle.js ports.
//
// A line reads a start and an end plus a probe point; a triangle reads three corners plus a probe point.
// The cases lean on what a hand port drifts on: a degenerate segment where the projection parameter would
// divide by zero, a point exactly on a corner, a point outside the edge plane, and the winding that decides
// which way the normal points.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-line3-triangle.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const {Line3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Line3.js')).href);
const {Triangle}=await import(pathToFileURL(path.join(sourceDir,'src/math/Triangle.js')).href);
const {Vector3}=await import(pathToFileURL(path.join(sourceDir,'src/math/Vector3.js')).href);
const {Plane}=await import(pathToFileURL(path.join(sourceDir,'src/math/Plane.js')).href);
const scenarios=[
    ['line3.closestPointToPoint',0,0,0, 10,0,0, 3,4,0],
    ['line3.closestPointToPoint',0,0,0, 10,0,0, -5,0,0],
    ['line3.closestPointToPoint',0,0,0, 0,0,0, 3,4,0],
    ['line3.distance',0,0,0, 3,4,0, 0,0,0],
    ['line3.distanceSq',0,0,0, 3,4,0, 0,0,0],
    ['line3.delta',0,0,0, 3,4,0, 0,0,0],
    ['line3.closestPointToPointParameter',0,0,0, 10,0,0, 3,0,0],
    ['line3.closestPointToPointParameter',0,0,0, 10,0,0, -5,0,0],
    ['line3.closestPointToPointParameter',0,0,0, 0,0,0, 3,0,0],
    ['tri.getNormal',0,0,0, 1,0,0, 0,1,0, 0,0,0],
    ['tri.getNormal',0,0,0, 0,1,0, 1,0,0, 0,0,0],
    ['tri.getArea',0,0,0, 2,0,0, 0,2,0, 0,0,0],
    ['tri.getArea',0,0,0, 1,0,0, 2,0,0, 0,0,0],
    ['tri.getMidpoint',0,0,0, 3,0,0, 0,3,0, 0,0,0],
    ['tri.containsPoint',0,0,0, 1,0,0, 0,1,0, 0.25,0.25,0],
    ['tri.containsPoint',0,0,0, 1,0,0, 0,1,0, 0.9,0.9,0],
    ['tri.containsPoint',0,0,0, 1,0,0, 0,1,0, 0,0,0],
    ['tri.closestPointToPoint',0,0,0, 1,0,0, 0,1,0, 2,2,0],
    ['tri.getPlane',0,0,0, 1,0,0, 0,1,0, 0,0,0]
];
const encode=value=>{
    if(Object.is(value,-0))return '-0';
    if(Number.isNaN(value))return 'NaN';
    if(value===Infinity)return 'Infinity';
    if(value===-Infinity)return '-Infinity';
    return value;
};
const lines=scenarios.map(parts=>parts.join(' '));
const raw=execFileSync(path.resolve(probePath),[],{input:lines.join('\n'),encoding:'utf8',maxBuffer:64*1024*1024});
const output=raw.trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
const v=(a,i)=>new Vector3(a[i],a[i+1],a[i+2]);
const call=(op,args)=>{
    const line=new Line3(v(args,0),v(args,3));
    const triangle=new Triangle(v(args,0),v(args,3),v(args,6));
    const p=v(args,9);
    switch(op){
        case 'line3.closestPointToPoint': return line.closestPointToPoint(p,true,new Vector3());
        case 'line3.distance': return line.distance();
        case 'line3.distanceSq': return line.distanceSq();
        case 'line3.delta': return line.delta(new Vector3());
        case 'line3.closestPointToPointParameter': return line.closestPointToPointParameter(p,true);
        case 'tri.getNormal': return triangle.getNormal(new Vector3());
        case 'tri.getArea': return triangle.getArea();
        case 'tri.getMidpoint': return triangle.getMidpoint(new Vector3());
        case 'tri.containsPoint': return triangle.containsPoint(p);
        case 'tri.closestPointToPoint': return triangle.closestPointToPoint(p,new Vector3());
        case 'tri.getPlane': return triangle.getPlane(new Plane()).normal;
        default: throw new Error('unknown op '+op);
    }
};
for(let index=0;index<scenarios.length;++index){
    const [op,...args]=scenarios[index];
    const value=call(op,args);
    const expected=typeof value==='number'?{value:encode(value)}
        :typeof value==='boolean'?{value:value}
        :{x:encode(value.x),y:encode(value.y),z:encode(value.z)};
    const actual=JSON.parse(output[index]);
    if(JSON.stringify(actual)!==JSON.stringify(expected)){
        console.log('MISMATCH',op,args.join(' '));
        console.log('  probe :',JSON.stringify(actual));
        console.log('  oracle:',JSON.stringify(expected));
    }
    assert.deepEqual(actual,expected,`${op} #${index}`);
}
console.log(`Line3/Triangle parity: ${scenarios.length} scenarios`);