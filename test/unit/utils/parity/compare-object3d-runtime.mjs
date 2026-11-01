import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {spawnSync,execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {cases} from './object3d-fixtures.mjs';
const options=new Map();for(let i=2;i<process.argv.length;i+=2){if(!['--output','--probe','--upstream'].includes(process.argv[i])||!process.argv[i+1])throw Error('Usage: --output new-directory [--probe executable] [--upstream pinned-checkout]');options.set(process.argv[i],process.argv[i+1])}
const upstream=path.resolve(options.get('--upstream')??'D:/project/DX/GitHub/three.js');
if(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw Error('Unexpected upstream');
const {Object3D}=await import(pathToFileURL(upstream+'/src/core/Object3D.js'));
const {Vector3}=await import(pathToFileURL(upstream+'/src/math/Vector3.js'));
const {Quaternion}=await import(pathToFileURL(upstream+'/src/math/Quaternion.js'));
const {Euler}=await import(pathToFileURL(upstream+'/src/math/Euler.js'));
const {Matrix4}=await import(pathToFileURL(upstream+'/src/math/Matrix4.js'));
const {EventDispatcher}=await import(pathToFileURL(upstream+'/src/core/EventDispatcher.js'));
const {setConsoleFunction}=await import(pathToFileURL(upstream+'/src/utils.js'));
const fail=e=>e instanceof Error?'#'+e.name:{thrown:e};
function special(request){
 const scalar=v=>typeof v==='function'?'#function':v===undefined?'#undefined':v;
 if(request.special==='metadata'){
  const descriptor=d=>{const result={};for(const key of ['value','writable','get','set','enumerable','configurable'])if(key in d)result[key]=scalar(d[key]);return result};
  return {constructorKeys:Reflect.ownKeys(Object3D),staticDescriptors:Object.fromEntries(['DEFAULT_UP','DEFAULT_MATRIX_AUTO_UPDATE','DEFAULT_MATRIX_WORLD_AUTO_UPDATE'].map(key=>[key,descriptor(Object.getOwnPropertyDescriptor(Object3D,key))])),name:Object3D.name,length:Object3D.length,source:Function.prototype.toString.call(Object3D),constructorParent:Object.getPrototypeOf(Object3D)===EventDispatcher,prototypeParent:Object.getPrototypeOf(Object3D.prototype)===EventDispatcher.prototype,keys:Reflect.ownKeys(Object3D.prototype),methods:Reflect.ownKeys(Object3D.prototype).filter(k=>k!=='constructor').map(key=>{const d=Object.getOwnPropertyDescriptor(Object3D.prototype,key);return {key,descriptor:descriptor(d),name:d.value.name,length:d.value.length,source:Function.prototype.toString.call(d.value)}})};
 }
 if(request.special==='defaults'){
  const up=Object3D.DEFAULT_UP,auto=Object3D.DEFAULT_MATRIX_AUTO_UPDATE,world=Object3D.DEFAULT_MATRIX_WORLD_AUTO_UPDATE;
  try{Object3D.DEFAULT_UP=new Vector3(2,3,4);Object3D.DEFAULT_MATRIX_AUTO_UPDATE='local';Object3D.DEFAULT_MATRIX_WORLD_AUTO_UPDATE=0;const value=new Object3D();return {up:value.up.toArray(),copied:value.up!==Object3D.DEFAULT_UP,auto:value.matrixAutoUpdate,world:value.matrixWorldAutoUpdate}}finally{Object3D.DEFAULT_UP=up;Object3D.DEFAULT_MATRIX_AUTO_UPDATE=auto;Object3D.DEFAULT_MATRIX_WORLD_AUTO_UPDATE=world}
 }
 if(request.special==='constructorDescriptors'){
  const value=new Object3D(),out={keys:Reflect.ownKeys(value),descriptors:{}};
  for(const key of ['id','position','rotation','quaternion','scale','modelViewMatrix','normalMatrix']){const d=Object.getOwnPropertyDescriptor(value,key);out.descriptors[key]={writable:d.writable,enumerable:d.enumerable,configurable:d.configurable}}
  try{value.position=new Vector3();out.assignment='accepted'}catch(e){out.assignment=fail(e)}
  const old=value.quaternion,rotation=value.rotation;value.rotation.x=.6;out.rotationCallback=old.x;old.set(0,Math.sin(.2),0,Math.cos(.2));out.quaternionCallback=rotation.toArray();return out;
 }
 if(request.special==='callbackBindings'){
  const value=new Object3D(),old=value.quaternion,replacement=new Quaternion();Object.defineProperty(value,'quaternion',{value:replacement});value.rotation.x=.4;const first={old:old.toArray(),replacement:replacement.toArray()};replacement.set(0,.3,0,.7);const unchanged=value.rotation.toArray();old.set(0,Math.sin(.2),0,Math.cos(.2));return {first,unchanged,synchronized:value.rotation.toArray()};
 }
 if(request.special==='defineProperties'){
  const events=[],target={},properties={};Object.defineProperty(properties,'a',{enumerable:true,get(){events.push('get:a');return {get value(){events.push('value:a');return 1},enumerable:true}}});Object.defineProperty(properties,'b',{enumerable:true,get(){events.push('get:b');throw 17}});
  let error;try{Object.defineProperties(target,properties)}catch(e){error=fail(e)}const result={events,error,unchanged:Reflect.ownKeys(target)};
  const key=Symbol('key'),success={a:{value:2,writable:true},[key]:{value:3,enumerable:true}};Object.defineProperty(success,'hidden',{get(){throw 23}});Object.defineProperties(target,success);result.values=[target.a,target[key]];result.keys=Reflect.ownKeys(target).map(k=>typeof k==='symbol'?'#symbol':k);
  const before=events.length;try{Object.defineProperties(5,properties)}catch(e){result.badTarget=fail(e)}result.badTargetReads=events.length-before;return result;
 }
 if(request.special==='eventKeys'){
  const object=new EventDispatcher(),symbol=Symbol('evt'),events=[];const listener=function(event){events.push(this===object&&event.target===object)};object.addEventListener(symbol,listener);const before=object.hasEventListener(symbol,listener),event={type:symbol};object.dispatchEvent(event);object.removeEventListener(symbol,listener);const after=object.hasEventListener(symbol,listener);
  const key={[Symbol.toPrimitive](hint){events.push('key:'+hint);return 'dynamic'}};object.addEventListener(key,listener);return {before,after,events,cleared:event.target===null};
 }
 if(request.special==='super'){
  const parent=Object.getPrototypeOf(Object3D),events=[];
  class Base{constructor(){events.push(new.target===Object3D);return {inherited:7}}}
  try{Object.setPrototypeOf(Object3D,Base);const value=new Object3D();return {events,inherited:value.inherited,flag:value.isObject3D,prototype:Object.getPrototypeOf(value)===Object.prototype}}finally{Object.setPrototypeOf(Object3D,parent)}
 }
 if(request.special==='module'){const facade={Object3D},saved=Object3D;facade.Object3D=null;return {unchanged:saved===Object3D,newObject:new Object3D().isObject3D,facade:facade.Object3D}}
 throw Error('Unknown special');
}
export function oracle(request){
 let seed=12345;const random=Math.random;Math.random=()=>{seed=(seed*1664525+1013904223)>>>0;return seed/4294967296};
 const events=[],nodes={},resources=[],state={active:false,entered:false,lastEvent:undefined};let oldConsole;
 const norm=(value,seen=new Set())=>{if(value===undefined)return '#undefined';if(typeof value==='number'){if(Number.isNaN(value))return '#NaN';if(!Number.isFinite(value))return value>0?'#Infinity':'#-Infinity';if(Object.is(value,-0))return '#-0'}if(typeof value==='function')return '#function';if(typeof value==='symbol')return '#symbol';if(value===null||typeof value!=='object')return value;for(const [name,node]of Object.entries(nodes))if(value===node)return {ref:name};if(value.isVector3)return {vector:value.toArray().map(v=>norm(v))};if(value.isQuaternion)return {quaternion:value.toArray().map(v=>norm(v))};if(value.isEuler)return {euler:value.toArray().map(v=>norm(v))};if(value.isMatrix4)return {matrix:value.toArray().map(v=>norm(v))};if(value.isObject3D)return snapshot(value);if(seen.has(value))return '#cycle';seen.add(value);const result=Array.isArray(value)?value.map(v=>norm(v,seen)):Object.fromEntries(Object.keys(value).map(k=>[k,norm(value[k],seen)]));seen.delete(value);return result};
 const emit=label=>{if(!state.active)return;events.push(label);if(request.throwAt===label)throw TypeError('fixture');if(request.mutateAt===label)nodes.root.remove(nodes.a);if(request.reenterAt===label&&!state.entered){state.entered=true;nodes.root.add(nodes.b)}};
 const snapshot=value=>({name:value.name,uuid:value.uuid,position:norm(value.position.toArray()),rotation:norm(value.rotation.toArray()),quaternion:norm(value.quaternion.toArray()),scale:norm(value.scale.toArray()),up:norm(value.up.toArray()),matrix:norm(value.matrix.toArray()),matrixWorld:norm(value.matrixWorld.toArray()),pivot:value.pivot===null?null:norm(value.pivot.toArray()),parent:value.parent===null?null:value.parent.name,children:value.children.map(c=>c.name),matrixAutoUpdate:value.matrixAutoUpdate,matrixWorldAutoUpdate:value.matrixWorldAutoUpdate,matrixWorldNeedsUpdate:value.matrixWorldNeedsUpdate,visible:value.visible,layers:value.layers.mask,userData:norm(value.userData)});
 const create=(name,position)=>{const object=new Object3D();nodes[name]=object;object.name=name;if(position)object.position.fromArray(position);return object};
 const resource=(name,uuid=name)=>{const output={metadata:{kind:'resource'},uuid,name};const value={uuid,toJSON(meta){emit('resource:'+name);if(request.resources==='primitive'&&name==='geometry')return 7;if(request.resources==='prototype'&&name==='geometry')meta.geometries.extra=Object.assign(Object.create({inherited:true}),{metadata:{kind:'inherited'},uuid:'extra'});return output}};resources.push({name,output});if(request.resources==='lockedMetadata'&&name==='geometry')Object.defineProperty(output,'metadata',{configurable:false});return value};
 const configureResources=()=>{
  if(!request.resources)return;
  if(request.resources==='scene'){nodes.root.isScene=true;nodes.root.background={isTexture:true,toJSON(meta){emit('background');return {uuid:'background'}}};nodes.root.environment={isTexture:true,toJSON(meta){emit('environment');return {uuid:'environment'}}};return}
  const geometry=resource('geometry'),material=resource('material'),second=request.resources==='sameUUID'?resource('second','material'):material;
  geometry.parameters={shapes:[resource('shape')]};for(const node of [nodes.root,nodes.a,nodes.b]){node.isMesh=true;node.geometry=geometry;node.material=[material,second]}
  nodes.root.animations=[resource('animation')];nodes.root.morphTargetDictionary={alpha:0};nodes.root.morphTargetInfluences=[.25];
  if(request.resources==='instanced'){nodes.root.isInstancedMesh=true;nodes.root.count=4;nodes.root.instanceMatrix=resource('instanceMatrix');nodes.root.instanceColor=resource('instanceColor')}
  if(request.resources==='skinned'){nodes.root.isSkinnedMesh=true;nodes.root.bindMode='attached';nodes.root.bindMatrix=new Matrix4();nodes.root.skeleton=resource('skeleton')}
  if(request.resources==='batched'){const r=nodes.root;r.isBatchedMesh=true;Object.assign(r,{perObjectFrustumCulled:true,sortObjects:false,_drawRanges:[{start:0,count:3}],_reservedRanges:[{start:0,count:8}],_geometryInfo:[{active:true,boundingBox:resource('box'),boundingSphere:resource('sphere')}],_instanceInfo:[{active:true,id:2}],_availableInstanceIds:[1],_availableGeometryIds:[3],_nextIndexStart:3,_nextVertexStart:4,_geometryCount:1,_maxInstanceCount:8,_maxVertexCount:32,_maxIndexCount:64,_geometryInitialized:true,_matricesTexture:resource('matrices'),_indirectTexture:resource('indirect'),_colorsTexture:resource('colors'),boundingSphere:resource('boundsSphere'),boundingBox:resource('boundsBox')})}
 };
 try{
  if(request.special)return special(request);
  oldConsole=setConsoleFunction((level,message)=>emit('log:'+level+':'+message));
  const root=create('root',[1,2,3]),a=create('a',[2,0,0]),b=create('b',[0,1,0]),c=create('c',[-2,1,.5]);root.rotation.set(.2,-.1,.3);root.scale.set(1,1.5,.75);c.rotation.set(.1,.2,-.3);root.add(a);a.add(b);
  if(request.parented){const p=create('p',[5,-3,2]);p.rotation.set(.1,.4,-.2);p.scale.set(2,2,2);p.add(root);p.updateMatrixWorld(true)}else root.updateMatrixWorld(true);c.updateMatrixWorld(true);
  if(request.pivot)root.pivot=new Vector3(.5,-1,2);if(request.hidden)a.visible=false;if(request.camera)root.isCamera=true;if(request.light)root.isLight=true;if(request.autoUpdate===false)root.matrixAutoUpdate=false;if(request.worldAuto===false)root.matrixWorldAutoUpdate=false;
  if(request.userData){root.userData={value:3};c.userData={nested:{value:5},ignored(){},array:[undefined,NaN]}}
  if(request.throwAt==='rotationSync'){const old=root.quaternion._onChangeCallback;root.quaternion._onChange(()=>{emit('rotationSync');old()})}
  for(const [name,node]of Object.entries(nodes))for(const type of ['added','removed','childadded','childremoved','dispose'])node.addEventListener(type,event=>{state.lastEvent=event;emit(name+':'+event.type)});
  if(request.scratchReentry){const old=root.quaternion.setFromRotationMatrix;Object.defineProperty(root.quaternion,'setFromRotationMatrix',{configurable:true,get(){emit('quaternion.method');if(!state.entered){state.entered=true;c.lookAt(-3,2,1)}return old}})}
  configureResources();
  if(request.observed)for(const key of ['matrixAutoUpdate','matrixWorldNeedsUpdate','matrixWorldAutoUpdate','parent','children','position','rotation','quaternion','scale','matrix','matrixWorld','name','type','visible','userData','animations','pivot']){let value=root[key];Object.defineProperty(root,key,{configurable:true,enumerable:true,get(){emit('get:root.'+key);return value},set(v){emit('set:root.'+key);value=v}})}
  const decode=value=>{if(value&&typeof value==='object'){if(value.ref)return nodes[value.ref];if(value.id)return nodes[value.id].id;if(value.vector)return new Vector3(...value.vector);if(value.quaternion)return new Quaternion(...value.quaternion);if(value.euler)return new Euler(...value.euler);if(value.matrix)return new Matrix4().fromArray(value.matrix);if(value.array)return value.array.map(decode);if(value.callback)return node=>emit('visit:'+node.name)}return value};
  const args=request.args.map(decode),out={};state.active=true;
  try{out.result=norm(request.method==='constructor'?new Object3D():root[request.method](...args))}catch(e){out.error=fail(e)}state.active=false;
  out.events=[...events];out.nodes=Object.fromEntries(Object.entries(nodes).map(([k,v])=>[k,snapshot(v)]));if(state.lastEvent)out.lastEvent={type:state.lastEvent.type,target:state.lastEvent.target?state.lastEvent.target.name:null,child:state.lastEvent.child?state.lastEvent.child.name:state.lastEvent.child===null?null:'#undefined'};
  if(resources.length)out.resources=resources.map(({name,output})=>({name,metadata:Object.hasOwn(output,'metadata')}));return out;
 }finally{state.active=false;Math.random=random;if(oldConsole!==undefined)setConsoleFunction(oldConsole);else setConsoleFunction(null)}
}
function differences(a,b,p=''){
 if(Object.is(a,b))return [];
 if(a===null||b===null||typeof a!==typeof b||typeof a!=='object')return [{path:p,expected:a,actual:b}];
 if(Array.isArray(a)!==Array.isArray(b))return [{path:p,kind:'array-type',expected:a,actual:b}];
 const out=[];
 if(Array.isArray(a)&&a.length!==b.length)out.push({path:p+'/length',expected:a.length,actual:b.length});
 for(const key of new Set([...Object.keys(a),...Object.keys(b)])){
  const expectedPresent=Object.hasOwn(a,key),actualPresent=Object.hasOwn(b,key);
  if(expectedPresent!==actualPresent)out.push({path:p+'/'+key,kind:'own-key',expectedPresent,actualPresent});
  else out.push(...differences(a[key],b[key],p+'/'+key));
 }
 return out;
}
function wireNumbers(value){
 if(typeof value==='number'){
  if(Number.isNaN(value))return '#NaN';
  if(value===Infinity)return '#Infinity';
  if(value===-Infinity)return '#-Infinity';
  if(Object.is(value,-0))return '#-0';
  return value;
 }
 if(Array.isArray(value))return value.map(wireNumbers);
 if(value!==null&&typeof value==='object')return Object.fromEntries(Object.entries(value).map(([key,item])=>[key,wireNumbers(item)]));
 return value;
}
if(options.has('--output')){
 const output=path.resolve(options.get('--output'));if(fs.existsSync(output))throw Error('Evidence exists');fs.mkdirSync(output,{recursive:true});fs.writeFileSync(path.join(output,'cases.json'),JSON.stringify(cases,null,2)+'\n');const expected=cases.map(request=>({name:request.name,value:wireNumbers(oracle(request))}));fs.writeFileSync(path.join(output,'expected.json'),JSON.stringify(expected,null,2)+'\n');const summary={node:process.version,cases:cases.length,coveredEntries:new Set(cases.filter(c=>c.method).map(c=>c.method)).size,mode:'oracle-only',nativeExecuted:false};
 if(options.has('--probe')){const binary=path.resolve(options.get('--probe')),hash=()=>createHash('sha256').update(fs.readFileSync(binary)).digest('hex');const before=hash();const result=spawnSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:8*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});summary.nativeExecuted=result.pid!==undefined;let actual;try{actual=JSON.parse(result.stdout)}catch{actual={parseError:true}}const diff=differences(expected,actual);fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2)+'\n');fs.writeFileSync(path.join(output,'differences.json'),JSON.stringify(diff,null,2)+'\n');const life=spawnSync(binary,['--lifetime'],{encoding:'utf8',maxBuffer:1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});fs.writeFileSync(path.join(output,'lifetime.txt'),String(life.stdout??'')+String(life.stderr??''));Object.assign(summary,{mode:'native-differential',binary,beforeSHA256:before,afterSHA256:hash(),exitCode:result.status,failedFields:diff.length,lifetimePassed:life.status===0&&/3 passed/.test(life.stdout)});process.exitCode=result.status!==0||diff.length||!summary.lifetimePassed?1:0}
 fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2)+'\n');console.log(JSON.stringify(summary));
}
