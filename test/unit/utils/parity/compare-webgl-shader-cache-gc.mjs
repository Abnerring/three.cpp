import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {writeFileSync} from 'node:fs';
const methods=['update','remove','getVertexShaderStage','getFragmentShaderStage','dispose','_getShaderCacheForMaterial','_getShaderStage'];
if(process.argv[2]==='--child'){
 const upstream=process.argv[3],mode=process.argv[4];const {WebGLShaderCache}=await import(pathToFileURL(`${upstream}/src/renderers/webgl/WebGLShaderCache.js`));
 let retained,initialId;
 const refs=(()=>{
  const cache=new WebGLShaderCache(),material={},stage=cache._getShaderStage('code');initialId=stage.id;
  cache.update(material,stage,stage);const set=cache._getShaderCacheForMaterial(material);
  const refs={cache:new WeakRef(cache),material:new WeakRef(material),stage:new WeakRef(stage),set:new WeakRef(set),shaderMap:new WeakRef(cache.shaderCache),materialMap:new WeakRef(cache.materialCache)};
  if(['retain','remove','dispose'].includes(mode))retained=cache;
  if(mode==='remove')cache.remove(material);if(mode==='dispose')cache.dispose();if(mode==='cycle')stage.code=cache;
  if(mode.startsWith('detached:')){
   const constructor=WebGLShaderCache,stageConstructor=stage.constructor,prototype=constructor.prototype,stagePrototype=stageConstructor.prototype,payload={},stagePayload={};
   prototype.payload=payload;stagePrototype.payload=stagePayload;
   Object.assign(refs,{cacheConstructor:new WeakRef(constructor),stageConstructor:new WeakRef(stageConstructor),payload:new WeakRef(payload),stagePayload:new WeakRef(stagePayload)});
   retained=prototype[mode.slice(9)];for(const name of ['constructor',...methods])delete prototype[name];delete stagePrototype.constructor;
  }
  return refs;
 })();
 for(let i=0;i<6;i++){await new Promise(r=>setImmediate(r));global.gc();}
 const result={mode};for(const [name,weak]of Object.entries(refs))result[name]=!!weak.deref();
 if(mode.startsWith('detached:')){result.detached=typeof retained==='function';const Stage=refs.stageConstructor.deref();result.nextId=Stage?new Stage('later').id-initialId:'expired'}
 console.log(JSON.stringify(result));
}else{
 const upstream=process.argv[2],probe=process.argv[3],prefix=process.argv[4];assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
 const node=[],native=[];
 for(const mode of ['retain','remove','dispose','cycle',...methods.map(name=>'detached:'+name)]){
  const expected=JSON.parse(execFileSync(process.execPath,['--expose-gc',fileURLToPath(import.meta.url),'--child',upstream,mode],{encoding:'utf8'}));
  const actual=JSON.parse(execFileSync(probe,[mode],{encoding:'utf8'}));assert.deepEqual(actual,expected,mode);node.push(expected);native.push(actual);
 }
 if(prefix){writeFileSync(prefix+'-node.json',JSON.stringify(node,null,2));writeFileSync(prefix+'-native.json',JSON.stringify(native,null,2))}
 console.log(`WebGLShaderCache lifetime: ${node.length} actual Node snapshots passed (${process.version}); GC timing is not an ECMAScript guarantee.`);
}
