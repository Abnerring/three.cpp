// Integration-only native test launcher. No oracle-only fallback is accepted.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync,execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import assert from 'node:assert/strict';

const options=new Map(),args=process.argv.slice(2);
for(let i=0;i<args.length;++i){
 const key=args[i];
 assert(['--suite','--probe','--upstream','--output-root','--input','--replay','--lifetime-only','--list'].includes(key),'Unknown or positional argument: '+key);
 assert(!options.has(key),'Duplicate argument: '+key);
 if(key==='--lifetime-only'||key==='--list'){options.set(key,true);continue;}
 const value=args[++i];assert(value&&!value.startsWith('--'),'Missing value: '+key);options.set(key,value);
}
const directory=path.dirname(fileURLToPath(import.meta.url));
const suites=JSON.parse(fs.readFileSync(path.join(directory,'object-model-native-suites.json'),'utf8'));
if(options.has('--list')){
 assert.equal(options.size,1,'--list is a standalone plan operation');
 console.log(JSON.stringify({status:'plan-only-native-not-executed',suites},null,2));
 process.exit(0);
}
assert.equal(process.version,'v26.5.0','Pinned Node oracle required');
assert.equal(typeof globalThis.gc,'function','Invoke Node with --expose-gc');
for(const key of ['--suite','--probe','--upstream','--output-root'])assert(options.has(key),'Required argument: '+key);
const suite=suites.find(s=>s.name===options.get('--suite'));assert(suite,'Unknown suite');
const lifetime=options.has('--lifetime-only');
assert(!(options.has('--input')&&options.has('--replay')),'Conflicting replay formats');
assert(!lifetime||(!options.has('--input')&&!options.has('--replay')),'Lifetime mode takes no replay data');
if(options.has('--input'))assert(suite.replay==='input','Suite does not accept --input');
if(options.has('--replay'))assert(suite.replay==='replay','Suite does not accept --replay');
const binary=path.resolve(options.get('--probe')),upstream=path.resolve(options.get('--upstream')),outputRoot=path.resolve(options.get('--output-root'));
assert(fs.statSync(binary).isFile(),'Native --probe must exist');
assert(/^D:[\\/]/i.test(outputRoot),'Evidence must remain on D:');
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
if(suite.fixedUpstream)assert.equal(upstream.replaceAll('\\','/').toLowerCase(),suite.fixedUpstream.toLowerCase(),'This frozen comparator has a fixed upstream path');
const script=path.join(directory,suite.comparator);
if(!lifetime)assert(fs.statSync(script).isFile(),'Apply the comparator from its frozen candidate first');
if(options.has('--input'))assert(fs.statSync(options.get('--input')).isFile(),'Replay input must exist');
if(options.has('--replay'))assert(fs.statSync(path.join(options.get('--replay'),'input.json')).isFile(),'Replay directory must contain original input');
process.env.TEMP='D:/Temp/surui';process.env.TMP='D:/Temp/surui';
fs.mkdirSync(outputRoot,{recursive:true});
const stamp=new Date().toISOString().replaceAll(/[-:.]/g,'');
const output=fs.mkdtempSync(path.join(outputRoot,`${suite.name}-${lifetime?'lifetime':'parity'}-${stamp}-`));
const write=(name,value)=>fs.writeFileSync(path.join(output,name),JSON.stringify(value,null,2)+'\n');
const digest=file=>createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const summary={status:'native-requested',suite:suite.name,mode:lifetime?'native-lifetime':'native-differential',nativeExecuted:false,binary,node:process.version,upstream,output,binaryBeforeSHA256:digest(binary),scope:suite.scope};
const comparatorOutput=path.join(output,'comparison');
let command,childArgs;
if(lifetime){command=binary;childArgs=['--lifetime'];}
else{
 command=process.execPath;
 childArgs=['--expose-gc',script,'--probe',binary,'--output',comparatorOutput];
 if(suite.upstream)childArgs.push('--upstream',upstream);
 if(suite.lifetimeFlag)childArgs.push('--lifetime');
 for(const flag of ['--input','--replay'])if(options.has(flag))childArgs.push(flag,path.resolve(options.get(flag)));
 summary.comparatorBeforeSHA256=digest(script);
}
write('request.json',{command,args:childArgs,env:{TEMP:process.env.TEMP,TMP:process.env.TMP},suite,summary});
let failed=true;
try{
 const result=spawnSync(command,childArgs,{encoding:'utf8',maxBuffer:512*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});
 fs.writeFileSync(path.join(output,'process.stdout.log'),result.stdout??'');
 fs.writeFileSync(path.join(output,'process.stderr.log'),result.stderr??'');
 summary.exitCode=result.status;summary.signal=result.signal;summary.processError=result.error?.message??null;
 summary.childStarted=result.pid!==undefined;
 write('process-exit.json',{status:result.status,signal:result.signal,pid:result.pid??null,error:result.error?.message??null});
 // Report actual execution even when a comparator detects real differences.
 // A Node child PID alone cannot establish native execution.
 if(lifetime) summary.nativeExecuted=result.pid!==undefined;
 else {
  const guardPath=path.join(comparatorOutput,suite.guard.file);
  if(fs.existsSync(guardPath)){
   const guard=JSON.parse(fs.readFileSync(guardPath,'utf8'));
   summary.nativeExecuted=guard[suite.guard.key]===true;
  }
 }
 assert.equal(result.status,0,result.stderr||String(result.error));
 if(lifetime){
  assert.match(result.stdout,suite.lifetimeOutput?new RegExp(suite.lifetimeOutput):/3.*passed/,'Expected genuine native lifetime marker');
  summary.nativeExecuted=result.pid!==undefined;
 }else{
  const guard=JSON.parse(fs.readFileSync(path.join(comparatorOutput,suite.guard.file),'utf8'));
  assert.equal(guard[suite.guard.key],true,'Comparator did not confirm native execution');
  summary.nativeExecuted=true;
 }
 assert(summary.nativeExecuted,'Native execution is required');
 failed=false;
}catch(error){summary.failure=String(error);}
finally{
 summary.binaryAfterSHA256=digest(binary);
 if(summary.binaryBeforeSHA256!==summary.binaryAfterSHA256){failed=true;summary.binaryChanged=true;}
 if(!lifetime){summary.comparatorAfterSHA256=digest(script);if(summary.comparatorAfterSHA256!==summary.comparatorBeforeSHA256){failed=true;summary.comparatorChanged=true;}}
 summary.status=failed?'native-failed':'native-passed';write('integration-summary.json',summary);console.log(JSON.stringify(summary));
}
if(failed)process.exitCode=1;
