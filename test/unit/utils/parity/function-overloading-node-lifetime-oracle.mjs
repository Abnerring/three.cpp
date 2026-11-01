import assert from 'node:assert/strict';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
assert.equal(typeof global.gc,'function');
const {default:Overloading,overloadingFn}=await import(pathToFileURL(path.join(process.argv[2],'src/nodes/utils/FunctionOverloadingNode.js')));
const tick=()=>new Promise(resolve=>setImmediate(resolve));
async function gc(){await tick();global.gc();await tick();}
async function gone(refs){for(let i=0;i<80;i++){await gc();if(refs.every(r=>r.deref()===undefined))return true;}return false;}
const observations=[];
for(let round=0;round<3;round++){
 let candidates=[],factory=overloadingFn(candidates),refs=[new WeakRef(candidates)];candidates=null;
 await gc();assert(refs.every(r=>r.deref()!==undefined));observations.push({round,phase:'closure-holds-candidates',alive:true});
 factory=null;assert(await gone(refs));observations.push({round,phase:'closure-release',gone:true});
 let fn=function(){},list=[fn],param={getNodeType(){return 'float';}},node=new Overloading(list,param);fn.shaderNode={layout:{inputs:[{type:'float'}],type:'float'}};node.getCandidateFn({});
 refs=[new WeakRef(fn),new WeakRef(list),new WeakRef(param)];fn=list=param=null;await gc();assert(refs.every(r=>r.deref()!==undefined));observations.push({round,phase:'node-holds-arguments-and-cache',alive:true});
 node.functionNodes=[];node.parametersNodes=[];await gc();assert.notEqual(refs[0].deref(),undefined);assert(await gone(refs.slice(1)));observations.push({round,phase:'cached-function-only',functionAlive:true,argumentsGone:true});
 node=null;assert(await gone(refs));observations.push({round,phase:'node-release',gone:true});
}
process.stdout.write(JSON.stringify({rounds:3,observations}));
