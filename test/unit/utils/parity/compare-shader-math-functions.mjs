import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {fileURLToPath} from 'node:url';
import {spawnSync,execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fixtures} from './shader-math-functions-fixtures.mjs';
import {strictDifferences} from './strict-parity.mjs';
const here=path.dirname(fileURLToPath(import.meta.url)),options={};
for(let i=2;i<process.argv.length;i++){
 const key=process.argv[i];if(key==='--node-only'){assert.equal(options.nodeOnly,undefined);options.nodeOnly=true;continue;}
 assert(['--upstream','--probe','--output','--input'].includes(key)&&process.argv[i+1]);assert.equal(options[key.slice(2)],undefined);options[key.slice(2)]=process.argv[++i];
}
assert.equal(Number(process.versions.node.split('.')[0]),26);assert(options.upstream&&options.output);assert(options.nodeOnly?!options.probe:options.probe);assert.match(path.resolve(options.output),/^D:[\\/]/i);assert(!fs.existsSync(options.output));
assert.equal(execFileSync('git',['-C',options.upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
assert.equal(execFileSync('git',['-C',options.upstream,'status','--porcelain','--','src'],{encoding:'utf8'}).trim(),'');
fs.mkdirSync(options.output,{recursive:true});const output=path.resolve(options.output),input=JSON.stringify(options.input?JSON.parse(fs.readFileSync(options.input,'utf8')):fixtures()),processes=[];fs.writeFileSync(path.join(output,'input.json'),input);
const hash=file=>createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const summary={cases:JSON.parse(input).length,status:'requested',nativeExecuted:false,nativeSuccessfulProcesses:0,sourceExecuted:false,strict:true,absoluteIdentities:true,nodeNumbersRenumbered:false,realGPUExecuted:false,nodeVersion:process.version,v8Version:process.versions.v8,upstream:'11738a9d55e502fb484cefffcea3a66dc44e9fe9'};
const update=()=>{summary.nativeExecuted=processes.some(p=>p.nativeExecuted);summary.nativeSuccessfulProcesses=processes.filter(p=>p.nativeExecuted&&p.status===0&&!p.error).length;summary.sourceExecuted=processes.some(p=>p.label==='node'&&p.executed);fs.writeFileSync(path.join(output,'processes.json'),JSON.stringify(processes,null,2));fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));};
function run(label,binary,args,stdin){
 const child=spawnSync(binary,args,{...(stdin===undefined?{}:{input:stdin}),encoding:'utf8',timeout:180000,maxBuffer:256*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui',THREEJS_SOURCE_DIR:path.resolve(options.upstream),NODE_V8_COVERAGE:path.join(output,label+'-coverage')}});
 const executed=Number.isInteger(child.pid)&&child.pid>0,native=!!options.probe&&label.startsWith('native')&&path.resolve(binary)===path.resolve(options.probe);
 processes.push({label,binary,args,pid:child.pid??null,executed,status:child.status,signal:child.signal,error:child.error?String(child.error):null,nativeExecuted:native&&executed});fs.writeFileSync(path.join(output,label+'.stdout.json'),child.stdout??'');fs.writeFileSync(path.join(output,label+'.stderr.log'),child.stderr??'');update();
 assert.equal(child.error,undefined);assert.equal(child.status,0,label+': '+child.stderr);assert(executed,'Positive integer child PID required');return JSON.parse(child.stdout);
}
try{
 const expected=run('node',process.execPath,['--expose-gc',path.join(here,'shader-math-functions-oracle.mjs'),options.upstream,'cases'],input);
 const lifetime=run('node-lifetime',process.execPath,['--expose-gc',path.join(here,'shader-math-functions-oracle.mjs'),options.upstream,'lifetime']);
 summary.sourceLifetimeObservations=lifetime.observations.length;
 if(!options.nodeOnly){
  const before=hash(options.probe),actual=run('native',options.probe,[],input),actualLifetime=run('native-lifetime',options.probe,['--lifetime']);
  const heaps=run('native-heap-lifetime',options.probe,['--heap-lifetime']);
  const differences=[...strictDifferences(expected,actual,'cases'),...strictDifferences(lifetime,actualLifetime,'lifetime'),...strictDifferences({heaps:3,retired:3},heaps,'heap-lifetime')];
  fs.writeFileSync(path.join(output,'differences.json'),JSON.stringify(differences,null,2));const after=hash(options.probe);fs.writeFileSync(path.join(output,'binary.json'),JSON.stringify({path:options.probe,beforeSHA256:before,afterSHA256:after},null,2));assert.equal(after,before);summary.differences=differences.length;assert.equal(differences.length,0,'Strict differences recorded without numeric, identity, key-order or error-message normalization');
 }
 summary.status=options.nodeOnly?'source-only-passed':'native-passed';console.log(JSON.stringify({cases:summary.cases,nativeExecuted:summary.nativeExecuted,output}));
}catch(error){summary.status='failed';summary.failure={name:error.name,message:error.message};throw error;}
finally{update();}
