import assert from 'node:assert/strict';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
const [upstream]=process.argv.slice(2);process.env.TEMP=process.env.TMP='D:/Temp/surui';
assert(upstream,'Explicit fixed upstream required');assert.equal(process.version,'v26.5.0');assert.equal(typeof globalThis.gc,'function','Use --expose-gc');
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--','src'],{encoding:'utf8'}).trim(),'');
const load=file=>import(pathToFileURL(path.join(upstream,'src',file+'.js')).href);
const {default:RenderList}=await load('renderers/common/RenderList');const {default:RenderLists}=await load('renderers/common/RenderLists');
const {Scene}=await load('scenes/Scene');const {PerspectiveCamera}=await load('cameras/PerspectiveCamera');
const {BoxGeometry}=await load('geometries/BoxGeometry');const {Material}=await load('materials/Material');const {Mesh}=await load('objects/Mesh');
function lightingProtocol(){const node={setLights(){}};return {enabled:true,getNode(){return node;}};}
function createHeld(phase){
 const scene=new Scene(),camera=new PerspectiveCamera(),geometry=new BoxGeometry(),material=new Material(),lighting=lightingProtocol();
 const target=new Mesh(geometry,material),weak=new WeakRef(target);
 if(phase==='dispose'){
  const manager=new RenderLists(),list=manager.get(scene,camera,lighting);list.push(target,geometry,material,0,1,null,null);
  return {scene,camera,lighting,weak,manager};
 }
 const list=new RenderList(lighting,scene,camera);
 if(phase==='finish'){const survivor=new Mesh(geometry,material);list.push(survivor,geometry,material,0,1,null,null);}
 list.push(target,geometry,material,0,2,null,null);
 return {scene,camera,lighting,weak,list};
}
async function gcCheckpoint(weak){
 let alive=false;
 for(let turn=0;turn<4;turn++){await new Promise(resolve=>setImmediate(resolve));globalThis.gc();alive=weak.deref()!==undefined;}
 return alive;
}
function release(holder,phase){
 if(phase==='dispose'){holder.manager.dispose();return;}
 if(phase==='clear'){holder.list.clear();return;}
 const survivor=holder.list.renderItems[0];holder.list.begin();holder.list.push(survivor.object,survivor.geometry,survivor.material,0,3,null,null);holder.list.finish();
}
const rows=[];
for(let round=0;round<3;round++)for(const phase of ['finish','clear','dispose']){
 const holder=createHeld(phase),heldAlive=await gcCheckpoint(holder.weak);
 release(holder,phase);const releasedGone=!(await gcCheckpoint(holder.weak));
 const holderAlive=phase==='dispose'?holder.manager instanceof RenderLists:holder.list instanceof RenderList;
 const retainedScene=phase==='dispose'?holder.scene instanceof Scene:holder.list.scene===holder.scene;
 const retainedCamera=phase==='dispose'?holder.camera instanceof PerspectiveCamera:holder.list.camera===holder.camera;
 const postState=phase==='dispose'?{activeSize:holder.manager._activeLists.size,frameId:holder.manager._frameId}:{renderItemsIndex:holder.list.renderItemsIndex,poolSize:holder.list.renderItems.length,targetItemCleared:holder.list.renderItems[phase==='finish'?1:0].object===null};
 rows.push({round,phase,heldAlive,releasedGone,holderAlive,retainedScene,retainedCamera,sceneId:holder.scene.id,cameraId:holder.camera.id,postState});
 assert(heldAlive&&releasedGone&&holderAlive&&retainedScene&&retainedCamera,'Actual lifetime protocol did not complete');
}
process.stdout.write(JSON.stringify(rows));
