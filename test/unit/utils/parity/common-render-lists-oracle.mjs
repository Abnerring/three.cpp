import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
const [upstream,fixtureFile]=process.argv.slice(2);
process.env.TEMP=process.env.TMP='D:/Temp/surui';
assert(upstream&&fixtureFile,'Usage: node --expose-gc render-lists-oracle.mjs <fixed-upstream> <fixtures>');
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

function run(request){
 const {scenario,variant=0}=request,events=[];
 if(scenario==='metadata')return {scenario,variant,result:{RenderList:metadata(RenderList),RenderLists:metadata(RenderLists)}};
 const world=makeWorld(scenario==='sort-default'?6:3),lighting=makeLighting(events);
 const {scene,camera,geometry,materials,objects}=world;
 let result;
 if(scenario==='defaults'){
  const list=new RenderList(lighting,scene,camera),initial=snapshot(list),beginSame=list.begin()===list;
  const light=new Light();list.pushLight(light);list.pushBundle({tag:'bundle'});push(list,world,0);
  const dirty=snapshot(list);list.begin();result={initial,beginSame,dirty,reset:snapshot(list),lightId:light.id};
 }else if(scenario==='reuse'){
  const list=new RenderList(lighting,scene,camera);
  const first=list.getNextRenderItem(objects[0],geometry,materials[0],1,4,{start:0,count:3},{tag:'clip0'});
  const second=list.getNextRenderItem(objects[1],geometry,materials[1],2,5,null,null);
  const before=snapshot(list);list.begin();objects[2].renderOrder=9;
  const reused=list.getNextRenderItem(objects[2],geometry,materials[2],3,6,{start:3,count:3},{tag:'clip1'});
  const afterReuse=snapshot(list);list.finish();
  result={before,reusedSame:first===reused,secondSlotSame:second===list.renderItems[1],afterReuse,afterFinish:snapshot(list)};
 }else if(scenario==='occlusion'){
  const list=new RenderList(lighting,scene,camera);objects[0].occlusionTest=objects[1].occlusionTest=true;
  push(list,world,0);push(list,world,0);push(list,world,1);push(list,world,0);
  const nonConsecutive=snapshot(list);list.begin();push(list,world,0);const afterBegin=snapshot(list);
  list.finish();const afterFinish=snapshot(list);list.begin();push(list,world,0);
  result={nonConsecutive,afterBegin,afterFinish,nextFrame:snapshot(list)};
 }else if(scenario==='classify'){
  const type=variant%8,method=variant<8?'push':'unshift',material=materials[0];
  Object.assign(material,{transparent:false,transmission:0,transmissionNode:null,backdropNode:null,side:DoubleSide,forceSinglePass:false});
  let node=null;
  if(type===1)material.transparent=true;
  if(type===2)material.transmission=1;
  if(type===3){node=new Node();material.transmissionNode=node;}
  if(type===4){node=new Node();material.backdropNode=node;}
  if(type===5){material.transmission=1;material.forceSinglePass=true;}
  if(type===6){material.transmission=1;material.side=FrontSide;}
  if(type===7)material.transparent='truthy-not-true';
  camera._reversedDepth=true;objects[0].occlusionTest=objects[1].occlusionTest=true;objects[1].renderOrder=10;
  listForClassify();
  function listForClassify(){
   const list=new RenderList(lighting,scene,camera);
   push(list,world,0,0,0,type===0?0:4,null,null,method);
   push(list,world,0,0,0,5,null,null,method);
   push(list,world,1,0,1,6,null,null,method);
   push(list,world,0,0,0,7,null,null,method);
   result={method,nodeId:node===null?null:node.id,list:snapshot(list),doublePassShared:list.transparentDoublePass.every(item=>list.transparent.includes(item))};
   return list;
  }
 }else if(scenario==='sort-default'){
  const list=new RenderList(lighting,scene,camera);
  materials[1].transparent=true;Object.assign(materials[2],{transmission:1,side:DoubleSide,forceSinglePass:false});
  const groups=[1,0,0,0,0,0],orders=[0,1,0,0,0,0],depths=[2,2,5,1,1,1];objects.forEach((object,i)=>object.renderOrder=orders[i]);
  for(let m=0;m<3;m++)for(const i of [5,4,3,2,1,0])push(list,world,i,m,groups[i],depths[i]);
  const before=snapshot(list),comparators=[],previous=Object.getOwnPropertyDescriptor(Array.prototype,'sort');
  Object.defineProperty(Array.prototype,'sort',{...previous,value:function(compare){comparators.push(functionMetadata(compare));return Reflect.apply(previous.value,this,[compare]);}});
  try{list.sort();}finally{Object.defineProperty(Array.prototype,'sort',previous);}
  result={before,after:snapshot(list),comparators,sortedOpaque:list.opaque.map(itemSnapshot),sortedDoublePass:list.transparentDoublePass.map(itemSnapshot),sortedTransparent:list.transparent.map(itemSnapshot)};
 }else if(scenario==='sort-custom'){
  const list=new RenderList(lighting,scene,camera),reason={tag:'sort-getter-abrupt'};
  push(list,world,1,0,0,variant===4?0:2);push(list,world,0,0,0,variant===4?-0:1);
  const item=list.renderItems[0],oldZ=Object.getOwnPropertyDescriptor(item,'z');let caught=null;
  if(variant===1||variant===2)Object.defineProperty(item,'z',{configurable:true,enumerable:true,get(){events.push(['get:z',this===item]);list.pushBundle({tag:'getter-reentry'});globalThis.gc();events.push(['gc','sort-getter']);if(variant===2)throw reason;return oldZ.value;}});
  try{list.sort(function(a,b){events.push(['compare',a.id,b.id,this===undefined]);if(variant===0){list.pushBundle({tag:'compare-reentry'});globalThis.gc();events.push(['gc','comparator']);return b.id-a.id;}if(variant===3)return NaN;if(variant===4)return -0;return a.z-b.z;});}catch(error){caught=error;}finally{Object.defineProperty(item,'z',oldZ);}
  result={caughtOriginal:caught===reason,caughtName:caught===null?null:caught.name??null,caughtTag:caught===null?null:caught.tag??null,list:snapshot(list)};
 }else if(scenario==='lifecycle'){
  let list;const reason={tag:'setLights-abrupt'};
  const host=makeLighting(events,'lifecycle',variant!==1,null,values=>{events.push(['lightsSame',values===list.lightsArray]);globalThis.gc();events.push(['gc','setLights']);if(variant===2)push(list,world,2,2,3,8);if(variant===3)throw reason;});
  list=new RenderList(host,scene,camera);objects[0].occlusionTest=true;
  push(list,world,0);push(list,world,1,1);list.begin();push(list,world,0);
  const light=new Light();list.pushLight(light);list.pushBundle({tag:'life-bundle'});const before=snapshot(list);let caught=null;
  try{list.finish();}catch(error){caught=error;}
  const afterFinish=snapshot(list);list.clear();
  result={before,caughtOriginal:caught===reason,afterFinish,afterClear:snapshot(list),lightId:light.id};
 }else if(scenario==='clear-break'){
  const list=new RenderList(lighting,scene,camera);push(list,world,0);push(list,world,1,1);list.renderItems[0].id=null;list.clear();
  result={list:snapshot(list),firstObjectRetained:list.renderItems[0].object===objects[0],laterObjectRetained:list.renderItems[1].object===objects[1]};
 }else if(scenario==='manager-identity'){
  const manager=new RenderLists(),scene2=new Scene(),camera2=new PerspectiveCamera(),otherLighting=makeLighting(events,'other');
  const initial={keys:Object.keys(manager),frameId:manager._frameId,active:activeSnapshot(manager)};manager.update(0);
  const a=manager.get(scene,camera,lighting),same=manager.get(scene,camera,lighting),b=manager.get(scene,camera2,lighting),c=manager.get(scene2,camera,lighting),d=manager.get(scene,camera,otherLighting);
  push(a,world,0);push(b,world,1);push(c,world,2);push(d,world,0);
  const identities={same:same===a,cameraDistinct:b!==a,sceneDistinct:c!==a,lightingDistinct:d!==a,brands:[a,b,c,d].every(x=>x instanceof RenderList)};
  const active0=activeSnapshot(manager);manager.update(10);const active10=activeSnapshot(manager),held10=[a,b,c,d].map(snapshot);manager.update(11);
  const active11=activeSnapshot(manager),held11=[a,b,c,d].map(snapshot),revived=manager.get(scene,camera,lighting),oldMap=manager.lists;
  manager.dispose();const disposed={frameId:manager._frameId,active:activeSnapshot(manager),newMap:manager.lists!==oldMap};const fresh=manager.get(scene,camera,lighting);
  result={initial,scene2Id:scene2.id,camera2Id:camera2.id,identities,active0,active10,held10,active11,held11,revivedSame:revived===a,disposed,freshDistinct:fresh!==a,fresh:snapshot(fresh)};
 }else if(scenario==='manager-iteration'){
  const manager=new RenderLists(),scene2=new Scene(),scene3=new Scene();manager.update(0);
  const a=manager.get(scene,camera,lighting),b=manager.get(scene2,camera,lighting);push(a,world,0);push(b,world,1);
  const realClear=RenderList.prototype.clear,reason={tag:'clear-abrupt'};let added=null,caught=null;
  a.clear=function(){events.push(['clear','a',this===a]);realClear.call(this);globalThis.gc();events.push(['gc','clear']);if(variant===0){added=manager.get(scene3,camera,lighting);events.push(['added',manager._activeLists.has(added)]);events.push(['touchB',manager.get(scene2,camera,lighting)===b]);}if(variant===1){manager.dispose();events.push(['dispose-reentry',manager._frameId]);}if(variant===2)throw reason;};
  b.clear=function(){events.push(['clear','b',this===b]);return realClear.call(this);};
  try{manager.update(11);}catch(error){caught=error;}
  const beforeSameFrame=activeSnapshot(manager);manager.update(11);
  result={scene2Id:scene2.id,scene3Id:scene3.id,caughtOriginal:caught===reason,frameId:manager._frameId,active:activeSnapshot(manager),beforeSameFrame,hasA:manager._activeLists.has(a),hasB:manager._activeLists.has(b),hasAdded:added!==null&&manager._activeLists.has(added),a:snapshot(a),b:snapshot(b),added:added===null?null:snapshot(added)};
 }else if(scenario==='manager-chain-reentry'){
  const manager=new RenderLists(),innerScene=new Scene(),innerCamera=new PerspectiveCamera(),innerLighting=makeLighting(events,'inner');manager.update(0);
  let first=true,inner=null,caught=null;
  const outerLighting=makeLighting(events,'outer',true,()=>{if(first){first=false;inner=manager.get(innerScene,innerCamera,innerLighting);events.push(['nested-get',inner instanceof RenderList]);globalThis.gc();events.push(['gc','constructor-reentry']);}});
  try{manager.get(scene,camera,outerLighting);}catch(error){caught=error;events.push(['caught',error.name,error.message]);}
  const afterFailure=activeSnapshot(manager),innerSame=manager.get(innerScene,innerCamera,innerLighting)===inner,retry=manager.get(scene,camera,outerLighting);
  result={innerSceneId:innerScene.id,innerCameraId:innerCamera.id,caughtTypeError:caught instanceof TypeError,caughtName:caught===null?null:caught.name,caughtMessage:caught===null?null:caught.message,afterFailure,innerSame,retry:snapshot(retry),active:activeSnapshot(manager)};
 }else throw new Error('Unknown scenario '+scenario);
 return {scenario,variant,world:worldSnapshot(world),events,result};
}
const requests=JSON.parse(fs.readFileSync(fixtureFile,'utf8'));
assert(Array.isArray(requests));const results=[];
for(const request of requests){globalThis.gc();results.push(run(request));}
process.stdout.write(JSON.stringify(results,wireReplacer));
