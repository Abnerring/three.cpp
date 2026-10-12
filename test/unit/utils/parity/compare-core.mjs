import path from 'node:path';
import fs from 'node:fs';
import { pathToFileURL } from 'node:url';
import { execFileSync } from 'node:child_process';

const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
const revision = execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim();
if (revision !== lock.commit) throw new Error(`Upstream mismatch: ${revision}; expected ${lock.commit}`);
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const expected = {};
function snapshot(object) {
    object.updateWorldMatrix(true, false);
    return {position:object.position.toArray(),scale:object.scale.toArray(),rotation:object.rotation.toArray(),quaternion:object.quaternion.toArray(),
        matrix:object.matrix.toArray(),matrixWorld:object.matrixWorld.toArray(),worldPosition:object.getWorldPosition(new T.Vector3()).toArray(),
        worldScale:object.getWorldScale(new T.Vector3()).toArray(),worldDirection:object.getWorldDirection(new T.Vector3()).toArray(),
        worldQuaternion:object.getWorldQuaternion(new T.Quaternion()).toArray()};
}
const parent = new T.Object3D(), nextParent = new T.Object3D(), child = new T.Object3D();
parent.position.set(-3,2,7);parent.rotateY(.43);parent.scale.setScalar(1.5);
child.position.set(2,-3,4);child.rotation.set(.2,.8,-.4,'YZX');child.scale.set(1.2,.7,1.4);child.pivot=new T.Vector3(1,2,-1);parent.add(child);
expected.initialTransform=snapshot(child);child.lookAt(9,3,-8);expected.lookAtTransform=snapshot(child);
nextParent.position.set(4,-1,2);nextParent.rotateZ(-.6);nextParent.scale.setScalar(.75);nextParent.attach(child);expected.attachedTransform=snapshot(child);
child.rotateX(.2).rotateOnWorldAxis(new T.Vector3(0,1,0),-.1).translateZ(2.2);expected.rotatedTransform=snapshot(child);
const local=new T.Vector3(2,-4,1);child.localToWorld(local);expected.localToWorld=local.toArray();child.worldToLocal(local);expected.worldToLocal=local.toArray();
expected.projections=[];
for(const coordinate of [T.WebGLCoordinateSystem,T.WebGPUCoordinateSystem])for(const reverse of [false,true]){
    const camera=new T.PerspectiveCamera(67,1.6,.3,888);camera.coordinateSystem=coordinate;camera._reversedDepth=reverse;camera.zoom=1.25;camera.filmOffset=3;camera.setViewOffset(1920,1080,240,120,640,480);
    camera.position.set(2,5,10);camera.scale.set(2,3,4);camera.lookAt(1,2,-3);camera.updateMatrixWorld();
    const min=new T.Vector2(),max=new T.Vector2(),size=new T.Vector2();camera.getViewBounds(12,min,max);camera.getViewSize(12,size);
    expected.projections.push({projection:camera.projectionMatrix.toArray(),inverse:camera.projectionMatrixInverse.toArray(),viewMatrix:camera.matrixWorldInverse.toArray(),boundsMin:min.toArray(),boundsMax:max.toArray(),viewSize:size.toArray(),focalLength:camera.getFocalLength(),effectiveFOV:camera.getEffectiveFOV()});
    const ortho=new T.OrthographicCamera(-4,6,5,-3,.2,100);ortho.coordinateSystem=coordinate;ortho._reversedDepth=reverse;ortho.zoom=1.4;ortho.setViewOffset(800,600,125,45,300,220);
    expected.projections.push({projection:ortho.projectionMatrix.toArray(),inverse:ortho.projectionMatrixInverse.toArray()});
}
const scene=new T.Scene();scene.uuid='scene';scene.name='sample';scene.background=new T.Color(0x336699);scene.backgroundIntensity=.7;scene.backgroundRotation.set(.1,.2,.3,'ZYX');scene.fog=new T.FogExp2(0x668899,.02);
const group=new T.Group();group.uuid='group';group.name='nested';
const box=new T.BoxGeometry(2,3,4,2,1,3);box.uuid='box';const plane=new T.PlaneGeometry(3,2,2,3);plane.uuid='plane';
const red=new T.MeshBasicMaterial({color:0xaa2211});red.uuid='red';const blue=new T.MeshBasicMaterial({color:0x1144aa});blue.uuid='blue';blue.opacity=.4;blue.transparent=true;
const a=new T.Mesh(box,red);a.uuid='a';a.name='first';a.position.set(2,3,-4);a.updateMatrix();
const b=new T.Mesh(box,red);b.uuid='b';b.visible=false;b.castShadow=true;b.userData.label='shared';
const nested=new T.Mesh(plane,[red,blue]);nested.uuid='nested';a.add(nested);group.add(a,b);scene.add(group);expected.serializedScene=scene.toJSON();
const clone=scene.clone();clone.uuid='scene-copy';const clonedGroup=clone.children[0];clonedGroup.uuid='group-copy';clonedGroup.children[0].uuid='a-copy';clonedGroup.children[1].uuid='b-copy';clonedGroup.children[0].children[0].uuid='nested-copy';expected.serializedClone=clone.toJSON();
const perspective=new T.PerspectiveCamera(62,1.5,.2,400);perspective.uuid='perspective';perspective.zoom=1.3;perspective.filmOffset=2;perspective.setViewOffset(1200,800,300,200,400,300);expected.serializedPerspective=perspective.toJSON();
const orthographic=new T.OrthographicCamera(-4,6,5,-3,.2,100);orthographic.uuid='orthographic';orthographic.zoom=1.4;orthographic.setViewOffset(800,600,125,45,300,220);orthographic.clearViewOffset();expected.serializedOrthographic=orthographic.toJSON();

class CountingGeometry extends T.BufferGeometry {
    constructor(marker) { super(); this.marker=marker; this.calls=0; }
    toJSON() { ++this.calls; return {...super.toJSON(),marker:this.marker}; }
}
class CountingMaterial extends T.MeshBasicMaterial {
    constructor(marker) { super(); this.marker=marker; this.calls=0; }
    toJSON(meta) { ++this.calls; return {...super.toJSON(meta),marker:this.marker}; }
}
class CountingMesh extends T.Mesh {
    constructor(geometry,material) { super(geometry,material); this.calls=0; this.order=null; }
    toJSON(meta) {
        ++this.calls;
        if(this.order)this.order.push(this.name);
        const data=super.toJSON(meta);
        data.object.extension=this.name;
        return data;
    }
}
{
    const firstGeometry=new CountingGeometry('first'),collisionGeometry=new CountingGeometry('collision'),uniqueGeometry=new CountingGeometry('unique');
    firstGeometry.uuid=collisionGeometry.uuid='geometry-first';uniqueGeometry.uuid='geometry-second';
    const firstMaterial=new CountingMaterial('first'),collisionMaterial=new CountingMaterial('collision'),uniqueMaterial=new CountingMaterial('unique');
    firstMaterial.uuid=collisionMaterial.uuid='material-first';uniqueMaterial.uuid='material-second';
    const root=new CountingMesh(firstGeometry,firstMaterial),branch=new CountingMesh(collisionGeometry,[collisionMaterial,uniqueMaterial,uniqueMaterial]);
    const leaf=new CountingMesh(uniqueGeometry,firstMaterial),sibling=new CountingMesh(firstGeometry,firstMaterial);
    root.uuid=root.name='cache-root';branch.uuid=branch.name='cache-branch';leaf.uuid=leaf.name='cache-leaf';sibling.uuid=sibling.name='cache-sibling';
    const order=[];root.order=branch.order=leaf.order=sibling.order=order;
    branch.add(leaf);root.add(branch,sibling);expected.sharedResourceCache=root.toJSON();
    const calls=()=>[firstGeometry.calls,collisionGeometry.calls,uniqueGeometry.calls,firstMaterial.calls,collisionMaterial.calls,uniqueMaterial.calls,root.calls,branch.calls,leaf.calls,sibling.calls];
    expected.sharedResourceCalls=calls();root.toJSON();expected.repeatedResourceCalls=calls();expected.customSerializerTraversal=order;
}
{
    class EncodedGeometry extends CountingGeometry { toJSON() { return {...super.toJSON(),uuid:'encoded-uuid'}; } }
    const original=new EncodedGeometry('original'),collision=new CountingGeometry('collision'),shared=new CountingMaterial('shared');
    original.uuid=collision.uuid='identity-uuid';shared.uuid='context-material';
    const first=new T.Mesh(original,shared),second=new T.Mesh(collision,shared);first.uuid='context-first';second.uuid='context-second';
    const meta={geometries:{},materials:{},textures:{},images:{},shapes:{},skeletons:{},animations:{},nodes:{}};
    const fragment=first.toJSON(meta),other=second.toJSON(meta);
    expected.contextMetadataBeforeExtraction=Object.hasOwn(meta.geometries['identity-uuid'],'metadata');
    // Snapshot prior fragments: root extraction mutates resource objects, as in Object3D.toJSON.
    expected.explicitContextFirst=structuredClone(fragment);expected.explicitContextSecond=structuredClone(other);
    for(const library of Object.keys(meta)) {
        const values=Object.values(meta[library]);
        for(const resource of values)delete resource.metadata;
        if(values.length)fragment[library]=values;
    }
    expected.explicitContextDocument=fragment;
    expected.contextMetadataAfterExtraction=Object.hasOwn(meta.geometries['identity-uuid'],'metadata');
    expected.explicitContextCalls=[original.calls,collision.calls,shared.calls];
}

const actual=JSON.parse(execFileSync(process.argv[3],[],{encoding:'utf8'}));
let assertions=0;
function compare(a,b,where){
    if(typeof a!==typeof b||a===null||b===null){if(a!==b)throw new Error(`${where}: ${JSON.stringify(a)} != ${JSON.stringify(b)}`);++assertions;return;}
    if(typeof a==='number'){
        if(!Number.isFinite(a)||!Number.isFinite(b)||Math.abs(a-b)>2e-12*Math.max(1,Math.abs(b)))throw new Error(`${where}: ${a} != three.js ${b}`);
        ++assertions;return;
    }
    if(typeof a!=='object'){if(a!==b)throw new Error(`${where}: ${a} != ${b}`);++assertions;return;}
    if(Array.isArray(a)!==Array.isArray(b))throw new Error(`${where}: array shape mismatch`);
    const ak=Object.keys(a).sort(),bk=Object.keys(b).sort();
    if(JSON.stringify(ak)!==JSON.stringify(bk))throw new Error(`${where}: keys differ ${ak} != ${bk}`);
    for(const key of ak)compare(a[key],b[key],`${where}${Array.isArray(a)?`[${key}]`:`.${key}`}`);
}
compare(actual,expected,'root');
console.log(`three.js r${T.REVISION} core/camera/scene parity passed: ${assertions} values.`);
