import assert from 'node:assert/strict';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
const [upstream]=process.argv.slice(2);process.env.TEMP=process.env.TMP='D:/Temp/surui';
assert(upstream,'Explicit fixed upstream required');assert.equal(process.version,'v26.5.0');
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--','src'],{encoding:'utf8'}).trim(),'');
const sourceURL=file=>pathToFileURL(path.join(upstream,'src',file+'.js')).href;
await import(sourceURL('constants'));await import(sourceURL('renderers/common/ChainMap'));
const previous=Object.getOwnPropertyDescriptor(Object,'freeze'),reason={tag:'module-freeze-abrupt'},caughtOriginal=[];let freezeCalls=0;
Object.defineProperty(Object,'freeze',{...previous,value:function(value){if(Array.isArray(value)&&value.length===0){freezeCalls++;throw reason;}return Reflect.apply(previous.value,this,[value]);}});
try{
 for(const file of ['renderers/common/RenderLists','renderers/common/RenderLists','renderers/common/RenderList']){
  try{await import(sourceURL(file));caughtOriginal.push(false);}catch(error){caughtOriginal.push(error===reason);}
 }
}finally{Object.defineProperty(Object,'freeze',previous);}
const restored=Object.getOwnPropertyDescriptor(Object,'freeze');
const result={attempts:3,freezeCalls,caughtOriginal,originalFreezeRestored:restored.value===previous.value&&restored.writable===previous.writable&&restored.enumerable===previous.enumerable&&restored.configurable===previous.configurable};
process.stdout.write(JSON.stringify(result));
assert.equal(result.freezeCalls,1);assert.deepEqual(caughtOriginal,[true,true,true]);assert.equal(result.originalFreezeRestored,true);
