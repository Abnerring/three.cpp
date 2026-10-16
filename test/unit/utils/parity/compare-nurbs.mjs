// Differential check for the native NURBS addon ports.
//
// NURBSUtils.js, NURBSCurve.js, NURBSSurface.js and NURBSVolume.js are imported from the
// pinned checkout, the same scenarios are replayed there, and the probe output is compared
// field by field. Scenario tokens are generated here, so both sides always call with the
// same arguments; the probe echoes only the results.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-nurbs.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const moduleURL = relative => pathToFileURL(path.join(sourceDir, relative)).href;
const NURBSUtils=await import(moduleURL('examples/jsm/curves/NURBSUtils.js'));
const {NURBSCurve}=await import(moduleURL('examples/jsm/curves/NURBSCurve.js'));
const {NURBSSurface}=await import(moduleURL('examples/jsm/curves/NURBSSurface.js'));
const {NURBSVolume}=await import(moduleURL('examples/jsm/curves/NURBSVolume.js'));
const {Vector4}=await import(moduleURL('src/math/Vector4.js'));

const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>values.map(encode);
const point4=value=>[value.x,value.y,value.z,value.w];
const point3=value=>[value.x,value.y,value.z];

const scenarios=[];
// kinds: S span, B basis, P B-spline point, F basis derivatives, D B-spline derivatives,
// K "k over i", R rational derivatives, N NURBS derivatives, U surface point,
// V volume point, C curve, M NURBS surface object, L NURBS volume object.
const p4=values=>values.map(v=>new Vector4(...v));
const quadraticKnots=[0,0,0,1,2,3,3,3];
const quadraticPoints=p4([[0,0,0,1],[1,2,0,1],[2,0,0,1],[3,1,0,1],[4,0,0,1]]);
const weightedPoints=p4([[0,0,0,1],[1,2,0,0.5],[2,0,0,2],[3,1,0,1.5],[4,0,0,1]]);
// Clamped cubic: U.length - degree - 1 = 4 control points, domain [0, 1].
const cubicKnots=[0,0,0,0,1,1,1,1];
const cubicPoints=p4([[0,0,0,1],[1,3,0,1],[2,-1,0,2],[3,2,0,1]]);
const unit=[0,0,1,1];
const grid=[[new Vector4(0,0,0,1),new Vector4(0,1,0,2)],[new Vector4(1,0,0,3),new Vector4(1,1,0,4)]];
const volume=[[[new Vector4(0,0,0,1),new Vector4(0,1,0,1)],[new Vector4(1,0,0,1),new Vector4(1,1,0,1)]],
    [[new Vector4(0,0,1,1),new Vector4(0,1,1,1)],[new Vector4(1,0,1,1),new Vector4(1,1,1,1)]]];

function add(line,expected){scenarios.push({line,expected});}
function knotTokens(knots){return [knots.length,...knots.map(token)].join(' ');}
function pointTokens(points){return [points.length,...points.flatMap(point4).map(token)].join(' ');}

for(const [p,knots] of [[2,quadraticKnots],[3,cubicKnots],[1,[0,0,1,1]],[2,[0,1,2,3,4,5,6]]]){
    for(const u of [-1,0,0.25,0.5,1,1.5,2,2.9999,3,7]){
        add(`S ${token(p)} ${token(u)} ${knotTokens(knots)}`,{span:encode(NURBSUtils.findSpan(p,u,knots))});
    }
}
for(const [span,u,p,knots] of [[2,0.5,2,quadraticKnots],[3,1,2,quadraticKnots],[4,1.5,2,quadraticKnots],
    [1,0,1,[0,0,1,1]],[2,0.75,3,cubicKnots],[5,2,3,cubicKnots]]){
    add(`B ${token(span)} ${token(u)} ${token(p)} ${knotTokens(knots)}`,
        {basis:encodeArray(NURBSUtils.calcBasisFunctions(span,u,p,knots))});
}
for(const [p,knots,points,us] of [[2,quadraticKnots,quadraticPoints,[0,0.5,1,1.5,2,3]],
    [2,quadraticKnots,weightedPoints,[0.4,1.2,2.5]],[3,cubicKnots,cubicPoints,[0,0.5,1,1.6,2]]]){
    for(const u of us){
        add(`P ${token(p)} ${token(u)} ${knotTokens(knots)} ${pointTokens(points)}`,
            {point:encodeArray(point4(NURBSUtils.calcBSplinePoint(p,knots,points,u)))});
    }
}
for(const [span,u,p,n,knots] of [[2,0.5,2,2,quadraticKnots],[3,1,2,1,quadraticKnots],[2,0.75,3,3,cubicKnots]]){
    add(`F ${token(span)} ${token(u)} ${token(p)} ${token(n)} ${knotTokens(knots)}`,
        {ders:NURBSUtils.calcBasisFunctionDerivatives(span,u,p,n,knots).map(encodeArray)});
}
for(const [p,knots,points,u,nd] of [[2,quadraticKnots,quadraticPoints,1,2],[2,quadraticKnots,weightedPoints,0.5,3],
    [3,cubicKnots,cubicPoints,1.5,1]]){
    add(`D ${token(p)} ${token(u)} ${token(nd)} ${knotTokens(knots)} ${pointTokens(points)}`,
        {ders:NURBSUtils.calcBSplineDerivatives(p,knots,points,u,nd).map(value=>encodeArray(point4(value)))});
}
for(const [k,i] of [[0,0],[4,2],[6,3],[5,0],[7,7]]){
    add(`K ${token(k)} ${token(i)}`,{value:encode(NURBSUtils.calcKoverI(k,i))});
}
add(`R ${pointTokens(quadraticPoints)}`,{ders:NURBSUtils.calcRationalCurveDerivatives(quadraticPoints).map(value=>encodeArray(point3(value)))});
add(`R ${pointTokens(weightedPoints)}`,{ders:NURBSUtils.calcRationalCurveDerivatives(weightedPoints).map(value=>encodeArray(point3(value)))});
for(const [p,knots,points,u,nd] of [[2,quadraticKnots,quadraticPoints,1,1],[2,quadraticKnots,weightedPoints,0.5,2],
    [3,cubicKnots,cubicPoints,1.5,1]]){
    add(`N ${token(p)} ${token(u)} ${token(nd)} ${knotTokens(knots)} ${pointTokens(points)}`,
        {ders:NURBSUtils.calcNURBSDerivatives(p,knots,points,u,nd).map(value=>encodeArray(point3(value)))});
}
const surfaceTarget={x:0,y:0,z:0,set(x,y,z){this.x=x;this.y=y;this.z=z;return this;}};
// One grid block per plane: <rows> <cols> <pointCount> <components...>.
const volumeGridTokens='2 2 2 '+pointTokens(volume[0].flat())+' 2 2 '+pointTokens(volume[1].flat());
for(const [u,v] of [[0,0],[0,1],[1,0],[0.5,0.5],[0.25,0.75],[1,1]]){
    NURBSUtils.calcSurfacePoint(1,1,unit,unit,grid,u,v,surfaceTarget);
    add(`U 1 1 ${token(u)} ${token(v)} ${knotTokens(unit)} ${knotTokens(unit)} 2 2 ${pointTokens(grid.flat())}`,
        {point:encodeArray(point3(surfaceTarget))});
}
for(const [u,v,w] of [[0,0,0],[1,1,1],[0.5,0.5,0.5],[0.25,0.75,0.5]]){
    NURBSUtils.calcVolumePoint(1,1,1,unit,unit,unit,volume,u,v,w,surfaceTarget);
    add(`V 1 1 1 ${token(u)} ${token(v)} ${token(w)} ${knotTokens(unit)} ${knotTokens(unit)} ${knotTokens(unit)}`
        +` ${volumeGridTokens}`,{point:encodeArray(point3(surfaceTarget))});
}
for(const [degree,knots,points,start,end] of [[2,quadraticKnots,quadraticPoints,undefined,undefined],
    [2,quadraticKnots,quadraticPoints,3,5],[2,quadraticKnots,quadraticPoints,0,NaN],
    [3,cubicKnots,cubicPoints,undefined,undefined],[2,quadraticKnots,weightedPoints,1,6]]){
    const curve=new NURBSCurve(degree,knots,points,start,end);
    const times=[0,0.25,0.5,1];
    const sampled=[],tangents=[];
    for(const t of times){
        sampled.push(encodeArray(point3(curve.getPoint(t))));
        tangents.push(encodeArray(point3(curve.getTangent(t))));
    }
    const copy=curve.clone();
    const original=curve.controlPoints[0].x;
    copy.controlPoints[0].x=99;
    const json=curve.toJSON();
    add(`C ${token(degree)} ${start===undefined?0:1} ${token(start===undefined?0:start)}`
        +` ${end===undefined?0:1} ${token(end===undefined?0:end)} ${knotTokens(knots)} ${pointTokens(points)}`
        +` ${times.length} ${times.map(token).join(' ')}`,
    {
        startKnot:encode(curve.startKnot),
        endKnot:encode(curve.endKnot),
        points:sampled,
        tangents,
        json:{degree:encode(json.degree),knots:encodeArray(json.knots),
            controlPoints:json.controlPoints.map(encodeArray),
            startKnot:encode(json.startKnot),endKnot:encode(json.endKnot)},
        copy:{degree:encode(copy.degree),controlCount:copy.controlPoints.length,
            startKnot:encode(copy.startKnot),endKnot:encode(copy.endKnot),
            independent:curve.controlPoints[0].x===original,arcLengthDivisions:encode(copy.arcLengthDivisions)}
    });
}
const surface=new NURBSSurface(1,1,unit,unit,grid);
for(const [t1,t2] of [[0,0],[1,1],[0.5,0.5]]){
    surface.getPoint(t1,t2,surfaceTarget);
    add(`M 1 1 ${knotTokens(unit)} ${knotTokens(unit)} 2 2 ${pointTokens(grid.flat())} 1 ${token(t1)} ${token(t2)}`,
        {point:encodeArray(point3(surfaceTarget)),
            shape:[surface.controlPoints.length,surface.controlPoints[0].length]});
}
const nurbsVolume=new NURBSVolume(1,1,1,unit,unit,unit,volume);
for(const [t1,t2,t3] of [[0,0,0],[1,1,1],[0.25,0.75,0.5]]){
    nurbsVolume.getPoint(t1,t2,t3,surfaceTarget);
    add(`L 1 1 1 ${knotTokens(unit)} ${knotTokens(unit)} ${knotTokens(unit)} ${volumeGridTokens}`
        +` 1 ${token(t1)} ${token(t2)} ${token(t3)}`,
        {point:encodeArray(point3(surfaceTarget)),
            shape:[nurbsVolume.controlPoints.length,nurbsVolume.controlPoints[0].length,
                nurbsVolume.controlPoints[0][0].length]});
}
// Missing control points are the upstream TypeError on both sides.
const tooFew=p4([[0,0,0,1],[1,1,0,1]]);
let errorName='none';
try{NURBSUtils.calcBSplinePoint(2,quadraticKnots,tooFew,2);}catch(error){errorName=error.constructor.name;}
add(`P 2 2 ${knotTokens(quadraticKnots)} ${pointTokens(tooFew)}`,{error:errorName});

const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:256*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,`scenario ${index}: ${line.slice(0,120)}`);
    compared+=1;
}
console.log(`NURBS parity: ${compared} scenarios across NURBSUtils, NURBSCurve, NURBSSurface and NURBSVolume`
    +` from the pinned checkout; ${process.version}`);
