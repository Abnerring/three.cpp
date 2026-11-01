import fs from 'node:fs';import assert from 'node:assert/strict';
assert.equal(process.version,'v26.5.0');assert.equal(typeof gc,'function');
const tick=()=>new Promise(resolve=>setImmediate(resolve));
async function collected(refs,expected){for(let i=0;i<60;i++){await tick();gc();await tick();if(refs.every(w=>(w.deref()===undefined)===expected))return;}assert.fail('Unexpected async frame retention state');}
function deferred(){let resolve;const promise=new Promise(a=>resolve=a);return{promise,resolve};}
async function keep(input,marker,rejected=false){await input;if(rejected)throw marker;return marker;}
const observations=[];
for(let round=0;round<3;round++){
 let input=deferred(),result,weak;{let marker={};marker.self=marker;weak=new WeakRef(marker);result=keep(input.promise,marker);marker=null;}
 await collected([weak],false);observations.push({round,phase:'await-held',alive:true});input.resolve();input=null;await tick();await collected([weak],false);observations.push({round,phase:'await-result-held',alive:true});result=null;await collected([weak],true);observations.push({round,phase:'await-dropped',gone:true});
 const cycle=(()=>{const input=deferred(),marker={};marker.self=marker;const result=keep(input.promise,marker);return[new WeakRef(marker),new WeakRef(input.promise),new WeakRef(result)];})();await collected(cycle,true);observations.push({round,phase:'unreachable-cycle-dropped',gone:true});
 let held,thenResult,thenWeak;{let marker={};marker.self=marker;thenWeak=new WeakRef(marker);thenResult=keep({then(resolve){held=resolve;}},marker);marker=null;}await tick();assert.equal(typeof held,'function');await collected([thenWeak],false);observations.push({round,phase:'thenable-held',alive:true});held=thenResult=null;await collected([thenWeak],true);observations.push({round,phase:'thenable-dropped',gone:true});
 let rejectInput=deferred(),rejection,rejectWeak;{let marker={};marker.self=marker;rejectWeak=new WeakRef(marker);rejection=keep(rejectInput.promise,marker,true);rejection.catch(()=>{});marker=null;}rejectInput.resolve();rejectInput=null;await tick();await collected([rejectWeak],false);observations.push({round,phase:'rejection-held',alive:true});rejection=null;await collected([rejectWeak],true);observations.push({round,phase:'rejection-dropped',gone:true});
}
const result={rounds:3,observations};if(process.argv[2]){assert(!fs.existsSync(process.argv[2]));fs.writeFileSync(process.argv[2],JSON.stringify(result,null,2)+'\n');}else process.stdout.write(JSON.stringify(result));
