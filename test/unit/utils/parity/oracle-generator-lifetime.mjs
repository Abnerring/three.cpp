import fs from 'node:fs';
import path from 'node:path';
const output=process.argv[2];if(!output||process.argv.length!==3||!globalThis.gc)throw new Error('Usage: node --expose-gc oracle-generator-lifetime.mjs new-output.json');if(fs.existsSync(output))throw new Error('Evidence exists: '+output);
const tick=()=>new Promise(resolve=>setImmediate(resolve));
const results=[];
for(let trial=0;trial<3;++trial){
 let state=(()=>{const capture={marker:trial},f=function*(){yield capture;return capture.marker;},g=f();capture.generator=g;capture.function=f;g.self=g;return{generator:g,weakCapture:new WeakRef(capture),weakFunction:new WeakRef(f),weakGenerator:new WeakRef(g)};})();
 await tick();globalThis.gc();await tick();
 ((current)=>{const live=current.weakCapture.deref();if(!live||current.generator.next().value!==live)throw new Error('Live suspended generator lost lexical capture identity');})(state);
 // The generator and all explicit roots are dropped. WeakRef observations are
 // only made after leaving the job that had strong temporary handles.
 const weak=[state.weakCapture,state.weakFunction,state.weakGenerator];state=null;
 await tick();
 for(let i=0;i<12;++i){globalThis.gc();await tick();}
 results.push({trial,capturedWhileSuspended:true,cycleCollected:weak.map(w=>w.deref()===undefined)});
}
fs.mkdirSync(path.dirname(path.resolve(output)),{recursive:true});fs.writeFileSync(output,JSON.stringify({node:process.version,v8:process.versions.v8,scope:'Actual engine observations; collection timing is not an ECMAScript guarantee',results},null,2)+'\n');console.log(JSON.stringify({trials:results.length,output}));
