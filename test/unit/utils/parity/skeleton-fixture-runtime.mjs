import {upstreamModule} from './threejs-source.mjs';
const {Bone}=await upstreamModule('src/objects/Bone.js');
const {Skeleton}=await upstreamModule('src/objects/Skeleton.js');
const {Object3D}=await upstreamModule('src/core/Object3D.js');
const {Matrix4}=await upstreamModule('src/math/Matrix4.js');
const {DataTexture}=await upstreamModule('src/textures/DataTexture.js');
const {getConsoleFunction,setConsoleFunction}=await upstreamModule('src/utils.js');
export const classes={Bone,Skeleton};
const flags=d=>[d.writable??false,d.enumerable??false,d.configurable??false];
const simple=v=>v===undefined?'#undefined':typeof v==='function'?'#function':typeof v==='symbol'?'#symbol':typeof v==='number'?Number.isNaN(v)?'#NaN':v===Infinity?'#Infinity':v===-Infinity?'#-Infinity':Object.is(v,-0)?'#-0':v:v;
const failure=e=>e instanceof Error?'#'+e.name:{thrown:simple(e)};
function plain(v,seen=new Set()){if(v===null||typeof v!=='object')return simple(v);if(seen.has(v))return '#cycle';seen=new Set(seen);seen.add(v);if(ArrayBuffer.isView(v))return Array.from(v,x=>plain(x,seen));if(Array.isArray(v))return Array.from(v,x=>plain(x,seen));const out={};for(const k of Object.keys(v))out[k]=plain(v[k],seen);return out;}
function emit(s,entry){if(!s.active)return;s.events.push(entry);if(s.q.gcAt===s.events.length){gc();s.events.push(['gc']);}if(s.q.throwAt===s.events.length)throw new TypeError('observed');if(s.q.reenterAt===s.events.length&&!s.entered){s.entered=true;s.events.push(['reenter']);s.nested.update();}}
function effects(s,d,self){if(d.gc){gc();emit(s,['gc']);}for(const m of d.mutations??[]){const target=m.ref?s.refs[m.ref]:self;if(m.op==='call'){target[m.key](...(m.args??[]).map(x=>decode(x,s)));}else if(m.op==='delete')delete target[m.key];else target[m.key]=decode(m.value,s);emit(s,['mutation',m.op,m.key]);}if(d.throw)throw new TypeError('fixture');}
function properties(object,d,s){if(d.id)s.refs[d.id]=object;for(const p of d.props??[]){const descriptor={enumerable:p.enumerable??true,configurable:p.configurable??true};if(p.getter)descriptor.get=function(){emit(s,['getter',p.key]);effects(s,p,this);return decode(p.value,s);};else{descriptor.value=decode(p.value,s);descriptor.writable=p.writable??true;}Object.defineProperty(object,p.key,descriptor);}return object;}
function decode(d,s){if(d===null||typeof d!=='object')return d;switch(d.$){
 case 'undefined':case 'hole':return undefined;case '-0':return -0;case 'NaN':return NaN;case 'Infinity':return Infinity;case '-Infinity':return -Infinity;case 'symbol':return Symbol('fixture');case 'ref':return s.refs[d.id];
 case 'function':return function(...args){emit(s,['call',d.name??'',args.map(x=>x===null||typeof x!=='object'?simple(x):'#object')]);effects(s,d,this);return d.self?this:Object.hasOwn(d,'arg')?args[d.arg]:decode(d.result,s);};
 case 'convert':return {[Symbol.toPrimitive](hint){emit(s,['convert',hint]);effects(s,d,this);return decode(d.value,s);}};
 case 'array':{const a=[];a.length=(d.items??[]).length;for(let i=0;i<a.length;++i)if(d.items[i]?.$!=='hole')a[i]=decode(d.items[i],s);return properties(a,d,s);}
 case 'object':return properties({},d,s);
 case 'matrix':{const m=new Matrix4();if(d.elements)m.fromArray(d.elements.map(x=>decode(x,s)));return properties(m,d,s);}
 case 'float32':return properties(new Float32Array((d.items??[]).map(x=>decode(x,s))),d,s);
 case 'texture':return properties(new DataTexture(new Float32Array([1,2,3,4]),1,1),d,s);
 case 'bone':{const b=new Bone();if(d.id)s.refs[d.id]=b;b.name=d.name??'';if(d.world)b.matrixWorld.fromArray(d.world);if(d.local)b.matrix.fromArray(d.local);return properties(b,d,s);}
 case 'skeleton':return properties(new Skeleton(...(d.args??[]).map(x=>decode(x,s))),d,s);
 case 'proxy':{const target=decode(d.value,s),handler={get(t,k,r){emit(s,['get',String(k)]);return Reflect.get(t,k,r);},set(t,k,v,r){emit(s,['set',String(k)]);return Reflect.set(t,k,v,r);}};const p=Proxy.revocable(target,handler);if(d.revoked)p.revoke();return properties(p.proxy,d,s);}
 default:throw new Error('Unknown fixture '+d.$);
}}
function safe(fn){try{return fn();}catch(e){return failure(e);}}
function boneSnapshot(b){if(!b||typeof b!=='object')return simple(b);const out={};for(const k of ['id','uuid','name','type','isBone'])out[k]=safe(()=>simple(b[k]));for(const k of ['matrix','matrixWorld'])out[k]=safe(()=>plain(b[k].elements));for(const k of ['position','quaternion','scale'])out[k]=safe(()=>plain(b[k].toArray()));out.parent=safe(()=>b.parent===null?null:simple(b.parent?.id));out.children=safe(()=>b.children.map(c=>c.id));return out;}
function textureSnapshot(t){if(t===null||typeof t!=='object')return simple(t);return {id:t.id,uuid:t.uuid,type:t.type,format:t.format,version:t.version,isDataTexture:t.isDataTexture,source:{id:t.source.id,uuid:t.source.uuid,version:t.source.version},image:{width:t.image.width,height:t.image.height,data:plain(t.image.data)},generateMipmaps:t.generateMipmaps,flipY:t.flipY,unpackAlignment:t.unpackAlignment};}
function snapshot(v){if(v===null||typeof v!=='object')return simple(v);if(v.isBone)return boneSnapshot(v);const out={keys:Object.keys(v)};for(const k of ['uuid','bones','boneInverses','boneMatrices','boneTexture'])out[k]=safe(()=>k==='bones'?Array.from(v[k],boneSnapshot):k==='boneInverses'?Array.from(v[k],m=>m===null?null:plain(m?.elements)):k==='boneTexture'?textureSnapshot(v[k]):plain(v[k]));out.textureDataIdentity=safe(()=>v.boneTexture!==null&&v.boneTexture!==undefined?v.boneTexture.image.data===v.boneMatrices:null);return out;}
function metadata(C){const fn=f=>({name:f.name,length:f.length,source:Function.prototype.toString.call(f),constructible:(()=>{try{Reflect.construct(function(){},[],f);return true;}catch{return false;}})(),prototype:Object.hasOwn(f,'prototype'),nameFlags:flags(Object.getOwnPropertyDescriptor(f,'name')),lengthFlags:flags(Object.getOwnPropertyDescriptor(f,'length'))});const out={...fn(C),keys:Reflect.ownKeys(C),prototypeKeys:Reflect.ownKeys(C.prototype),prototypeDescriptor:flags(Object.getOwnPropertyDescriptor(C,'prototype')),constructorDescriptor:flags(Object.getOwnPropertyDescriptor(C.prototype,'constructor')),methods:{}};out.parent=C===Bone?Object.getPrototypeOf(C)===Object3D:Object.getPrototypeOf(C)===Function.prototype;out.prototypeParent=C===Bone?Object.getPrototypeOf(C.prototype)===Object3D.prototype:Object.getPrototypeOf(C.prototype)===Object.prototype;for(const k of out.prototypeKeys)if(k!=='constructor')out.methods[k]={...fn(C.prototype[k]),flags:flags(Object.getOwnPropertyDescriptor(C.prototype,k))};return out;}
function setup(q,s){const bones=[];for(let i=0;i<(q.count??3);++i){if(q.hole===i){bones.length++;continue;}const b=new Bone();b.name='b'+i;b.position.set(i+1,i*2,-i);b.updateMatrix();b.matrixWorld.copy(b.matrix);if(i&&q.chain!==false){bones[i-1]?.add(b);b.updateMatrixWorld(true);}bones.push(b);s.refs['b'+i]=b;}s.refs.inputBones=bones;let inverses=q.inverses?decode(q.inverses,s):undefined;const sk=new Skeleton(bones,inverses);s.refs.sk=sk;s.refs.initialMatrices=sk.boneMatrices;s.refs.initialInverses=sk.boneInverses;if(q.texture)sk.computeBoneTexture();return sk;}
function observe(value,s){return new Proxy(value,{get(t,k,r){emit(s,['get',String(k)]);return Reflect.get(t,k,r);},set(t,k,v,r){emit(s,['set',String(k)]);return Reflect.set(t,k,v,r);}});}
export function run(q){const randomDescriptor=Object.getOwnPropertyDescriptor(Math,'random'),logging=getConsoleFunction();let randomIndex=0;Object.defineProperty(Math,'random',{...randomDescriptor,value:()=>((randomIndex++*37+11)%997)/997});const s={q,events:[],refs:{},active:false,entered:false},out={};setConsoleFunction((...args)=>emit(s,['console',...args.map(simple)]));const C=classes[q.class??'Skeleton'];let target,oldGlobal,globalKey,oldBase;
 try{
  if(q.kind==='metadata')return metadata(C);
  if(q.kind==='construct'){
   s.active=true;const args=(q.args??[]).map(x=>decode(x,s));let NewTarget=C;
   if(q.newTarget||q.setter){NewTarget=function Target(){};NewTarget.prototype=Object.create(C.prototype);if(q.setter)Object.defineProperty(NewTarget.prototype,q.setter,{configurable:true,set(value){s.refs.partial=this;emit(s,['setter',q.setter,typeof value==='object'&&value!==null?'#object':simple(value)]);if(q.throwSetter)throw new TypeError('setter');Object.defineProperty(this,q.setter,{value,writable:true,enumerable:true,configurable:true});}});}
   if(q.badBase){oldBase=Object.getPrototypeOf(C);Object.setPrototypeOf(C,null);}
   target=q.withoutNew?Reflect.apply(C,undefined,args):Reflect.construct(C,args,NewTarget);out.prototype=Object.getPrototypeOf(target)===NewTarget.prototype;out.ownConstructor=Object.hasOwn(target,'constructor');
  }else{
   target=q.receiver?decode(q.receiver,s):setup(q,s);s.refs.sk=target;
   for(const field of q.fields??[])Object.defineProperty(target,field.key,{value:decode(field.value,s),writable:field.writable??true,enumerable:true,configurable:true});
   if(q.reenterAt){s.nested=new Skeleton([new Bone()]);s.nested.bones[0].matrixWorld.elements[12]=19;const first=target.bones[0];first.matrixWorld=observe(first.matrixWorld,s);}
   if(q.global){globalKey=q.global;oldGlobal=Object.getOwnPropertyDescriptor(globalThis,globalKey);const original=globalThis[globalKey];if(globalKey==='Float32Array')globalThis[globalKey]=function(...args){emit(s,['global','Float32Array',...args.map(simple)]);if(q.globalThrow)throw new TypeError('global');return new original(...args);};else{const m=Object.create(original);for(const key of ['sqrt','ceil','max'])Object.defineProperty(m,key,{get(){emit(s,['globalGet',key]);return function(...args){emit(s,['globalCall',key,...args.map(simple)]);if(q.globalThrow===key)throw new TypeError('global');return original[key](...args);};}});globalThis.Math=m;}}
   const receiver=q.observed?observe(target,s):target;s.active=true;
   const steps=q.steps??[{method:q.method,args:q.args??[]}];out.results=[];
   for(const step of steps){const args=(step.args??[]).map(x=>decode(x,s));const fn=(q.borrow?Skeleton.prototype:receiver)[step.method];const result=Reflect.apply(fn,receiver,args);s.active=false;out.results.push(result===receiver?'#self':result===target?'#target':result?.isBone?boneSnapshot(result):result&&Object.hasOwn(result,'boneMatrices')?snapshot(result):plain(result));s.active=true;}
  }
 }catch(e){out.error=failure(e);}finally{if(oldBase)Object.setPrototypeOf(C,oldBase);if(globalKey)Object.defineProperty(globalThis,globalKey,oldGlobal);s.active=false;out.events=s.events;out.randomCalls=randomIndex;if(target!==undefined)out.target=safe(()=>snapshot(target));else if(s.refs.partial)out.partial=safe(()=>snapshot(s.refs.partial));if(s.refs.inputBones){out.inputIdentity=safe(()=>target.bones===s.refs.inputBones);out.inverseIdentity=safe(()=>target.boneInverses===s.refs.initialInverses);out.matricesIdentity=safe(()=>target.boneMatrices===s.refs.initialMatrices);}Object.defineProperty(Math,'random',randomDescriptor);setConsoleFunction(logging);}
 return out;
}
export async function gcEvidence(){const rows=[];let skeleton,bone,clone,texture,data;let refs;{
 bone=new Bone();skeleton=new Skeleton([bone]);skeleton.computeBoneTexture();clone=skeleton.clone();texture=skeleton.boneTexture;data=skeleton.boneMatrices;refs=[skeleton,bone,clone,texture,data].map(v=>new WeakRef(v));}
 const settle=async()=>{await new Promise(setImmediate);gc();await new Promise(setImmediate);gc();};
 skeleton=null;bone=null;await settle();rows.push(refs.map(r=>!!r.deref()));
 clone=null;await settle();rows.push(refs.map(r=>!!r.deref()));
 texture=null;await settle();rows.push(refs.map(r=>!!r.deref()));
 data=null;await settle();rows.push(refs.map(r=>!!r.deref()));
 return rows;}
