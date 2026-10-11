import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../upstream.json',import.meta.url)));
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!==lock.commit) throw new Error('Upstream mismatch');
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const number=v=>Number.isNaN(v)?'NaN':v===Infinity?'Infinity':v===-Infinity?'-Infinity':v;
const vec=v=>v.toArray().map(number);
const hits=results=>results.map(hit=>{
    const result={distance:number(hit.distance),point:vec(hit.point),object:hit.object?hit.object.name:null};
    for(const name of ['uv','uv1','normal','barycoord']) if(hit[name]) result[name]=vec(hit[name]);
    if(hit.faceIndex!==undefined) result.faceIndex=hit.faceIndex;
    if(hit.face) result.face={a:hit.face.a,b:hit.face.b,c:hit.face.c,normal:vec(hit.face.normal),materialIndex:hit.face.materialIndex};
    return result;
});
const run=(mesh,origin=new T.Vector3(.25,.4,4),direction=new T.Vector3(0,0,-1),near=0,far=Infinity)=>{
    mesh.updateWorldMatrix(true,false);
    return hits(new T.Raycaster(origin,direction,near,far).intersectObject(mesh));
};
const ray=query=>({origin:vec(query.ray.origin),direction:vec(query.ray.direction),camera:query.camera?query.camera.type:null});
const expected={};
const geometry=new T.PlaneGeometry(2,2),material=new T.MeshBasicMaterial(),plane=new T.Mesh(geometry,material);plane.name='plane';
geometry.setAttribute('uv1',new T.Float32BufferAttribute([0,2,2,2,0,0,2,0],2));
expected.sides=[T.FrontSide,T.BackSide,T.DoubleSide].map(side=>{
    material.side=side;return {above:run(plane),below:run(plane,new T.Vector3(.25,.4,-4),new T.Vector3(0,0,1)),miss:run(plane,new T.Vector3(3,3,4))};
});
material.side=T.FrontSide;
expected.distanceRanges=[[4,4],[4.001,9],[0,3.999],[0,4],[-1,5]].map(([near,far])=>run(plane,new T.Vector3(.25,.4,4),new T.Vector3(0,0,-1),near,far));
const boxGeometry=new T.BoxGeometry(2,3,4),box=new T.Mesh(boxGeometry,material);box.name='box';boxGeometry.computeBoundingBox();
expected.indexedBox=run(box,new T.Vector3(0,0,5));
const expanded=new T.Mesh(boxGeometry.toNonIndexed(),material);expanded.name='expanded';expected.nonIndexedBox=run(expanded,new T.Vector3(0,0,5));
const materials=Array.from({length:6},(_,i)=>new T.MeshBasicMaterial({side:i===4?T.DoubleSide:T.FrontSide}));
const grouped=new T.Mesh(boxGeometry,materials);grouped.name='grouped';expected.groupedBox=run(grouped,new T.Vector3(.25,.4,5));
boxGeometry.setDrawRange(24,3);expected.drawRangeBox=run(grouped,new T.Vector3(0,0,5));
boxGeometry.setDrawRange(24,0);expected.emptyDrawRange=run(grouped,new T.Vector3(0,0,5));boxGeometry.setDrawRange(0,Infinity);
const ungrouped=new T.Mesh(geometry,materials);ungrouped.name='ungrouped';expected.noGroups=run(ungrouped);
const parent=new T.Group();parent.position.set(1,-2,3);parent.rotation.set(.2,.3,-.1);parent.scale.set(-2,.5,1.5);
plane.position.set(2,1,-4);plane.rotation.set(-.2,.1,.4);parent.add(plane);parent.updateMatrixWorld(true);
const point=new T.Vector3(.25,.4,0).applyMatrix4(plane.matrixWorld),direction=new T.Vector3(0,0,-1).transformDirection(plane.matrixWorld),origin=point.clone().addScaledVector(direction,-5);
expected.transformedPlane=run(plane,origin,direction);
parent.remove(plane);plane.position.set(0,0,0);plane.rotation.set(0,0,0);
const morphGeometry=new T.PlaneGeometry(2,2),relative=new T.Float32BufferAttribute([0,0,2,0,0,2,0,0,2,0,0,2],3);
morphGeometry.morphAttributes.position=[relative];morphGeometry.morphTargetsRelative=true;
const morph=new T.Mesh(morphGeometry,material);morph.name='morph';morph.morphTargetInfluences[0]=.25;expected.relativeMorph=run(morph);
morphGeometry.morphTargetsRelative=false;relative.copyArray([-1,1,2,1,1,2,-1,-1,2,1,-1,2]);morphGeometry.boundingSphere=null;expected.absoluteMorph=run(morph);
const root=new T.Group(),farther=new T.Mesh(geometry,material);farther.name='farther';farther.position.z=-2;
root.add(farther,plane);root.layers.set(1);root.updateMatrixWorld(true);
const query=new T.Raycaster(new T.Vector3(.25,.4,4),new T.Vector3(0,0,-1));
expected.recursive=hits(query.intersectObject(root));expected.notRecursive=hits(query.intersectObject(root,false));
plane.layers.set(1);expected.layerFiltered=hits(query.intersectObject(root));plane.layers.set(0);
plane.visible=false;material.visible=false;expected.invisibleStillPickable=hits(query.intersectObject(root));plane.visible=true;material.visible=true;
expected.objects=hits(query.intersectObjects([farther,plane],false));
const target=[{distance:1,point:new T.Vector3(0,0,3),object:null}];query.intersectObject(plane,false,target);expected.appendTarget=hits(target);
class Gate extends T.Object3D { raycast(){return false;} }
const gate=new Gate();gate.add(plane);gate.updateMatrixWorld(true);expected.stoppedPropagation=hits(query.intersectObject(gate));
expected.cameraRays=[];
for(const coordinates of [T.WebGLCoordinateSystem,T.WebGPUCoordinateSystem]) for(const reversed of [false,true]) {
    const perspective=new T.PerspectiveCamera(55,1.6,.2,60),orthographic=new T.OrthographicCamera(-2,3,4,-1,.2,60);
    for(const camera of [perspective,orthographic]) {
        camera.coordinateSystem=coordinates;camera._reversedDepth=reversed;camera.updateProjectionMatrix();
        camera.position.set(2,3,7);camera.lookAt(0,-1,0);camera.updateMatrixWorld(true);query.setFromCamera(new T.Vector2(.3,-.4),camera);expected.cameraRays.push(ray(query));
    }
}
const controller=new T.Object3D();controller.position.set(1,2,3);controller.rotation.set(.1,.2,.3);controller.scale.set(2,3,4);controller.updateMatrixWorld(true);
query.setFromXRController(controller);expected.xrRay=ray(query);expected.params=query.params;
const actual=JSON.parse(execFileSync(process.argv[3],[],{encoding:'utf8'}));
let count=0;
function compare(actual,expected,where){
    if(typeof actual!==typeof expected||actual===null||expected===null){if(actual!==expected)throw new Error(`${where}: ${actual} != ${expected}`);}
    else if(typeof actual==='number'){if(Math.abs(actual-expected)>2e-11*Math.max(1,Math.abs(expected)))throw new Error(`${where}: ${actual} != ${expected}`);}
    else if(typeof actual==='object'){
        if(Array.isArray(actual)!==Array.isArray(expected)||JSON.stringify(Object.keys(actual).sort())!==JSON.stringify(Object.keys(expected).sort()))throw new Error(`${where}: structure differs`);
        for(const key of Object.keys(expected))compare(actual[key],expected[key],`${where}.${key}`);
        return;
    } else if(actual!==expected)throw new Error(`${where}: ${actual} != ${expected}`);
    ++count;
}
compare(actual,expected,'root');console.log(`three.js r${T.REVISION} raycast parity passed: ${count} values.`);
