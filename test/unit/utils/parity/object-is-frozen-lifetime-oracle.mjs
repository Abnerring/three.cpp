import assert from 'node:assert/strict';
assert.equal(process.version,'v26.5.0');assert.equal(typeof gc,'function');
function create(phase){const child={value:1};let target=phase==='accessor'?{get child(){return child;}}:{child};if(phase==='proxy')target=new Proxy(target,{});Object.freeze(target);return {target,weak:new WeakRef(child),frozen:Object.isFrozen(target)};}
async function alive(weak){let result=false;for(let turn=0;turn<4;++turn){await new Promise(resolve=>setImmediate(resolve));gc();result=weak.deref()!==undefined;}return result;}
const rows=[];
for(let round=0;round<3;++round)for(const phase of ['data','accessor','proxy']){const holder=create(phase),holderWeak=new WeakRef(holder);const heldAlive=await alive(holder.weak);holder.target=null;const releasedGone=!(await alive(holder.weak)),holderAlive=holderWeak.deref()===holder;assert(holder.frozen&&heldAlive&&releasedGone&&holderAlive,'Actual frozen reference lifetime failed');rows.push({round,phase,frozen:holder.frozen,heldAlive,releasedGone,holderAlive});}
process.stdout.write(JSON.stringify(rows));
