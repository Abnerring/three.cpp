import {upstreamModule} from './threejs-source.mjs';
const {seededRandom}=await upstreamModule('src/math/MathUtils.js');
import fs from 'node:fs';
if(typeof globalThis.gc!=='function')throw Error('Use --expose-gc');
const output=process.argv[2];if(!output||process.argv.length!==3||fs.existsSync(output))throw Error('One new output file required');
for(const k of ['TEMP','TMP'])if(process.env[k]?.replaceAll('\\','/').toLowerCase()!=='d:/temp/surui')throw Error('TEMP/TMP');
const tick=()=>new Promise(resolve=>setImmediate(resolve));const collect=async()=>{for(let i=0;i<5;i++){await tick();globalThis.gc();}};
const rounds=[];
for(let i=0;i<3;i++){
 let weak;(()=>{const seed={[Symbol.toPrimitive](){throw 77;}};weak=new WeakRef(seed);try{seededRandom(seed);}catch(e){if(e!==77)throw e;}})();
 await collect();const retained=weak.deref()!==undefined;seededRandom(7);await collect();const released=weak.deref()===undefined;
 if(!retained||!released)throw Error('Unexpected source seed reachability');rounds.push({retainedAfterAbruptConversion:retained,releasedAfterReplacement:released});
}
const audit={node:process.version,upstream:'11738a9d55e502fb484cefffcea3a66dc44e9fe9',rounds,nativeExecuted:false};fs.writeFileSync(output,JSON.stringify(audit,null,2)+'\n');console.log(JSON.stringify(audit));
