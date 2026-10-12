import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
if(process.argv.length!==4)throw Error('Usage: node compare-skinning.mjs <three.js> <probe.exe>');
const upstream=path.resolve(process.argv[2]),lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const box=v=>v?[v.min.toArray(),v.max.toArray()]:null,sphere=v=>v?[v.center.toArray(),v.radius]:null;
const objectBounds=value=>({conservative:box(new T.Box3().setFromObject(value)),precise:box(new T.Box3().setFromObject(value,true)),
    frustum:[-12,-2,0,2,12].map(shift=>new T.Frustum().setFromProjectionMatrix(new T.Matrix4().multiplyMatrices(
        new T.Matrix4().makePerspective(-2,2,2,-2,.1,50),new T.Matrix4().makeTranslation(shift,0,-5))).intersectsObject(value))});
const pixels=t=>t?{width:t.source.data.width,height:t.source.data.height,data:[...t.source.data.data],version:t.version,sourceVersion:t.source.version,
    format:t.format,type:t.type,minFilter:t.minFilter,magFilter:t.magFilter,flipY:t.flipY,generateMipmaps:t.generateMipmaps,unpackAlignment:t.unpackAlignment}:null;
const skeletonState=s=>({inverses:s.boneInverses.map(x=>x.toArray()),world:s.bones.map(x=>x?.matrixWorld.toArray()??null),
    local:s.bones.map(x=>x?.matrix.toArray()??null),positions:s.bones.map(x=>x?[x.position.toArray(),x.quaternion.toArray(),x.scale.toArray()]:null),
    matrices:[...s.boneMatrices],texture:pixels(s.boneTexture)});
function skeletonCase(n,mode){
    const parent=new T.Object3D();parent.position.set(2,-3,4);parent.rotation.set(.1,.2,-.3);const bones=[];
    for(let i=0;i<n;++i){const bone=new T.Bone();bone.name=`bone${i}`;bone.uuid=bone.name;
        bone.position.set(i*.5,i+1,-i*.25);bone.rotation.set(i*.02,i*.03,-i*.01);bone.scale.set(1+i*.05,1,1-i*.01);
        if(mode===1&&i>0)bones[i-1].add(bone);else if(mode===2)parent.add(bone);bones.push(bone);
    }
    parent.updateMatrixWorld(true);for(const b of bones)if(!b.parent)b.updateMatrixWorld(true);
    if(mode===3&&n>0)bones[n-1]=null;
    const supplied=[];if(mode===4)for(let i=0;i<n+1;++i)supplied.push(new T.Matrix4());
    const value=new T.Skeleton(bones,supplied);value.uuid='skeleton';
    const result={inputBonesDistinct:value.bones!==bones,inverseInputSame:value.boneInverses===supplied,initial:skeletonState(value)};
    const clone=value.clone();result.clone={bonesDistinct:clone.bones!==value.bones,inversesSame:clone.boneInverses===value.boneInverses,
        matricesDistinct:clone.boneMatrices!==value.boneMatrices,textureNull:clone.boneTexture===null};
    for(let i=0;i<bones.length;++i)if(bones[i]){bones[i].position.x+=.25*(i+1);bones[i].rotation.y+=.3;}
    parent.updateMatrixWorld(true);for(const b of bones)if(b&&!b.parent)b.updateMatrixWorld(true);
    value.update();result.updated=skeletonState(value);const old=value.boneMatrices;value.computeBoneTexture();
    result.textureIdentity=[old!==value.boneMatrices,value.boneMatrices===value.boneTexture.source.data.data];
    value.update();result.textured=skeletonState(value);value.pose();result.posed=skeletonState(value);
    if(mode!==3||n===0){result.json=value.toJSON();result.find=[Boolean(value.getBoneByName('bone0')),Boolean(value.getBoneByName('absent'))];}
    let disposed=0;value.boneTexture.addEventListener('dispose',()=>++disposed);value.dispose();value.dispose();result.disposed=[disposed,value.boneTexture===null];
    return result;
}
function triangle(seed,skinned){
    const g=new T.BufferGeometry();g.uuid='geometry';g.setAttribute('position',new T.Float32BufferAttribute([-1,-1,0,1,-1,0,0,1,0],3));
    g.setAttribute('normal',new T.Float32BufferAttribute([0,0,1,0,0,1,0,0,1],3));g.setAttribute('uv',new T.Float32BufferAttribute([0,0,1,0,.5,1],2));
    if(seed%2)g.setIndex([0,1,2]);if(seed%3===1){g.morphAttributes.position=[new T.Float32BufferAttribute([0,0,1,0,0,2,0,0,3],3)];g.morphTargetsRelative=Boolean(seed%2);}
    if(skinned){g.setAttribute('skinIndex',new T.Uint16BufferAttribute([0,1,0,1,1,0,0,1,0,1,0,1],4));
        const sets=[[1,0,0,0,.5,.5,0,0,0,1,0,0],[.2,.8,0,0,.7,.3,0,0,.1,.9,0,0],[0,0,0,0,0,0,0,0,0,0,0,0],[-2,1,0,0,1,-2,0,0,-1,-1,0,0],[2,3,0,0,3,2,0,0,4,1,0,0]];
        g.setAttribute('skinWeight',new T.Float32BufferAttribute(sets[seed%5],4));}
    return g;
}
const hits=(values,owner)=>values.map(h=>({distance:h.distance,point:h.point.toArray(),object:h.object===owner,instanceId:h.instanceId??null,
    faceIndex:h.faceIndex??null,uv:h.uv?.toArray()??null,normal:h.normal?.toArray()??null,barycoord:h.barycoord?.toArray()??null,
    ...(h.face?{face:[h.face.a,h.face.b,h.face.c,h.face.normal.toArray(),h.face.materialIndex]}:{})}));
function skinCase(seed){
    const g=triangle(seed,true),material=new T.MeshBasicMaterial();material.uuid='material';material.side=T.DoubleSide;
    const mesh=new T.SkinnedMesh(g,material);mesh.uuid='mesh';mesh.position.set(.5,-.5,.25);mesh.scale.set(1,1.25,.75);mesh.bindMode=seed%2?T.DetachedBindMode:T.AttachedBindMode;
    const b0=new T.Bone(),b1=new T.Bone();b0.uuid='bone0';b1.uuid='bone1';b0.name='first';b1.name='second';b0.position.set(.25,.5,0);b1.position.set(0,1,.25);b0.add(b1);mesh.add(b0);mesh.updateMatrixWorld(true);
    const skeleton=new T.Skeleton([b0,b1]);skeleton.uuid='skeleton';if(seed%3===0)mesh.bind(skeleton,new T.Matrix4().makeTranslation(.5,0,-.25));else mesh.bind(skeleton);
    if(seed%4===0)mesh.normalizeSkinWeights();if(mesh.morphTargetInfluences)mesh.morphTargetInfluences[0]=.25;
    b0.position.x+=.5;b1.rotation.z=.1*seed;b1.position.y+=.25;mesh.position.x+=.25;mesh.updateMatrixWorld(true);skeleton.update();
    const vertices=[],directions=[],points=[];for(let i=0;i<3;++i){const v=new T.Vector3();mesh.getVertexPosition(i,v);vertices.push(v.toArray());points.push(v);const d=new T.Vector4(1,2,3,seed%2?0:2);mesh.applyBoneTransform(i,d);directions.push(d.toArray());}
    mesh.computeBoundingBox();mesh.computeBoundingSphere();const clone=mesh.clone();
    const center=new T.Vector3();for(const p of points)center.add(p);center.divideScalar(3).applyMatrix4(mesh.matrixWorld);
    const [a,b,c]=points.map(p=>p.clone().applyMatrix4(mesh.matrixWorld));const normal=new T.Vector3().subVectors(b,a).cross(new T.Vector3().subVectors(c,a)).normalize(),origin=center.clone().addScaledVector(normal,3);
    const intersections=new T.Raycaster(origin,normal.clone().negate()).intersectObject(mesh,false);
    return {vertices,directions,box:box(mesh.boundingBox),sphere:sphere(mesh.boundingSphere),bind:mesh.bindMatrix.toArray(),inverse:mesh.bindMatrixInverse.toArray(),
        matrices:[...skeleton.boneMatrices],weights:g.getAttribute('skinWeight').toJSON(),hits:hits(intersections,mesh),
        clone:[clone.skeleton===skeleton,clone.geometry===g,clone.material===material,clone.boundingBox!==mesh.boundingBox,clone.boundingSphere!==mesh.boundingSphere,clone.children[0]!==b0,clone.children[0].isBone],json:mesh.toJSON(),objectBounds:objectBounds(mesh)};
}
function instanceCase(seed){
    const g=triangle(seed,false),material=new T.MeshBasicMaterial();material.uuid='material';const count=[0,1,2,7,1.5][seed%5];
    const mesh=new T.InstancedMesh(g,material,count);mesh.uuid='mesh';mesh.position.set(.5,0,.25);mesh.updateMatrixWorld();
    const local=new T.Matrix4();for(let i=0;i<count;++i){local.makeTranslation(i*3,i*.25,-i*.5);local.scale(new T.Vector3(1+i*.1,1,1));mesh.setMatrixAt(i,local);}
    const color=new T.Color();mesh.getColorAt(-1,color);const defaultColor=color.toArray();mesh.setColorAt(1,new T.Color(.25,.5,.75));mesh.setColorAt(.5,new T.Color(.1,.2,.3));mesh.getColorAt(0,color);
    const read=new T.Matrix4();mesh.getMatrixAt(-.125,read);const source=new T.Mesh();source.morphTargetInfluences=[.2,-.3,.7];let error='';
    try{mesh.setMorphAt(0,source);}catch(e){error=e.name;}source.morphTargetInfluences=[0,0,0];if(mesh.morphTexture)mesh.getMorphAt(0,source);
    mesh.computeBoundingBox();mesh.computeBoundingSphere();const clone=mesh.clone(),rays=[];if(seed%5!==4)for(let i=0;i<3;++i)rays.push(hits(new T.Raycaster(new T.Vector3(.5+i*3,i*.25,5),new T.Vector3(0,0,-1)).intersectObject(mesh,false),mesh));
    const result={type:mesh.type,morphAbsent:mesh.morphTargetInfluences===undefined,defaultColor,color0:color.toArray(),matrixOffset:read.toArray(),
        instanceMatrix:mesh.instanceMatrix.toJSON(),instanceColor:mesh.instanceColor.toJSON(),versions:[mesh.instanceMatrix.version,mesh.instanceColor.version],
        box:box(mesh.boundingBox),sphere:sphere(mesh.boundingSphere),texture:pixels(mesh.morphTexture),morph:source.morphTargetInfluences,morphError:error,rays,
        clone:[clone.instanceMatrix!==mesh.instanceMatrix,clone.instanceColor!==mesh.instanceColor,clone.morphTexture!==mesh.morphTexture,clone.morphTexture.source===mesh.morphTexture.source,clone.boundingBox!==mesh.boundingBox,clone.boundingSphere!==mesh.boundingSphere],json:mesh.toJSON(),objectBounds:objectBounds(mesh)};
    const events=[];mesh.addEventListener('dispose',()=>events.push('mesh'));mesh.morphTexture.addEventListener('dispose',()=>events.push('texture'));mesh.dispose();result.dispose={events,textureNull:mesh.morphTexture===null};return result;
}
function fromJSONCase(){
    const bone=new T.Bone();bone.uuid='known';bone.name='retained';const original=new T.Skeleton([bone]);original.uuid='source';const json=original.toJSON();
    json.bones.push('missing');json.boneInverses.push(new T.Matrix4().makeTranslation(1,2,3).toArray());const result=new T.Skeleton([bone]);result.fromJSON(json,{known:bone});const clone=result.clone();
    return {length:result.bones.length,uuid:result.uuid,alias:result.bones[0]===result.bones[1],missingBone:result.bones[2].isBone,inverses:result.boneInverses.map(x=>x.toArray()),cloneInverseAlias:clone.boneInverses===result.boneInverses,sourceName:result.bones[1].name};
}
function edgeCases(){
    const result={},omitted=new T.InstancedMesh();result.omittedCount={undefined:omitted.count===undefined,length:omitted.instanceMatrix.count,jsonHasCount:JSON.stringify(omitted.toJSON()).includes('"count":')};
    const object={repeat:2},attribute=new T.InstancedBufferAttribute(new Int16Array([1,2,3,4]),2,true,object),clone=attribute.clone();
    result.attributeAlias=[clone.meshPerAttribute===object,clone.toJSON()];attribute.copy(new T.BufferAttribute(new Float64Array([.25,.75]),1));result.attributePlain=[attribute.meshPerAttribute===undefined,attribute.toJSON()];
    const bone=new T.Bone(),skeleton=new T.Skeleton([bone]);skeleton.update();skeleton.computeBoneTexture();const texture=skeleton.boneTexture,oldMatrices=skeleton.boneMatrices;let events=0;
    texture.addEventListener('dispose',()=>++events);skeleton.computeBoneTexture();result.retexture=[events,texture!==skeleton.boneTexture,oldMatrices!==skeleton.boneMatrices,pixels(texture),pixels(skeleton.boneTexture)];
    const retained=skeleton.boneTexture;skeleton.init();bone.matrixWorld.makeTranslation(1,2,3);skeleton.update();
    result.initRetainsTexture={textureSame:retained===skeleton.boneTexture,dataDetached:skeleton.boneMatrices!==retained.source.data.data,matrices:[...skeleton.boneMatrices],texture:pixels(retained)};
    const g=triangle(0,true),mesh=new T.SkinnedMesh(g);mesh.bind(new T.Skeleton([bone]),new T.Matrix4());g.getAttribute('skinIndex').setX(0,99);const target=new T.Vector3(1,2,3);let error='';
    try{mesh.applyBoneTransform(0,target);}catch(e){error=e.name;}result.invalidBone=[error,target.toArray()];mesh.bindMatrixInverse.makeTranslation(3,4,5);mesh.bindMode='unknown';mesh.updateMatrixWorld(true);result.unknownBind=mesh.bindMatrixInverse.toArray();
    result.counts=[undefined,null,false,true,'2',-.1,-1,1.5,NaN].map(count=>{try{const value=new T.InstancedMesh(new T.BufferGeometry(),new T.MeshBasicMaterial(),count);return {undefined:value.count===undefined,value:value.count??null,data:value.instanceMatrix.toJSON()};}catch(e){return e.name;}});
    return result;
}
function reentryCase(){
    const bone=new T.Bone(),skeleton=new T.Skeleton([bone,bone]),nested=new T.SkinnedMesh(triangle(0,true));nested.bind(skeleton,new T.Matrix4());
    const pos=nested.geometry.getAttribute('position');for(let i=0;i<3;++i)pos.setX(i,pos.getX(i)+100);
    const outer=new T.SkinnedMesh(triangle(0,true));outer.bind(skeleton,new T.Matrix4());outer.getVertexPosition=function(i,target){T.SkinnedMesh.prototype.getVertexPosition.call(this,i,target);if(i===0)nested.computeBoundingBox();return target;};outer.computeBoundingBox();outer.computeBoundingSphere();
    const first=new T.Bone(),second=new T.Bone();first.position.x=1;second.position.x=2;first.updateMatrixWorld();second.updateMatrixWorld();const pose=new T.Skeleton([first,second]);
    first.position.x=11;second.position.x=12;first.updateMatrixWorld();second.updateMatrixWorld();let calls=0;first.quaternion._onChange(()=>{++calls;pose.bones.length=0;});pose.pose();
    return {box:box(outer.boundingBox),sphere:sphere(outer.boundingSphere),pose:[calls,pose.bones.length,first.position.toArray(),second.position.toArray(),second.matrixWorld.toArray()]};
}
function animationCase(mode){
    const mesh=new T.SkinnedMesh(triangle(1,true)),first=new T.Bone(),second=new T.Bone();first.name='first';second.name='second';second.position.y=1;first.add(second);
    if(mode!==4)mesh.add(first);else{mesh.bindMode=T.DetachedBindMode;first.updateMatrixWorld(true);}mesh.updateMatrixWorld(true);
    const skeleton=new T.Skeleton([first,second]);mesh.bind(skeleton);skeleton.computeBoneTexture();
    const tracks=[new T.VectorKeyframeTrack(mode>=3?'second.position':'.bones[second].position',[0,1],[0,1,0,0,3,1]),
        new T.QuaternionKeyframeTrack(mode>=3?'first.quaternion':'.bones[first].quaternion',[0,1],[0,0,0,1,0,0,Math.sqrt(.5),Math.sqrt(.5)]),
        new T.NumberKeyframeTrack(mode>=3?'second.scale[y]':'.bones[second].scale[y]',[0,1],[1,2])];
    const clip=new T.AnimationClip('bones',1,tracks),mixer=new T.AnimationMixer(mesh),action=mixer.clipAction(clip);action.setLoop(mode===0?T.LoopOnce:mode===1?T.LoopRepeat:T.LoopPingPong,2);action.clampWhenFinished=true;action.play();
    const result={identity:[mesh.skeleton===skeleton,mesh.skeleton.bones[0]===first,mesh.skeleton.bones[1]===second,skeleton.boneMatrices===skeleton.boneMatrices]},samples=[];
    for(const delta of [.25,.5,.75,1,.5]){mixer.update(delta);mesh.updateMatrixWorld(true);if(mode===4)first.updateMatrixWorld(true);skeleton.update();const vertex=new T.Vector3();mesh.getVertexPosition(0,vertex);globalThis.gc?.();
        samples.push({time:action.time,enabled:action.enabled,paused:action.paused,position:second.position.toArray(),quaternion:first.quaternion.toArray(),scale:second.scale.toArray(),vertex:vertex.toArray(),matrices:[...skeleton.boneMatrices],textureVersion:skeleton.boneTexture.version});}
    action.stop();mesh.updateMatrixWorld(true);if(mode===4)first.updateMatrixWorld(true);skeleton.update();result.samples=samples;result.restored=[second.position.toArray(),first.quaternion.toArray(),second.scale.toArray()];return result;
}
function boundsEdgeCase(){
    const result={},frustum=new T.Frustum(),attempt=call=>{try{return call();}catch(e){return e.name;}};
    for(const skin of [true,false]){
        const mesh=skin?new T.SkinnedMesh():new T.InstancedMesh();mesh.geometry=null;mesh.boundingSphere=new T.Sphere(new T.Vector3(),1);
        result[skin?'skinCached':'instanceCached']=attempt(()=>frustum.intersectsObject(mesh));
        result[skin?'skinDirect':'instanceDirect']=attempt(()=>frustum.intersectsObject(mesh));
    }
    const computed=new T.SkinnedMesh();computed.geometry=null;computed.calls=0;
    computed.computeBoundingSphere=function(){++this.calls;this.boundingSphere=new T.Sphere(new T.Vector3(),1);};
    result.computed=attempt(()=>frustum.intersectsObject(computed));result.computedCalls=computed.calls;
    const invalid=new T.SkinnedMesh();invalid.calls=0;invalid.computeBoundingSphere=function(){++this.calls;this.boundingSphere=null;};
    result.invalidComputed=attempt(()=>frustum.intersectsObject(invalid));result.invalidComputedCalls=invalid.calls;
    const customBox=new T.SkinnedMesh(),customBoxResult=new T.Box3();customBox.boxCalls=0;customBox.provideBox=true;
    customBox.computeBoundingBox=function(){++this.boxCalls;this.boundingBox=this.provideBox?new T.Box3(new T.Vector3(),new T.Vector3(1,2,3)):null;};
    result.computedBox=attempt(()=>box(customBoxResult.setFromObject(customBox)));result.computedBoxCalls=customBox.boxCalls;
    customBox.provideBox=false;customBox.boundingBox=null;result.invalidComputedBox=attempt(()=>box(customBoxResult.setFromObject(customBox)));result.invalidComputedBoxCalls=customBox.boxCalls;
    const absent=new T.SkinnedMesh();absent.geometry=null;result.absent=attempt(()=>frustum.intersectsObject(absent));result.absentAllocated=Boolean(absent.boundingSphere);
    const noGeometry=new T.InstancedMesh();noGeometry.geometry=null;result.instanceAbsent=attempt(()=>frustum.intersectsObject(noGeometry));result.instanceAbsentAllocated=Boolean(noGeometry.boundingSphere);
    const geometry=triangle(0,true),bone=new T.Bone(),skeleton=new T.Skeleton([bone,bone]),mesh=new T.SkinnedMesh(geometry);mesh.bind(skeleton,new T.Matrix4());
    bone.matrixWorld.makeTranslation(10,0,0);mesh.isMesh=false;const value=new T.Box3();result.falseMeshFlag=box(value.setFromObject(mesh,true));mesh.isMesh=true;result.trueMeshFlag=box(value.setFromObject(mesh,true));
    mesh.geometry=null;mesh.boundingBox=new T.Box3(new T.Vector3(),new T.Vector3(1,1,1));result.boxNullGeometry=attempt(()=>box(value.setFromObject(mesh)));return result;
}
const expected={};for(const n of [0,1,2,5,17])for(let mode=0;mode<5;++mode)expected[`skeleton:${n}:${mode}`]=skeletonCase(n,mode);
for(let seed=0;seed<30;++seed)expected[`skin:${seed}`]=skinCase(seed);for(let seed=0;seed<15;++seed)expected[`instance:${seed}`]=instanceCase(seed);expected.fromJSON=fromJSONCase();expected.edges=edgeCases();expected.reentry=reentryCase();
for(let mode=0;mode<5;++mode)expected[`animation:${mode}`]=animationCase(mode);
expected.boundsEdges=boundsEdgeCase();
const actual=JSON.parse(execFileSync(path.resolve(process.argv[3]),[],{encoding:'utf8',maxBuffer:32*1024*1024}));
let comparisons=0;const keyOrderFailures=[];
function compare(a,b,at){
    ++comparisons;if(typeof b==='number'&&typeof a==='number'){
        assert.ok(a===b||Math.abs(a-b)<=1e-11*Math.max(1,Math.abs(a),Math.abs(b)),`${at}: ${a} != ${b}`);return;
    }
    if(b&&typeof b==='object'){
        assert.equal(Array.isArray(a),Array.isArray(b),at);
        if(at.includes('.json')&&JSON.stringify(Object.keys(a))!==JSON.stringify(Object.keys(b)))
            keyOrderFailures.push(`${at}: ${Object.keys(a)} != ${Object.keys(b)}`);
        assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),at);
        for(const key of Object.keys(b))compare(a[key],b[key],`${at}.${key}`);return;
    }
    assert.equal(a,b,at);
}
compare(actual,JSON.parse(JSON.stringify(expected)),'result');
assert.equal(keyOrderFailures.length,0,`JSON key order mismatches (${keyOrderFailures.length}):\n${keyOrderFailures.slice(0,12).join('\n')}`);
console.log(`Skinning and instancing parity: ${Object.keys(expected).length} scenarios / ${comparisons} comparisons against ${lock.commit}`);
