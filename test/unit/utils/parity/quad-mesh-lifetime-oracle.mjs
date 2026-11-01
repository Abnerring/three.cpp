import assert from 'node:assert/strict';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
assert.equal(typeof global.gc,'function');
const upstream=process.argv[2],load=p=>import(pathToFileURL(path.join(upstream,'src',p+'.js')));
const {default:QuadMesh}=await load('renderers/common/QuadMesh'),utils=await load('utils');
utils.setConsoleFunction(()=>{});
const tick=()=>new Promise(resolve=>setImmediate(resolve));
async function gone(refs){for(let i=0;i<80;i++){await tick();global.gc();await tick();if(refs.every(ref=>ref.deref()===undefined))return true;}return false;}
const output=[];
for(let round=0;round<3;round++){
 let q=new QuadMesh({vertexNode:null}),refs=[new WeakRef(q)],geometry=new WeakRef(q.geometry),camera=new WeakRef(q.camera);q=null;
 assert.equal(await gone(refs),true);assert.notEqual(geometry.deref(),undefined);assert.notEqual(camera.deref(),undefined);
 output.push({round,phase:'module-resources-held-instance-released',instanceGone:true,geometryAlive:true,cameraAlive:true});geometry=camera=null;
 for(const rejected of [false,true]){
  let deferred=Promise.withResolvers(),renderer={init(){return deferred.promise;},render(){}},quad=new QuadMesh({vertexNode:null}),promise=quad.renderAsync(renderer);
  if(rejected)promise.catch(()=>{});
  refs=[quad,renderer].map(v=>new WeakRef(v));quad=renderer=null;
  await tick();global.gc();assert(refs.every(r=>r.deref()!==undefined));
  output.push({round,phase:rejected?'await-rejection-held':'await-fulfillment-held',alive:true});
  if(rejected)deferred.reject('stop');else deferred.resolve(7);
  if(rejected)await assert.rejects(promise,reason=>reason==='stop');else assert.equal(await promise,undefined);
  assert.equal(await gone(refs),true);
  output.push({round,phase:rejected?'await-rejection-released':'await-fulfillment-released',gone:true});promise=deferred=null;
 }
}
utils.setConsoleFunction(null);process.stdout.write(JSON.stringify({rounds:3,observations:output}));
