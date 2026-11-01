import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
const [upstream,fixtureFile]=process.argv.slice(2);
process.env.TEMP=process.env.TMP='D:/Temp/surui';
assert(upstream&&fixtureFile,'Usage: node --expose-gc render-lists-accessor-oracle.mjs <fixed-upstream> <fixtures>');
assert.equal(process.version,'v26.5.0');
assert.equal(typeof globalThis.gc,'function','Use --expose-gc');
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--','src'],{encoding:'utf8'}).trim(),'');
const load=file=>import(pathToFileURL(path.join(upstream,'src',file+'.js')).href);
const {default:RenderList}=await load('renderers/common/RenderList');
const {default:RenderLists}=await load('renderers/common/RenderLists');
const {Scene}=await load('scenes/Scene');
const {PerspectiveCamera}=await load('cameras/PerspectiveCamera');
const {BoxGeometry}=await load('geometries/BoxGeometry');
const {Material}=await load('materials/Material');
const {Mesh}=await load('objects/Mesh');
const {Light}=await load('lights/Light');
const {default:Node}=await load('nodes/core/Node');
const {DoubleSide,FrontSide}=await load('constants');
function makeWorld(count=3){
 const scene=new Scene(),camera=new PerspectiveCamera(),geometry=new BoxGeometry();
 const materials=[new Material(),new Material(),new Material()];
 const objects=Array.from({length:count},()=>new Mesh(geometry,materials[0]));
 return {scene,camera,geometry,materials,objects};
}
function worldSnapshot(world){return {sceneId:world.scene.id,cameraId:world.camera.id,geometryId:world.geometry.id,materialIds:world.materials.map(x=>x.id),objectIds:world.objects.map(x=>x.id)};}
// Explicit Lighting/LightsNode protocol adapter; these are not implementations of either upstream module.
function makeLighting(events,label='lighting',enabled=true,onGet=null,onSet=null){
 const nodes=new WeakMap();
 const lighting={enabled,getNode(scene){
  events.push(['getNode',label,scene.id]);
  if(onGet)onGet(scene);
  let node=nodes.get(scene);
  if(!node){node={setLights(values){events.push(['setLights',label,values.map(x=>x.id),Object.isFrozen(values)]);if(onSet)onSet(values);}};nodes.set(scene,node);}
  return node;
 }};
 return lighting;
}
function itemSnapshot(item){return {keys:Object.keys(item),id:item.id,objectId:item.object===null?null:item.object.id,geometryId:item.geometry===null?null:item.geometry.id,materialId:item.material===null?null:item.material.id,groupOrder:item.groupOrder,renderOrder:item.renderOrder,z:item.z,group:item.group,clippingContext:item.clippingContext};}
function snapshot(list){return {keys:Object.keys(list),renderItemsIndex:list.renderItemsIndex,renderItems:list.renderItems.map(itemSnapshot),opaque:list.opaque.map(x=>x.id),transparentDoublePass:list.transparentDoublePass.map(x=>x.id),transparent:list.transparent.map(x=>x.id),bundles:list.bundles.map(x=>x.tag),lights:list.lightsArray.map(x=>x.id),sceneId:list.scene.id,cameraId:list.camera.id,frameId:list.frameId,occlusionQueryCount:list.occlusionQueryCount,lastOcclusionObjectId:list._lastOcclusionObject===null?null:list._lastOcclusionObject.id};}
function push(list,world,index,materialIndex=0,groupOrder=0,z=1,group=null,clippingContext=null,method='push'){return list[method](world.objects[index],world.geometry,world.materials[materialIndex],groupOrder,z,group,clippingContext);}
function functionMetadata(fn){return {name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn)};}
function metadata(C){return {constructor:functionMetadata(C),ownKeys:Reflect.ownKeys(C),prototypeOwnKeys:Reflect.ownKeys(C.prototype),entries:Object.getOwnPropertyNames(C.prototype).map(key=>{const d=Object.getOwnPropertyDescriptor(C.prototype,key);return {key,writable:d.writable,enumerable:d.enumerable,configurable:d.configurable,value:functionMetadata(d.value)};})};}
function activeSnapshot(manager){return Array.from(manager._activeLists,list=>({sceneId:list.scene.id,cameraId:list.camera.id,frameId:list.frameId}));}
function wireReplacer(key,value){if(typeof value==='number'){if(Object.is(value,-0))return {$number:'-0'};if(Number.isNaN(value))return {$number:'NaN'};if(value===Infinity)return {$number:'Infinity'};if(value===-Infinity)return {$number:'-Infinity'};}return value;}

function runAccessor(request){
 const {scenario,variant=0}=request,events=[],world=makeWorld(),{scene,camera,geometry,materials,objects}=world;
 const lighting=makeLighting(events),material=materials[0];let result;
 if(scenario.startsWith('material-')){
  const list=new RenderList(lighting,scene,camera),reason={tag:'material-abrupt'};objects[0].occlusionTest=true;let node=null,valueCalls=0,caught=null;
  const getter=(key,value)=>Object.defineProperty(material,key,{configurable:true,enumerable:true,get(){events.push(['get',key,this===material]);return value;}});
  const amount={valueOf(){valueCalls++;events.push(['valueOf',valueCalls,this===amount]);globalThis.gc();events.push(['gc','valueOf']);if(scenario==='material-throw')throw reason;return scenario==='material-chain'&&valueCalls===1?0:1;}};
  if(scenario==='material-node'||scenario==='material-chain'){
   node=new Node();Object.defineProperty(node,'isNode',{configurable:true,enumerable:true,get(){events.push(['get:isNode',this===node]);return true;}});
  }
  getter('transparent',scenario==='material-short');getter('transmission',scenario==='material-node'?0:amount);
  getter('transmissionNode',scenario==='material-node'?node:null);getter('backdropNode',scenario==='material-chain'?node:null);getter('side',DoubleSide);getter('forceSinglePass',false);
  try{push(list,world,0);}catch(error){caught=error;events.push(['caught',error===reason]);}
  result={caughtOriginal:caught===reason,nodeId:node===null?null:node.id,valueCalls,list:snapshot(list)};
 }else if(scenario==='next-reentry'){
  const list=new RenderList(lighting,scene,camera),target=objects[0];let entered=false,recording=true,nested=null;
  const proxy=new Proxy(target,{get(actual,key,receiver){if(key==='id'&&recording){events.push(['get:id',receiver===proxy]);if(!entered){entered=true;nested=list.getNextRenderItem(objects[2],geometry,materials[2],7,9,null,{tag:'inner'});events.push(['inner-created',nested===list.renderItems[0]]);globalThis.gc();events.push(['gc','id-getter']);}}return Reflect.get(actual,key,receiver);}});
  const outer=list.getNextRenderItem(proxy,geometry,materials[0],1,2,{start:0,count:3},{tag:'outer'});recording=false;
  result={targetIdConfigurable:Object.getOwnPropertyDescriptor(target,'id').configurable,proxyIsMesh:proxy instanceof Mesh,outerSameSlot1:outer===list.renderItems[1],nestedSameSlot0:nested===list.renderItems[0],outerObjectSameProxy:outer.object===proxy,list:snapshot(list)};
 }else if(scenario==='reuse-setter'){
  const list=new RenderList(lighting,scene,camera),reason={tag:'reuse-setter-abrupt'};
  const item=list.getNextRenderItem(objects[0],geometry,materials[0],1,2,null,null);list.begin();const previous=Object.getOwnPropertyDescriptor(item,'object');let backing=previous.value,entered=false,nested=null,caught=null;
  Object.defineProperty(item,'object',{configurable:true,enumerable:true,get(){return backing;},set(value){events.push(['set:object',value.id,this===item]);if(variant===1){globalThis.gc();events.push(['gc','reuse-setter']);throw reason;}backing=value;if(!entered){entered=true;nested=list.getNextRenderItem(objects[2],geometry,materials[2],7,9,null,{tag:'inner'});events.push(['inner-same-item',nested===item]);}globalThis.gc();events.push(['gc','reuse-setter']);}});
  try{list.getNextRenderItem(objects[1],geometry,materials[1],3,4,{start:3,count:3},{tag:'outer'});}catch(error){caught=error;events.push(['caught',error===reason]);}finally{Object.defineProperty(item,'object',{...previous,value:backing});}
  result={caughtOriginal:caught===reason,nestedSameItem:nested===item,objectSameInitial:item.object===objects[0],objectSameNested:item.object===objects[2],list:snapshot(list)};
 }else if(scenario==='reset-setter'){
  const phases=[];objects[0].occlusionTest=objects[1].occlusionTest=true;
  for(const phase of ['finish','clear']){
   events.push(['phase',phase]);const list=new RenderList(makeLighting(events,phase),scene,camera),reason={tag:'reset-'+phase};push(list,world,0);push(list,world,1,1);
   if(phase==='finish'){list.begin();push(list,world,0);}
   const item=list.renderItems[phase==='finish'?1:0],previous=Object.getOwnPropertyDescriptor(item,'object');let backing=previous.value,caught=null;
   Object.defineProperty(item,'object',{configurable:true,enumerable:true,get(){return backing;},set(value){events.push(['set:object',phase,value===null,this===item]);list.pushBundle({tag:'reset-reentry'});globalThis.gc();events.push(['gc','reset-setter']);throw reason;}});
   try{list[phase]();}catch(error){caught=error;events.push(['caught',phase,error===reason]);}finally{Object.defineProperty(item,'object',{...previous,value:backing});}
   phases.push({phase,caughtOriginal:caught===reason,idAlreadyNull:item.id===null,objectRetained:item.object===previous.value,geometryRetained:item.geometry===geometry,materialRetained:item.material===materials[phase==='finish'?1:0],list:snapshot(list)});
  }
  result={phases};
 }else throw new Error('Unknown accessor scenario '+scenario);
 return {scenario,variant,world:worldSnapshot(world),events,result};
}
const requests=JSON.parse(fs.readFileSync(fixtureFile,'utf8'));assert(Array.isArray(requests));const rows=[];
for(const request of requests){globalThis.gc();rows.push(runAccessor(request));}
process.stdout.write(JSON.stringify(rows,wireReplacer));
