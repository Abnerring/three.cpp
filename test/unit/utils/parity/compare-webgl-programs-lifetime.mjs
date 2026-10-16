import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]);
const revision='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),revision);
assert.equal(typeof global.gc,'function','Run Node with --expose-gc');
const {WebGLPrograms}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLPrograms.js')));
const turn=()=>new Promise(resolve=>setImmediate(resolve));
async function settle(){for(let i=0;i<6;i++){await turn();global.gc();}await turn();}
function fixture(name,viaPrototype){
  const inputs=[{},{},{},{logarithmicDepthBuffer:false},{},{}],weak=inputs.map(value=>new WeakRef(value));
  const module=WebGLPrograms(...inputs),method=module[name];
  return {keeper:viaPrototype?Object.create(method.prototype):method,weak};
}
let checks=0;
for(const name of ['getParameters','getProgramCacheKey','getUniforms','acquireProgram','releaseProgram','releaseShaderCache','dispose'])for(const viaPrototype of [false,true]){
  const record=fixture(name,viaPrototype);await settle();
  for(const weak of record.weak){assert.notEqual(weak.deref(),undefined,`${name}: shared invocation environment`);checks++;}
  record.keeper=undefined;await settle();
  for(const weak of record.weak){assert.equal(weak.deref(),undefined,`${name}: released invocation environment`);checks++;}
}
console.log(`WebGLPrograms pinned Node lifetime: 14 scenarios / ${checks} checks passed (${revision}); forced-GC observation, not a deterministic collection deadline.`);
