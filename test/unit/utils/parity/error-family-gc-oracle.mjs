import fs from 'node:fs';
import assert from 'node:assert/strict';
import {setImmediate as turn} from 'node:timers/promises';
assert.equal(process.version,'v26.5.0');assert.equal(typeof global.gc,'function');assert.equal(process.argv.length,3);assert(!fs.existsSync(process.argv[2]));
Error.stackTraceLimit=0;
const names=['Error','EvalError','RangeError','ReferenceError','SyntaxError','TypeError','URIError'];
async function collect(weak,alive){for(let i=0;i<20;i++){await turn();global.gc();await turn();if((weak.deref()!==undefined)===alive)return;}assert.fail('Unexpected retention state');}
const result=[];
for(let round=0;round<3;round++){
 const entries=[];
 for(const name of names){const ctor=globalThis[name];let error=new ctor();let cause={};const weakCause=new WeakRef(cause);error.cause=cause;cause=null;await collect(weakCause,true);delete error.cause;await collect(weakCause,false);
  let cycle=new ctor();const weakCycle=new WeakRef(cycle);cycle.cause=cycle;cycle=null;await collect(weakCycle,false);
  const original=Object.getOwnPropertyDescriptor(Error,'prepareStackTrace');let value=new ctor();const weakFormatter=new WeakRef(value);Error.prepareStackTrace=((captured)=>()=>captured)(value);value=null;await collect(weakFormatter,true);Object.defineProperty(Error,'prepareStackTrace',original);await collect(weakFormatter,false);
  entries.push({name,causeRetained:true,causeReleased:true,cycleReleased:true,formatterRetained:true,formatterReleased:true});error=null;
 }
 result.push({round,entries});
}
fs.writeFileSync(process.argv[2],JSON.stringify({node:process.version,rounds:result},null,2)+'\n');console.log('ErrorFamily Node GC: 3 passed');
