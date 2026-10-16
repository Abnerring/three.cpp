// Differential check for the native examples/jsm/math/ConvexHull.js and
// examples/jsm/geometries/ConvexGeometry.js ports.
//
// The two pinned modules are imported from the checkout, the same scenarios are replayed there,
// and every probe line is compared with assert.deepEqual, so no tolerance is used anywhere.
// Point clouds are generated here exactly once and handed to both sides as tokens, which keeps
// the two hull computations on bit-identical binary64 inputs; the probe only replays the calls
// and echoes results.
//
// ConvexHull has no intersectLine, intersectsBox, intersectsSphere, distanceToPoint or
// closestPointToPoint in the pinned revision; Face.distanceToPoint is the only distance query
// of the module and is covered by the hull-facedistance scenarios. Degenerate clouds make the
// pinned setFromPoints() throw a TypeError ("Cannot read properties of undefined (reading
// 'point')") because v2/v3 stay undefined; those scenarios compare the error message of the
// native JSTypeError with the message of the pinned TypeError.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-convex-hull.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const moduleURL=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const {ConvexHull, VertexNode}=await import(moduleURL('examples/jsm/math/ConvexHull.js'));
const {ConvexGeometry}=await import(moduleURL('examples/jsm/geometries/ConvexGeometry.js'));
const {Vector3}=await import(moduleURL('src/math/Vector3.js'));
const {Ray}=await import(moduleURL('src/math/Ray.js'));

const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>Array.from(values,encode);
const token=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':String(value);
const tokens=values=>values.map(token).join(' ');
const v3=value=>[value.x,value.y,value.z];
const vector3Tokens=value=>tokens(v3(value));
const cloudTokens=points=>[token(points.length),...points.flatMap(point=>v3(point).map(token))].join(' ');
const v=(x,y,z)=>new Vector3(x,y,z);

// ---------------------------------------------------------------- state serialization
// Both sides number the half edges the same way: the faces are visited in `faces` order and
// every face cycle is walked from `face.edge`. An object that is not reachable through those
// arrays (a deleted face, a vertex that was never created by setFromPoints) is reported as
// null on both sides.
const edgeIndexOf=(edges,edge)=>edge===null||edge===undefined||edges.indexOf(edge)===-1?null:edges.indexOf(edge);
const vertexIndexOf=(hull,vertex)=>vertex===null||vertex===undefined||hull.vertices.indexOf(vertex)===-1
    ? null:hull.vertices.indexOf(vertex);
const faceIndexOf=(hull,face)=>face===null||face===undefined||hull.faces.indexOf(face)===-1
    ? null:hull.faces.indexOf(face);
const vertexChain=(hull,head)=>{
    const chain=[];
    let vertex=head;
    for(let guard=0;vertex!==null&&vertex!==undefined&&guard<4096;++guard){
        chain.push(vertexIndexOf(hull,vertex));
        vertex=vertex.next;
    }
    return chain;
};
const edgeState=(hull,edges,edge)=>({
    vertex:vertexIndexOf(hull,edge.vertex),
    head:vertexIndexOf(hull,edge.head()),
    tail:vertexIndexOf(hull,edge.tail()),
    prev:edgeIndexOf(edges,edge.prev),
    next:edgeIndexOf(edges,edge.next),
    twin:edgeIndexOf(edges,edge.twin),
    face:faceIndexOf(hull,edge.face),
    length:encode(edge.length()),
    lengthSquared:encode(edge.lengthSquared())});
const faceEdges=(hull,edges,face)=>{
    const faceEdgeState=[];
    let edge=face.edge;
    for(let guard=0;edge!==null&&edge!==undefined&&guard<64;++guard){
        faceEdgeState.push({...edgeState(hull,edges,edge),
            getEdgeMinus1:edgeIndexOf(edges,face.getEdge(-1)),
            getEdge0:edgeIndexOf(edges,face.getEdge(0)),
            getEdge1:edgeIndexOf(edges,face.getEdge(1)),
            getEdge2:edgeIndexOf(edges,face.getEdge(2)),
            getEdge3:edgeIndexOf(edges,face.getEdge(3))});
        edge=edge.next;
        if(edge===face.edge)break;
    }
    return faceEdgeState;
};
const hullState=hull=>{
    const edges=[];
    for(const face of hull.faces){
        let edge=face.edge;
        for(let guard=0;edge!==null&&edge!==undefined&&guard<64;++guard){
            edges.push(edge);
            edge=edge.next;
            if(edge===face.edge)break;
        }
    }
    return {
        tolerance:encode(hull.tolerance),
        vertices:hull.vertices.map(vertex=>({point:encodeArray(v3(vertex.point)),
            prev:vertexIndexOf(hull,vertex.prev),next:vertexIndexOf(hull,vertex.next),
            face:faceIndexOf(hull,vertex.face)})),
        faces:hull.faces.map(face=>({normal:encodeArray(v3(face.normal)),
            midpoint:encodeArray(v3(face.midpoint)),area:encode(face.area),
            constant:encode(face.constant),mark:face.mark,
            outside:vertexIndexOf(hull,face.outside),edges:faceEdges(hull,edges,face)})),
        newFaces:hull.newFaces.length,
        assignedEmpty:hull.assigned.isEmpty(),
        unassignedEmpty:hull.unassigned.isEmpty(),
        assignedHead:vertexIndexOf(hull,hull.assigned.head),
        assignedTail:vertexIndexOf(hull,hull.assigned.tail),
        assigned:vertexChain(hull,hull.assigned.head),
        unassignedHead:vertexIndexOf(hull,hull.unassigned.head),
        unassignedTail:vertexIndexOf(hull,hull.unassigned.tail),
        unassigned:vertexChain(hull,hull.unassigned.head)};
};
const initialHullState=hull=>{
    const extremes=hull._computeExtremes();
    const tolerance=hull.tolerance;
    hull._computeInitialHull();
    return {tolerance:encode(tolerance),
        min:extremes.min.map(vertex=>vertexIndexOf(hull,vertex)),
        max:extremes.max.map(vertex=>vertexIndexOf(hull,vertex)),
        nextVertexToAdd:vertexIndexOf(hull,hull._nextVertexToAdd()),
        state:hullState(hull)};
};
const geometryState=geometry=>({
    type:geometry.type,
    position:geometry.attributes.position?encodeArray(Array.from(geometry.attributes.position.array)):null,
    normal:geometry.attributes.normal?encodeArray(Array.from(geometry.attributes.normal.array)):null,
    uv:geometry.attributes.uv?encodeArray(Array.from(geometry.attributes.uv.array)):null,
    hasPosition:'position' in geometry.attributes,
    hasNormal:'normal' in geometry.attributes,
    hasUv:'uv' in geometry.attributes,
    positionItemSize:geometry.attributes.position.itemSize,
    normalItemSize:geometry.attributes.normal.itemSize,
    positionCount:geometry.attributes.position.count,
    normalCount:geometry.attributes.normal.count,
    indexIsNull:geometry.index===null});

const scenarios=[];
// A scenario whose pinned counterpart throws records the error message, which the native
// JSTypeError has to reproduce exactly. `label` names the point cloud in failure messages.
const add=(line,compute,label='')=>{
    let expected;
    try{
        expected=compute();
    }catch(error){
        expected={probeError:error.message};
    }
    scenarios.push({line,expected,label});
};

// ---------------------------------------------------------------- point clouds
const cubeCorners=[];
for(const x of [-1,1])
    for(const y of [-1,1])
        for(const z of [-1,1]) cubeCorners.push(v(x,y,z));
const cubeInterior=[...cubeCorners,v(0,0,0),v(0.25,0.25,-0.25),v(-0.5,0.5,0.5),v(0.75,-0.75,0.1)];
const cubeDuplicated=cubeCorners.flatMap(point=>[point.clone(),point.clone()]);
const cubeWithFacePoints=[...cubeCorners,v(0,0,1),v(0.5,0.5,1),v(-0.5,-0.5,1),v(0,0,-1),v(0.25,-0.25,-1)];
const cubeNearCoplanar=[...cubeCorners,v(0,0,1+1e-12),v(0.5,0.5,1-1e-12),v(1e-15,0,1)];
const cubeOffset=cubeCorners.map(point=>v(point.x+1e6,point.y-1e6,point.z+0.5));
const cubeTiny=cubeCorners.map(point=>v(point.x*1e-8,point.y*1e-8,point.z*1e-8));
const cubeWide=cubeCorners.map(point=>v(point.x*1e8,point.y*1e8,point.z*1e8));
const cubeSignedZero=[v(-0,-0,-0),v(1,-0,0),v(0,1,-0),v(-0,0,1),v(1,1,1),v(-1,-1,-1),v(1,-1,1),v(-1,1,-1)];
const spherePoints=[v(0,1,0),v(0,-1,0)];
for(let ring=1;ring<=3;++ring){
    const phi=Math.PI*ring/4;
    for(let step=0;step<8;++step){
        const theta=2*Math.PI*step/8;
        spherePoints.push(v(Math.sin(phi)*Math.cos(theta),Math.cos(phi),Math.sin(phi)*Math.sin(theta)));
    }
}
const gridPlane=[];
for(let x=0;x<4;++x)
    for(let y=0;y<4;++y) gridPlane.push(v(x-1.5,y-1.5,0));
const gridPlaneWithOffset=[...gridPlane,v(0,0,4),v(1,1,-3)];
const triangleCloud=[v(0,0,0),v(1,0,0),v(0,1,0)];
const twoPoints=[v(0,0,0),v(1,1,1)];
const onePoint=[v(0.5,-0.25,3)];
const noPoints=[];
const collinearPoints=[v(0,0,0),v(1,1,1),v(2,2,2),v(-1,-1,-1),v(0.5,0.5,0.5)];
const identicalPoints=[v(0.5,0.5,0.5),v(0.5,0.5,0.5),v(0.5,0.5,0.5),v(0.5,0.5,0.5)];
const nanCloud=[...cubeCorners.slice(0,6),v(Number.NaN,0,0),v(0,Number.NaN,0)];
const infiniteCloud=[...cubeCorners.slice(0,6),v(Infinity,0,0),v(0,-Infinity,0)];
const negativeZeroCloud=[...cubeCorners.slice(0,7),v(-0,-0,-0)];

const clouds=[
    ['cube',cubeCorners],
    ['cube-interior',cubeInterior],
    ['cube-duplicated',cubeDuplicated],
    ['cube-face-points',cubeWithFacePoints],
    ['cube-near-coplanar',cubeNearCoplanar],
    ['cube-offset',cubeOffset],
    ['cube-tiny',cubeTiny],
    ['cube-wide',cubeWide],
    ['cube-signed-zero',cubeSignedZero],
    ['sphere',spherePoints],
    ['grid-plane',gridPlane],
    ['grid-plane-offset',gridPlaneWithOffset],
    ['triangle',triangleCloud],
    ['two-points',twoPoints],
    ['one-point',onePoint],
    ['no-points',noPoints],
    ['collinear',collinearPoints],
    ['identical',identicalPoints],
    ['nan-cloud',nanCloud],
    ['infinite-cloud',infiniteCloud],
    ['negative-zero-cloud',negativeZeroCloud]];

const queryPoints=[v(0,0,0),v(1,1,1),v(-1,-1,-1),v(2,0,0),v(0,2,0),v(0,0,2),v(1.5,0,0),
    v(1.0000001,0,0),v(0.5,0.5,0.5),v(-0.5,-0.5,-0.5),v(1e6,1e6,1e6),v(-0,-0,-0),
    v(Number.NaN,0,0),v(Infinity,0,0),v(-Infinity,-Infinity,-Infinity),v(1e-300,0,0)];
const rays=[
    [v(0,0,-5),v(0,0,1)],
    [v(0,0,5),v(0,0,-1)],
    [v(0,0,0),v(1,0,0)],
    [v(5,0,0),v(-1,0,0)],
    [v(4,4,0),v(-1,-1,0)],
    [v(0,0,-5),v(0,0,-1)],
    [v(0,0,0),v(0,0,0)],
    [v(1,1,1),v(1,1,1)],
    [v(0,0,-5),v(0,0,Number.NaN)],
    [v(Number.NaN,0,-5),v(0,0,1)]];

// ---------------------------------------------------------------- scenarios
add('empty-hull',()=>hullState(new ConvexHull()));
// A second cloud for the repeated-setFromPoints scenarios; it has to be a real hull so that
// the replacement is visible in the dumped state.
const replacementCloud=[v(0,0,0),v(1,0,0),v(0,1,0),v(0,0,1),v(-1,-1,-1)];
for(const [cloudName,points] of clouds){
    const cloud=cloudTokens(points);
    const label=cloudName;
    add(`hull ${cloud}`,()=>hullState(new ConvexHull().setFromPoints(points)),label);
    add(`hull-initial ${cloud}`,()=>initialHullState((()=>{
        const hull=new ConvexHull();
        // The pinned module has no arena: the generator pushes the vertex nodes itself, which
        // is what the native probe does as well.
        for(const point of points) hull.vertices.push(new VertexNode(point));
        return hull;
    })()),label);
    add(`hull-makeempty ${cloud}`,()=>{
        const hull=new ConvexHull().setFromPoints(points);
        const returnedThis=hull.makeEmpty()===hull;
        return {returnedThis,state:hullState(hull)};
    },label);
    add(`hull-twice ${cloud} ${cloudTokens(replacementCloud)}`,()=>{
        const hull=new ConvexHull().setFromPoints(points);
        const returnedThis=hull.setFromPoints(replacementCloud)===hull;
        return {returnedThis,state:hullState(hull)};
    },label);
    add(`hull-contains ${cloud} ${cloudTokens(queryPoints)}`,()=>{
        const hull=new ConvexHull().setFromPoints(points);
        return {tolerance:encode(hull.tolerance),
            contains:queryPoints.map(point=>hull.containsPoint(point))};
    },label);
    add(`hull-facedistance ${cloud} ${cloudTokens(queryPoints)}`,()=>{
        const hull=new ConvexHull().setFromPoints(points);
        return {distances:queryPoints.map(point=>hull.faces.map(face=>encode(face.distanceToPoint(point))))};
    },label);
    add(`hull-geometry ${cloud}`,()=>geometryState(new ConvexGeometry(points)),label);
    for(const [origin,direction] of rays)
        add(`hull-ray ${cloud} ${vector3Tokens(origin)} ${vector3Tokens(direction)}`,()=>{
            const hull=new ConvexHull().setFromPoints(points);
            const ray=new Ray(origin.clone(),direction.clone());
            const target=new Vector3();
            const hit=hull.intersectRay(ray,target);
            return {hit:hit!==null,point:hit===null?null:encodeArray(v3(target)),
                intersects:hull.intersectsRay(ray)};
        },label);
}

// ---------------------------------------------------------------- run the probe
const input=scenarios.map(scenario=>scenario.line).join('\n');
const output=execFileSync(path.resolve(probePath),[],{input,encoding:'utf8',maxBuffer:512*1024*1024})
    .trim().split('\n');
assert.equal(output.length,scenarios.length,'one probe line per scenario');
let compared=0;
for(let index=0;index<scenarios.length;++index){
    const {line,expected,label}=scenarios[index];
    assert.deepEqual(JSON.parse(output[index]),expected,
        `${label===''?'empty hull':label}, scenario ${index}: ${line.slice(0,160)}`);
    compared+=1;
}
const counts={hull:0,initial:0,makeempty:0,twice:0,contains:0,facedistance:0,ray:0,geometry:0,empty:0,error:0};
for(const {line,expected} of scenarios){
    const kind=line.slice(0,line.indexOf(' ')===-1?line.length:line.indexOf(' '));
    if('probeError' in expected)counts.error+=1;
    else if(kind==='hull')counts.hull+=1;
    else if(kind==='hull-initial')counts.initial+=1;
    else if(kind==='hull-makeempty')counts.makeempty+=1;
    else if(kind==='hull-twice')counts.twice+=1;
    else if(kind==='hull-contains')counts.contains+=1;
    else if(kind==='hull-facedistance')counts.facedistance+=1;
    else if(kind==='hull-ray')counts.ray+=1;
    else if(kind==='hull-geometry')counts.geometry+=1;
    else counts.empty+=1;
}
console.log(`Convex hull parity: ${compared} scenarios (${counts.hull} hull states, ${counts.initial} initial-hull states, `
    +`${counts.makeempty} makeEmpty, ${counts.twice} repeated setFromPoints, ${counts.contains} containsPoint, `
    +`${counts.facedistance} Face.distanceToPoint, ${counts.ray} intersectRay, ${counts.geometry} ConvexGeometry, `
    +`${counts.empty} empty hull, ${counts.error} throwing degenerate clouds) match the pinned checkout bit for bit; `
    +`${process.version}`);
