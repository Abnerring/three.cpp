import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const number=n=>Number.isNaN(n)?'$NaN':n===Infinity?'$Infinity':n===-Infinity?'$-Infinity':Object.is(n,-0)?'$-0':n;
const read=n=>n==='$NaN'?NaN:n==='$Infinity'?Infinity:n==='$-Infinity'?-Infinity:n==='$-0'?-0:n;
const numbers=a=>a.map(number),vector=(a,fallback=[0,0,0])=>new T.Vector3(...(a??fallback).map(read));
function geometry(r){const g=new T.BufferGeometry();g.uuid='geometry';if(!r.missing)g.setAttribute('position',new T.Float32BufferAttribute(r.positions.map(read),3));if(r.indices)g.index=new T.Float32BufferAttribute(r.indices.map(read),1);if(r.start!==undefined)g.drawRange.start=read(r.start);if(r.count!==undefined)g.drawRange.count=read(r.count);if(r.largeSphere)g.boundingSphere=new T.Sphere(new T.Vector3(),100);return g;}
function create(name,g){return name==='Sprite'?new T.Sprite():new T[name](g);}
function hits(intersections,object){return intersections.map(h=>({distance:number(h.distance),point:numbers(h.point.toArray()),index:h.index===undefined?'$undefined':number(h.index),distanceToRay:h.distanceToRay===undefined?'$undefined':number(h.distanceToRay),uv:h.uv?numbers(h.uv.toArray()):'$undefined',object:h.object===object,ownsObject:h.object===object}));}
function rays(r){
    const g=geometry(r),object=create(r.type,g);object.uuid='object';object.position.copy(vector(r.position));object.scale.copy(vector(r.scale,[1,1,1]));object.rotation.setFromVector3(vector(r.rotation));
    const parent=new T.Object3D();parent.scale.copy(vector(r.parentScale,[1,1,1]));parent.add(object);parent.updateMatrixWorld(true);
    const ray=new T.Raycaster(vector(r.origin,[0,0,3]),vector(r.direction,[0,0,-1]),read(r.near??0),read(r.far??Infinity));if(r.threshold!==undefined)ray.params[r.type==='Points'?'Points':'Line'].threshold=r.threshold;
    if(r.type==='Sprite'){
        if(r.camera!=='none'){const camera=r.camera==='orthographic'?new T.OrthographicCamera(-2,2,2,-2,.1,30):new T.PerspectiveCamera(50,1,.1,30);camera.position.copy(vector(r.cameraPosition));camera.rotation.setFromVector3(vector(r.cameraRotation));camera.updateMatrixWorld(true);ray.camera=camera;}
        object.center.fromArray(r.center);object.material.rotation=read(r.spriteRotation);object.material.sizeAttenuation=r.attenuation;
    }
    const log=[],nested=[];
    if(r.reenter||r.swap){
        const attribute=new T.Float32BufferAttribute(r.positions.map(read),3);g.setAttribute('position',attribute);g.boundingSphere=new T.Sphere(new T.Vector3(),100);
        const other=create(r.type,geometry(r));other.position.set(2,1,0);other.updateMatrixWorld(true);
        const replacement=geometry(r);replacement.setAttribute('position',new T.Float32BufferAttribute([-1,0,-2,1,0,-2,0,1,-2],3));let entered=false;
        const getX=attribute.getX;attribute.getX=function(i){log.push(number(i));if(!entered){entered=true;if(r.swap)object.geometry=replacement;else other.raycast(ray,nested);}return getX.call(this,i);};
    }
    const intersections=[];let error='';try{object.raycast(ray,intersections);}catch(e){error=e.name;}
    const out={error,hits:hits(intersections,object),log};if(r.type==='Sprite')out.modelView=numbers(object.modelViewMatrix.elements);return out;
}
function distances(r){const g=geometry(r),line=create(r.type,g),old=new T.Float32BufferAttribute([17],1);if(r.old)g.setAttribute('lineDistance',old);let error='',self=false;try{self=line.computeLineDistances()===line;}catch(e){error=e.name;}const distance=g.getAttribute('lineDistance');return {error,self,present:!!distance,sameOld:distance===old,data:distance?numbers([...distance.array]):[]};}
const morph=o=>o.isSprite?null:{influences:o.morphTargetInfluences??'$undefined',dictionary:o.morphTargetDictionary??'$undefined'};
function copying(r){
    const g=new T.BufferGeometry();g.uuid='geometry';const a=new T.Float32BufferAttribute([0,0,0],3);a.name='target';g.morphAttributes.position=[a];
    const source=create(r.type,g);source.uuid='object';source.name='source';source.add(new T.Object3D());source.children[0].uuid='child';source.material.uuid='material';
    const target=create(r.type,g);target.uuid='target';target.add(new T.Object3D());
    if(r.type!=='Sprite'){source.morphTargetInfluences[0]=.8;target.morphTargetInfluences[0]=.3;if(r.array)source.material=[source.material,source.material];}
    else{source.geometry=g;source.center.set(.2,.8);source.count=9;target.geometry=new T.BufferGeometry();target.count=7;}
    const targetGeometry=target.geometry,defaultGeometry=create(r.type).geometry;target.copy(source,r.recursive);const clone=source.clone(r.recursive);clone.uuid='clone';if(clone.children.length)clone.children[0].uuid='clone-child';
    const out={sourceMorph:morph(source),targetMorph:morph(target),cloneMorph:morph(clone),targetGeometrySame:target.geometry===source.geometry,targetGeometryPreserved:target.geometry===targetGeometry,
        cloneGeometrySame:clone.geometry===source.geometry,cloneDefaultGeometry:clone.geometry===defaultGeometry,cloneType:clone.type,targetChildren:target.children.length,cloneChildren:clone.children.length,
        materialShared:Array.isArray(source.material)?clone.material[0]===source.material[0]:clone.material===source.material,sourceJSON:source.toJSON(),cloneJSON:clone.toJSON()};
    if(r.type==='Sprite')Object.assign(out,{center:clone.center.toArray(),cloneCount:clone.count,targetCount:target.count});else out.arrayCopied=!Array.isArray(source.material)||clone.material!==source.material;
    return JSON.parse(JSON.stringify(out));
}
function bounds(r){const object=create(r.type,geometry(r));object.position.copy(vector(r.position));object.scale.copy(vector(r.scale,[1,1,1]));object.rotation.setFromVector3(vector(r.rotation));if(object.isSprite)object.center.fromArray(r.center);object.updateMatrixWorld(true);const frustum=new T.Frustum().setFromProjectionMatrix(new T.Matrix4().makePerspective(-1,1,1,-1,1,10)),visible=object.intersectsFrustum(frustum),box=new T.Box3().setFromObject(object,r.precise);return {visible,min:numbers(box.min.toArray()),max:numbers(box.max.toArray())};}
function serialization(r){
    const geometry=new T.BufferGeometry();geometry.uuid='geometry';const object=create(r.type,geometry);object.uuid='object';const first=new T.LineBasicMaterial(),second=new T.LineBasicMaterial();first.uuid='first';second.uuid='second';object.material=first;const log=[];
    const geometryJSON=geometry.toJSON,materialJSON=first.toJSON;geometry.toJSON=function(){log.push('geometry');object.material=second;return geometryJSON.call(this);};const single=object.toJSON();geometry.toJSON=geometryJSON;object.material=[first,first];
    first.toJSON=function(meta){log.push('material');object.material[1]=second;return materialJSON.call(this,meta);};const multiple=object.toJSON();return JSON.parse(JSON.stringify({single,multiple,log}));
}
function bindings(r){
    const g=new T.BufferGeometry(),a=new T.Float32BufferAttribute([0,0,0],3);a.name='smile';g.morphAttributes.position=[a];
    const object=create(r.type,g),material=object.material,opacity=new T.PropertyBinding(object,'.material.opacity');opacity.setValue([.25],0);
    const out={opacity:material.opacity,version:material.version,flags:[!!object.isMesh,!!object.isLine,!!object.isLineLoop,!!object.isLineSegments,!!object.isPoints,!!object.isSprite],type:object.type,materialType:object.material.type};
    const replacement=new T.SpriteMaterial();object.material=replacement;opacity.setValue([.5],0);out.oldOpacity=material.opacity;out.newOpacity=replacement.opacity;opacity.unbind();opacity.setValue([.75],0);out.rebound=replacement.opacity;
    if(r.type==='Sprite'){const center=object.center;new T.PropertyBinding(object,'.center').setValue([.1,.9],0);new T.PropertyBinding(object,'.count').setValue([4],0);Object.assign(out,{center:object.center.toArray(),centerIdentity:center===object.center,count:object.count});}
    else{const influences=object.morphTargetInfluences;new T.PropertyBinding(object,'.morphTargetInfluences[smile]').setValue([.8],0);Object.assign(out,{morph:influences[0],morphIdentity:influences===object.morphTargetInfluences});}
    out.dirty=object.matrixWorldNeedsUpdate;return out;
}
const cases=[];
for(const type of ['Line','LineLoop','LineSegments','Points','Sprite'])cases.push({kind:'binding',type});
for(const type of ['Line','LineLoop','LineSegments','Points'])cases.push({kind:'serialize',type});
for(const type of ['Line','LineLoop','LineSegments','Points','Sprite'])for(const precise of [false,true])for(const position of [[0,0,-3],[4,1,-2],[0,0,3]])for(const center of [[.5,.5],[0,0]])cases.push({kind:'bounds',type,precise,positions:[-1,0,0,1,0,0,0,1,0],position,scale:[2,.5,1],rotation:[.2,.1,.3],center});
const positions=[-1,0,0,1,0,0,0,1,0,0,-1,0];
for(const type of ['Line','LineLoop','LineSegments'])for(const p of [[],[1,2,3],[-1,0,0,1,0,0],[-1,0,0,1,0,0,0,1,0],positions])for(const indexed of [false,true])cases.push({kind:'distance',type,positions:p,...(indexed?{indices:[0,1]}:{}),old:true});
for(const type of ['Line','LineSegments'])cases.push({kind:'distance',type,positions:[],missing:true});
for(const type of ['Line','LineLoop','LineSegments','Points']){
    for(const indices of [undefined,[0,1,2,3],[3,1,0,2]])for(const threshold of [0,.1,1,-.5,'0.2'])for(const origin of [[0,0,3],[.3,.3,3],[2,0,3]])cases.push({kind:'ray',type,positions,indices,threshold,origin});
    for(const [start,count] of [[0,0],[1,2],[-1,3],[.5,2],[0,-1],[8,3],['$NaN',3],[0,'$NaN'],[0,'$Infinity']])for(const indices of [undefined,[0,1,2,3]])cases.push({kind:'ray',type,positions,indices,start,count,threshold:.3,largeSphere:true});
    for(const scale of [[2,.5,1],[-1,1,1],[0,0,0],[1,1,1]])for(const parentScale of [[1,1,1],[3,2,1]])cases.push({kind:'ray',type,positions,scale,parentScale,position:[.2,.1,-1],rotation:[.1,.2,.3],threshold:.4,origin:[.2,.1,3],near:2,far:5});
    for(const effect of ['reenter','swap'])cases.push({kind:'ray',type,positions,threshold:.4,largeSphere:true,[effect]:true});
    cases.push({kind:'ray',type,positions:[],largeSphere:true},{kind:'ray',type,positions:[],missing:true,largeSphere:true});
}
for(const type of ['Line','LineLoop','LineSegments','Points','Sprite'])for(const recursive of [false,true])for(const array of [false,true])if(type!=='Sprite'||!array)cases.push({kind:'copy',type,recursive,array});
for(const camera of ['perspective','orthographic'])for(const attenuation of [false,true])for(const center of [[.5,.5],[0,0],[.2,.8]])for(const spriteRotation of [0,.4,Math.PI/2])for(const origin of [[0,0,2],[.4,-.2,2],[2,1,2]])cases.push({kind:'ray',type:'Sprite',positions:[],camera,attenuation,center,spriteRotation,origin,position:[0,0,-3],scale:[1.5,.8,1]});
cases.push({kind:'ray',type:'Sprite',positions:[],camera:'none',attenuation:true,center:[.5,.5],spriteRotation:0});
for(const camera of ['perspective','orthographic'])for(const scale of [[1,1,1],[-1,2,1],[0,0,1]])for(const cameraRotation of [[0,.4,0],[.2,.1,.3]])for(const near of [0,4])cases.push({kind:'ray',type:'Sprite',positions:[],camera,attenuation:false,center:[.2,.8],spriteRotation:.8,origin:[1,.3,2],position:[1,0,-2],scale,cameraPosition:[1,.2,3],cameraRotation,near,far:6});
const expected=cases.map(r=>r.kind==='distance'?distances(r):r.kind==='copy'?copying(r):r.kind==='binding'?bindings(r):r.kind==='bounds'?bounds(r):r.kind==='serialize'?serialization(r):rays(r));
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:32*1024*1024}));
assert.equal(actual.length,expected.length);let comparisons=0;
function compare(a,e,p){comparisons++;if(Array.isArray(e)){assert.ok(Array.isArray(a),p);assert.equal(a.length,e.length,p);e.forEach((value,i)=>compare(a[i],value,`${p}[${i}]`));}else if(e!==null&&typeof e==='object'){assert.deepEqual(Object.keys(a).sort(),Object.keys(e).sort(),p);for(const key of Object.keys(e))compare(a[key],e[key],`${p}.${key}`);}else if(typeof e==='number'&&typeof a==='number'){assert.ok(Math.abs(a-e)<=1e-11*Math.max(1,Math.abs(a),Math.abs(e)),`${p}: ${a} != ${e}`);}else assert.equal(a,e,p);}
for(let i=0;i<expected.length;++i){try{compare(actual[i],expected[i],`case ${i}`);}catch(error){console.error(cases[i]);console.error('actual',JSON.stringify(actual[i]));console.error('expected',JSON.stringify(expected[i]));throw error;}}
console.log(`Primitive objects parity: ${cases.length} scenarios, ${comparisons} comparisons passed (${lock.commit}).`);
