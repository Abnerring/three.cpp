import {upstreamModule} from './threejs-source.mjs';
const {InstancedMesh}=await upstreamModule('src/objects/InstancedMesh.js');
const {SkinnedMesh}=await upstreamModule('src/objects/SkinnedMesh.js');
const {BatchedMesh}=await upstreamModule('src/objects/BatchedMesh.js');
const {Mesh}=await upstreamModule('src/objects/Mesh.js');
const {Bone}=await upstreamModule('src/objects/Bone.js');
const {Skeleton}=await upstreamModule('src/objects/Skeleton.js');
const {BufferGeometry}=await upstreamModule('src/core/BufferGeometry.js');
const {BufferAttribute}=await upstreamModule('src/core/BufferAttribute.js');
const {InstancedBufferAttribute}=await upstreamModule('src/core/InstancedBufferAttribute.js');
const {Matrix4}=await upstreamModule('src/math/Matrix4.js');
const {Vector3}=await upstreamModule('src/math/Vector3.js');
const {Vector4}=await upstreamModule('src/math/Vector4.js');
const {Color}=await upstreamModule('src/math/Color.js');
const {Ray}=await upstreamModule('src/math/Ray.js');
const {Box3}=await upstreamModule('src/math/Box3.js');
const {Sphere}=await upstreamModule('src/math/Sphere.js');
const {Frustum}=await upstreamModule('src/math/Frustum.js');
const {DataTexture}=await upstreamModule('src/textures/DataTexture.js');
const {MeshBasicMaterial}=await upstreamModule('src/materials/MeshBasicMaterial.js');
const {getConsoleFunction,setConsoleFunction}=await upstreamModule('src/utils.js');
export const classes={InstancedMesh,SkinnedMesh,BatchedMesh};
const constructors={...classes,Mesh,Bone,Skeleton,BufferGeometry,BufferAttribute,InstancedBufferAttribute,Matrix4,Vector3,Vector4,Color,Ray,Box3,Sphere,Frustum,DataTexture,MeshBasicMaterial,Float32Array,Uint32Array,Uint16Array,Int32Array};
const flags=d=>[d.writable??false,d.enumerable??false,d.configurable??false];
const simple=v=>v===undefined?'#undefined':typeof v==='function'?'#function':typeof v==='symbol'?'#symbol':typeof v==='number'?Number.isNaN(v)?'#NaN':v===Infinity?'#Infinity':v===-Infinity?'#-Infinity':Object.is(v,-0)?'#-0':v:v;
const failure=e=>e instanceof Error?'#'+e.name:{thrown:simple(e)};
function plain(v,seen=new Set()){if(v===null||typeof v!=='object')return simple(v);if(seen.has(v))return '#cycle';seen=new Set(seen);seen.add(v);if(ArrayBuffer.isView(v))return Array.from(v,x=>plain(x,seen));if(Array.isArray(v))return Array.from(v,x=>plain(x,seen));const out={};for(const k of Object.keys(v))out[k]=plain(v[k],seen);return out;}
function emit(s,entry){if(!s.active)return;s.events.push(entry);if(s.q.gcAt===s.events.length){gc();s.events.push(['gc']);}if(s.q.throwAt===s.events.length)throw new TypeError('observed');if(s.q.reenterAt===s.events.length&&!s.entered){s.entered=true;s.events.push(['reenter']);const r=s.q.reenter??{};const t=s.refs[r.ref??'target'];t[r.method??'computeBoundingBox'](...(r.args??[]).map(v=>decode(v,s)));}}
function effects(s,d,self){if(d.gc){gc();emit(s,['gc']);}for(const m of d.mutations??[]){const target=m.ref?s.refs[m.ref]:self;if(m.op==='call'){target[m.key](...(m.args??[]).map(x=>decode(x,s)));}else if(m.op==='delete')delete target[m.key];else target[m.key]=decode(m.value,s);emit(s,['mutation',m.op,m.key]);}if(d.throw)throw new TypeError('fixture');}
function properties(object,d,s){if(d.id)s.refs[d.id]=object;for(const p of d.props??[]){const descriptor={enumerable:p.enumerable??true,configurable:p.configurable??true};if(p.getter)descriptor.get=function(){emit(s,['getter',p.key]);effects(s,p,this);return decode(p.value,s);};else{descriptor.value=decode(p.value,s);descriptor.writable=p.writable??true;}Object.defineProperty(object,p.key,descriptor);}if(Object.hasOwn(d,'proto'))Object.setPrototypeOf(object,decode(d.proto,s));return object;}
function decode(d,s){if(d===null||typeof d!=='object')return d;switch(d.$){
 case 'path':{let v=s.refs[d.ref??'target'];for(const k of d.keys)v=v[k];return v;}
 case 'camera':return properties({matrixWorld:new Matrix4(),matrixWorldInverse:new Matrix4(),projectionMatrix:new Matrix4(),coordinateSystem:2000},d,s);
 case 'undefined':case 'hole':return undefined;case '-0':return -0;case 'NaN':return NaN;case 'Infinity':return Infinity;case '-Infinity':return -Infinity;case 'symbol':return Symbol('fixture');case 'ref':return s.refs[d.id];
 case 'function':return function(...args){emit(s,['call',d.name??'',args.map(x=>x===null||typeof x!=='object'?simple(x):'#object')]);effects(s,d,this);return d.self?this:Object.hasOwn(d,'arg')?args[d.arg]:decode(d.result,s);};
 case 'convert':return {[Symbol.toPrimitive](hint){emit(s,['convert',hint]);effects(s,d,this);return decode(d.value,s);}};
 case 'array':{const a=[];a.length=(d.items??[]).length;for(let i=0;i<a.length;++i)if(d.items[i]?.$!=='hole')a[i]=decode(d.items[i],s);return properties(a,d,s);}
 case 'object':return properties({},d,s);
 case 'matrix':{const m=new Matrix4();if(d.elements)m.fromArray(d.elements.map(x=>decode(x,s)));return properties(m,d,s);}
 case 'float32':return properties(new Float32Array((d.items??[]).map(x=>decode(x,s))),d,s);
 case 'texture':return properties(new DataTexture(new Float32Array([1,2,3,4]),1,1),d,s);
 case 'bone':{const b=new Bone();if(d.id)s.refs[d.id]=b;b.name=d.name??'';if(d.world)b.matrixWorld.fromArray(d.world);if(d.local)b.matrix.fromArray(d.local);return properties(b,d,s);}
 case 'new':return properties(Reflect.construct(constructors[d.class],(d.args??[]).map(x=>decode(x,s))),d,s);
 case 'geometry':return properties(geometry(d,s),d,s);
 case 'skeleton':return properties(new Skeleton(...(d.args??[]).map(x=>decode(x,s))),d,s);
 case 'proxy':{const target=decode(d.value,s),handler={get(t,k,r){emit(s,['get',String(k)]);return Reflect.get(t,k,r);},set(t,k,v,r){emit(s,['set',String(k)]);return Reflect.set(t,k,v,r);}};const p=Proxy.revocable(target,handler);if(d.revoked)p.revoke();return properties(p.proxy,d,s);}
 default:throw new Error('Unknown fixture '+d.$);
}}
function safe(fn){try{return fn();}catch(e){return failure(e);}}

function geometry(d,s){const g=new BufferGeometry();if(d.id)s.refs[d.id]=g;
 const attributes=d.attributes??{position:{items:[0,0,0,2,0,0,0,2,0],itemSize:3},normal:{items:[0,0,1,0,0,1,0,0,1],itemSize:3},uv:{items:[0,0,1,0,0,1],itemSize:2}};
 for(const [k,a]of Object.entries(attributes)){const A=constructors[a.array??'Float32Array'];g.setAttribute(k,new BufferAttribute(new A(a.items),a.itemSize,a.normalized));}
 if(d.index!==false)g.setIndex(d.index??[0,1,2]);
 if(d.skin){g.setAttribute('skinIndex',new BufferAttribute(new Uint16Array([0,1,0,0,0,1,0,0,0,1,0,0]),4));g.setAttribute('skinWeight',new BufferAttribute(new Float32Array(d.weights??[1,0,0,0,.5,.5,0,0,0,1,0,0]),4));}
 if(d.morph){g.morphAttributes.position=[new BufferAttribute(new Float32Array([0,0,1,2,0,1,0,2,1]),3)];g.morphTargetsRelative=d.relative??false;}
 if(d.bounds){g.computeBoundingBox();g.computeBoundingSphere();}return g;}
function textureSnapshot(t){if(t===null||typeof t!=='object')return simple(t);return{id:t.id,uuid:t.uuid,type:t.type,format:t.format,version:t.version,colorSpace:t.colorSpace,source:{id:t.source.id,uuid:t.source.uuid,version:t.source.version},image:{width:plain(t.image.width),height:plain(t.image.height),data:plain(t.image.data)}};}
function attributeSnapshot(a){if(a===undefined||a===null)return simple(a);return {itemSize:a.itemSize,count:a.count,normalized:a.normalized,version:a.version,array:plain(a.array),updateRanges:plain(a.updateRanges)};}
function geometrySnapshot(g){if(!g||typeof g!=='object')return simple(g);const attrs={};for(const k of Object.keys(g.attributes))attrs[k]=attributeSnapshot(g.attributes[k]);return{id:g.id,uuid:g.uuid,index:attributeSnapshot(g.index),attributes:attrs,drawRange:plain(g.drawRange),groups:plain(g.groups),boundingBox:plain(g.boundingBox),boundingSphere:plain(g.boundingSphere),morphTargetsRelative:g.morphTargetsRelative};}
function snapshot(t){if(t===null||typeof t!=='object')return simple(t);if(!t.isMesh)return plain(t);const out={keys:Object.keys(t)};for(const k of ['id','uuid','type','isInstancedMesh','isSkinnedMesh','isBatchedMesh','count','bindMode','perObjectFrustumCulled','sortObjects','maxInstanceCount','instanceCount','unusedVertexCount','unusedIndexCount','_maxInstanceCount','_maxVertexCount','_maxIndexCount','_nextIndexStart','_nextVertexStart','_geometryCount','_geometryInitialized','_visibilityChanged','_multiDrawCount','_multiDrawBytesPerElement'])out[k]=safe(()=>simple(t[k]));
 for(const k of ['boundingBox','boundingSphere','_instanceInfo','_geometryInfo','_availableInstanceIds','_availableGeometryIds','_multiDrawCounts','_multiDrawStarts','morphTargetInfluences'])out[k]=safe(()=>plain(t[k]));
 for(const k of ['instanceMatrix','instanceColor'])out[k]=safe(()=>attributeSnapshot(t[k]));for(const k of ['morphTexture','_matricesTexture','_indirectTexture','_colorsTexture'])out[k]=safe(()=>textureSnapshot(t[k]));
 out.geometry=safe(()=>geometrySnapshot(t.geometry));out.material=safe(()=>Array.isArray(t.material)?t.material.map(m=>m?.uuid):t.material?.uuid??simple(t.material));
 for(const k of ['matrixWorld','bindMatrix','bindMatrixInverse'])out[k]=safe(()=>plain(t[k]?.elements));out.skeleton=safe(()=>t.skeleton?{uuid:t.skeleton.uuid,bones:t.skeleton.bones.map(b=>({id:b.id,uuid:b.uuid,matrixWorld:plain(b.matrixWorld.elements)})),inverses:t.skeleton.boneInverses.map(m=>plain(m.elements))}:simple(t.skeleton));return out;}
function resultSnapshot(v){if(v===null||typeof v!=='object')return simple(v);if(Array.isArray(v))return v.map(resultSnapshot);if(v.isMesh)return snapshot(v);if(v.isMatrix4)return plain(v.elements);if(v.isVector3||v.isVector4||v.isColor)return plain(v.toArray());if(v.isBufferGeometry)return geometrySnapshot(v);if(v.isDataTexture)return textureSnapshot(v);if(Object.hasOwn(v,'distance')&&Object.hasOwn(v,'object')){const out={};for(const k of Object.keys(v))out[k]=k==='object'?v[k].id:resultSnapshot(v[k]);return out;}return plain(v);}
function metadata(C){const fn=f=>({name:f.name,length:f.length,source:Function.prototype.toString.call(f),constructible:(()=>{try{Reflect.construct(function(){},[],f);return true;}catch{return false;}})(),prototype:Object.hasOwn(f,'prototype'),nameFlags:flags(Object.getOwnPropertyDescriptor(f,'name')),lengthFlags:flags(Object.getOwnPropertyDescriptor(f,'length'))});const out={...fn(C),keys:Reflect.ownKeys(C),prototypeKeys:Reflect.ownKeys(C.prototype),prototypeDescriptor:flags(Object.getOwnPropertyDescriptor(C,'prototype')),constructorDescriptor:flags(Object.getOwnPropertyDescriptor(C.prototype,'constructor')),parent:Object.getPrototypeOf(C)===Mesh,prototypeParent:Object.getPrototypeOf(C.prototype)===Mesh.prototype,methods:{}};for(const k of out.prototypeKeys)if(k!=='constructor'){const d=Object.getOwnPropertyDescriptor(C.prototype,k);out.methods[k]={...fn(d.get??d.value),flags:flags(d),kind:d.get?'get':'method'};}return out;}
function setup(q,s){const name=q.class??'InstancedMesh';let target;
 if(name==='BatchedMesh'){target=new BatchedMesh(q.maxInstances??4,q.maxVertices??18,q.maxIndices??18,new MeshBasicMaterial());s.refs.target=target;for(const [i,d]of(q.geometries??[{}]).entries()){const id=target.addGeometry(geometry(d,s),q.reservedVertices??-1,q.reservedIndices??-1);s.refs['g'+i]=id;}for(const [i,id]of(q.instances??[0,0]).entries()){s.refs['i'+i]=target.addInstance(id);}}
 else{const g=geometry({...q.geometry,skin:name==='SkinnedMesh',morph:q.morph},s);target=Reflect.construct(classes[name],[g,new MeshBasicMaterial(),q.count??3]);s.refs.target=target;if(name==='SkinnedMesh'){const bones=[new Bone(),new Bone()];bones[1].matrixWorld.makeTranslation(q.boneX??3,0,0);const sk=new Skeleton(bones,[new Matrix4(),new Matrix4()]);s.refs.skeleton=sk;s.refs.b0=bones[0];s.refs.b1=bones[1];target.bind(sk,new Matrix4());}}
 s.refs.geometry=target.geometry;s.refs.material=target.material;return target;}
function globalHook(q,s){if(!q.global)return()=>{};const key=q.global,descriptor=Object.getOwnPropertyDescriptor(globalThis,key),original=globalThis[key];if(key==='Math'){const value=Object.create(original);for(const k of ['sqrt','ceil','max','abs','floor','min'])Object.defineProperty(value,k,{configurable:true,get(){emit(s,['globalGet',k]);return function(...args){emit(s,['globalCall',k,...args.map(simple)]);if(q.globalThrow===k)throw new TypeError('global');return Reflect.apply(original[k],original,args);};}});globalThis[key]=value;}else{globalThis[key]=function(...args){emit(s,['global',key,...args.map(simple)]);if(q.globalThrow)throw new TypeError('global');return Reflect.construct(original,args);};}return()=>Object.defineProperty(globalThis,key,descriptor);}
export function run(q){const randomDescriptor=Object.getOwnPropertyDescriptor(Math,'random'),logging=getConsoleFunction();let randomIndex=0;Object.defineProperty(Math,'random',{...randomDescriptor,value:()=>((randomIndex++*37+11)%997)/997});const s={q,events:[],refs:{},active:false,entered:false},out={};setConsoleFunction((...args)=>emit(s,['console',...args.map(simple)]));const C=classes[q.class??'InstancedMesh'];let target,restore=()=>{},oldBase;
 try{if(q.kind==='metadata')return metadata(C);
 if(q.kind==='construct'){s.active=true;const args=(q.args??[]).map(x=>decode(x,s));let NewTarget=C;if(q.newTarget||q.setter){NewTarget=function Target(){};NewTarget.prototype=Object.create(C.prototype);if(q.setter)Object.defineProperty(NewTarget.prototype,q.setter,{configurable:true,set(v){s.refs.partial=this;emit(s,['setter',q.setter,typeof v==='object'&&v!==null?'#object':simple(v)]);if(q.throwSetter)throw new TypeError('setter');Object.defineProperty(this,q.setter,{value:v,writable:true,enumerable:true,configurable:true});}});}if(q.badBase){oldBase=Object.getPrototypeOf(C);Object.setPrototypeOf(C,null);}target=q.withoutNew?Reflect.apply(C,undefined,args):Reflect.construct(C,args,NewTarget);s.refs.target=target;out.prototype=Object.getPrototypeOf(target)===NewTarget.prototype;out.ownConstructor=Object.hasOwn(target,'constructor');}
 else{target=q.receiver?decode(q.receiver,s):setup(q,s);s.refs.target=target;
 for(const [k,d]of Object.entries(q.refs??{}))s.refs[k]=decode(d,s);
 for(const f of q.fields??[]){const o=s.refs[f.ref??'target'];Object.defineProperty(o,f.key,{value:decode(f.value,s),writable:f.writable??true,enumerable:true,configurable:true});}
 for(const hook of q.listeners??[]){const o=s.refs[hook.ref??'target'];o.addEventListener(hook.type??'dispose',function(e){emit(s,['event',hook.ref??'target',e.type,e.target===o]);effects(s,hook,o);});}
 restore=globalHook(q,s);const receiver=q.observed?new Proxy(target,{get(t,k,r){emit(s,['get',String(k)]);return Reflect.get(t,k,r);},set(t,k,v,r){emit(s,['set',String(k)]);return Reflect.set(t,k,v,r);}}):target;s.active=true;out.results=[];
 for(const step of q.steps??[{method:q.method,args:q.args??[]}]){const o=step.ref?s.refs[step.ref]:receiver;let result;const args=(step.args??[]).map(x=>decode(x,s));if(step.get)result=o[step.get];else if(step.set){o[step.set]=decode(step.value,s);result=undefined;}else result=Reflect.apply((q.borrow?C.prototype:o)[step.method],o,args);if(step.save)s.refs[step.save]=result;s.active=false;out.results.push(result===receiver?'#self':result===target?'#target':resultSnapshot(result));s.active=true;}
 }
 }catch(e){out.error=failure(e);}finally{if(oldBase)Object.setPrototypeOf(C,oldBase);restore();s.active=false;out.events=s.events;out.randomCalls=randomIndex;if(target!==undefined)out.target=safe(()=>snapshot(target));else if(s.refs.partial)out.partial=safe(()=>snapshot(s.refs.partial));out.refs={};for(const k of q.inspect??[])out.refs[k]=safe(()=>resultSnapshot(s.refs[k]));Object.defineProperty(Math,'random',randomDescriptor);setConsoleFunction(logging);}return out;}
export async function gcEvidence(){const rows=[];for(const C of [InstancedMesh,SkinnedMesh,BatchedMesh]){let mesh,resource,geometry,material,weak;{mesh=C===BatchedMesh?new C(2,6,6):new C(new BufferGeometry(),new MeshBasicMaterial(),2);geometry=mesh.geometry;material=mesh.material;resource=C===InstancedMesh?mesh.instanceMatrix:C===BatchedMesh?mesh._matricesTexture:mesh.bindMatrix;mesh.userData.cycle=mesh;weak=[mesh,resource,geometry,material].map(v=>new WeakRef(v));}const settle=async()=>{await new Promise(setImmediate);gc();await new Promise(setImmediate);gc();};mesh=null;await settle();rows.push(weak.map(w=>!!w.deref()));resource=null;await settle();rows.push(weak.map(w=>!!w.deref()));geometry=null;await settle();rows.push(weak.map(w=>!!w.deref()));material=null;await settle();rows.push(weak.map(w=>!!w.deref()));}return rows;}
