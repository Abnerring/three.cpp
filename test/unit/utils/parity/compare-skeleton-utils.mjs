// Differential check for the native examples/jsm/utils/SkeletonUtils.js port.
//
// The pinned module and its core classes are imported from the checkout, the same literal
// rig is built there, and the probe output is compared field by field. The rig literals are
// duplicated on purpose: any drift makes the comparison fail rather than pass silently.
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw new Error('Usage: node compare-skeleton-utils.mjs <threejs-source-dir> <probe.exe>');
assert.equal(process.version,'v26.5.0','The differential oracle is pinned to the Node release recorded in upstream.json');
const [, , sourceDir, probePath]=process.argv;
const url=relative=>pathToFileURL(path.join(sourceDir,relative)).href;
const SkeletonUtils=await import(url('examples/jsm/utils/SkeletonUtils.js'));
const {Object3D}=await import(url('src/core/Object3D.js'));
const {Bone}=await import(url('src/objects/Bone.js'));
const {Skeleton}=await import(url('src/objects/Skeleton.js'));
const {SkinnedMesh}=await import(url('src/objects/SkinnedMesh.js'));
const {Vector3}=await import(url('src/math/Vector3.js'));
const {Quaternion}=await import(url('src/math/Quaternion.js'));
const {VectorKeyframeTrack}=await import(url('src/animation/tracks/VectorKeyframeTrack.js'));
const {QuaternionKeyframeTrack}=await import(url('src/animation/tracks/QuaternionKeyframeTrack.js'));
const {AnimationClip}=await import(url('src/animation/AnimationClip.js'));

const encode=value=>Number.isNaN(value)?'NaN':Object.is(value,-0)?'-0':
    value===Infinity?'Infinity':value===-Infinity?'-Infinity':value;
const encodeArray=values=>values.map(encode);
const v3=value=>[value.x,value.y,value.z];
const q4=value=>[value.x,value.y,value.z,value.w];

// Shared literal rig: 0 hip, 1 spine (child of hip), 2 arm (child of spine).
const sourceSpec=[
    {name:'hip',x:0,y:1,z:0,ax:0,ay:0,az:1,deg:0,parent:-1},
    {name:'spine',x:0,y:0.5,z:0,ax:0,ay:0,az:1,deg:30,parent:0},
    {name:'arm',x:-0.4,y:0.1,z:0,ax:1,ay:0,az:0,deg:15,parent:1}];
const targetSpec=[
    {name:'hip',x:0.25,y:0.9,z:0.1,ax:0,ay:1,az:0,deg:10,parent:-1},
    {name:'spine',x:0,y:0.4,z:0,ax:0,ay:1,az:0,deg:20,parent:0},
    {name:'arm',x:-0.3,y:0.2,z:0.05,ax:1,ay:0,az:0,deg:25,parent:1}];

function buildRig(spec,name){
    const root=new Object3D();root.name=name;
    const list=[];
    for(const entry of spec){
        const bone=new Bone();bone.name=entry.name;
        bone.position.set(entry.x,entry.y,entry.z);
        bone.quaternion.setFromAxisAngle(new Vector3(entry.ax,entry.ay,entry.az),entry.deg*Math.PI/180);
        if(entry.parent>=0)list[entry.parent].add(bone);else root.add(bone);
        list.push(bone);
    }
    root.updateMatrixWorld(true);
    const mesh=new SkinnedMesh();
    mesh.name=name+'Mesh';
    mesh.skeleton=new Skeleton(list);
    mesh.bindMatrix.makeTranslation(0.1,0,0.2);
    mesh.bindMatrixInverse.copy(mesh.bindMatrix).invert();
    root.add(mesh);
    root.updateMatrixWorld(true);
    return {root,mesh,bones:list};
}
function buildClip(){
    return new AnimationClip('wave',1,[
        new VectorKeyframeTrack('.bones[hip].position',new Float32Array([0,0.5,1]),
            new Float32Array([0,1,0, 0.1,1.2,0.2, 0.2,1.4,0.4])),
        new QuaternionKeyframeTrack('.bones[spine].quaternion',new Float32Array([0,0.5,1]),
            new Float32Array([0,0,0,1, 0.1,0,0,0.99498743710662, 0.2,0,0,0.97979589711327]))]);
}
const boneJSON=list=>list.map(bone=>({name:bone.name,position:encodeArray(v3(bone.position)),
    quaternion:encodeArray(q4(bone.quaternion)),scale:encodeArray(v3(bone.scale))}));

function expectedClone(){
    const source=buildRig(sourceSpec,'armature');
    const cloned=SkeletonUtils.clone(source.root);
    const hierarchy=[];
    cloned.traverse(node=>hierarchy.push({name:node.name,type:node.type,children:node.children.length,
        chained:node.parent!==null}));
    const clonedMesh=cloned.children[1];
    const rebinding=clonedMesh.skeleton.bones.map((bone,index)=>({name:bone.name,
        distinct:bone!==source.bones[index]}));
    return {hierarchy,rebinding,distinctSkeleton:clonedMesh.skeleton!==source.mesh.skeleton,
        distinctBones:rebinding.every(entry=>entry.distinct),
        bindMatrix:encodeArray(Array.from(clonedMesh.bindMatrix.elements)),
        sourceSkeletonUntouched:source.mesh.skeleton.bones[0]===source.bones[0]};
}
function expectedRetarget(withClip){
    const source=buildRig(sourceSpec,'source');
    const target=buildRig(targetSpec,'target');
    const output={};
    if(!withClip){
        const options={};
        SkeletonUtils.retarget(target.mesh,source.mesh,options);
        output.bones=boneJSON(target.bones);
        output.filled={preserveBoneMatrix:options.preserveBoneMatrix,preserveBonePositions:options.preserveBonePositions,
            useTargetMatrix:options.useTargetMatrix,hip:options.hip,
            hipInfluence:encodeArray(v3(options.hipInfluence)),scale:options.scale};
        const scaled=buildRig(targetSpec,'scaled');
        const adjusted={hip:'spine',scale:2,hipInfluence:new Vector3(1,0.5,0.25),hipPosition:new Vector3(0.1,0,-0.1),
            preserveBonePositions:false};
        SkeletonUtils.retarget(scaled.mesh,source.mesh,adjusted);
        output.adjustedBones=boneJSON(scaled.bones);
        output.adjustedFilled={preserveBoneMatrix:adjusted.preserveBoneMatrix,
            preserveBonePositions:adjusted.preserveBonePositions,useTargetMatrix:adjusted.useTargetMatrix};
        return output;
    }
    const clip=buildClip();
    const clipOptions={};
    const converted=SkeletonUtils.retargetClip(target.mesh,source.mesh,clip,clipOptions);
    output.clip={name:converted.name,duration:encode(converted.duration),
        tracks:converted.tracks.map(track=>({name:track.name,times:encodeArray(Array.from(track.times)),
            values:encodeArray(Array.from(track.values))}))};
    output.fps=encode(clipOptions.fps);
    output.bones=boneJSON(target.bones);
    const skeletonTarget=buildRig(targetSpec,'skeletonTarget');
    const skeletonClip=SkeletonUtils.retargetClip(skeletonTarget.mesh,source.mesh.skeleton,buildClip());
    output.skeletonFormTracks=skeletonClip.tracks.map(track=>track.name);
    output.skeletonFormDuration=encode(skeletonClip.duration);
    return output;
}

const modes=[['clone',expectedClone()],['retarget',expectedRetarget(false)],['retarget-clip',expectedRetarget(true)]];
let compared=0;
for(const [mode,expected] of modes){
    const raw=execFileSync(path.resolve(probePath),[mode],{encoding:'utf8',maxBuffer:64*1024*1024}).trim();
    assert.deepEqual(JSON.parse(raw),expected,`mode ${mode}`);
    compared+=1;
}
const clipTracks=expectedRetarget(true).clip.tracks.length;
console.log(`SkeletonUtils parity: ${compared} modes (clone, retarget with filled options and hip adjustment, `
    +`retargetClip sampling ${clipTracks} converted tracks) match the pinned checkout; ${process.version}`);
