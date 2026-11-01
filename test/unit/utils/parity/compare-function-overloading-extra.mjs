import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync,execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {strictDifferences} from './strict-parity.mjs';
const options={};
for(let i=2;i<process.argv.length;++i){const key=process.argv[i];if(key==='--node-only'){assert.equal(options.nodeOnly,undefined);options.nodeOnly=true;continue;}assert(['--upstream','--probe','--output'].includes(key)&&process.argv[i+1]);assert.equal(options[key],undefined);options[key]=process.argv[++i];}
assert.equal(process.version,'v26.5.0');assert(options['--upstream']&&options['--output']);assert(options.nodeOnly?!options['--probe']:options['--probe']);
const upstream=path.resolve(options['--upstream']),output=path.resolve(options['--output']),probe=options['--probe']&&path.resolve(options['--probe']),here=path.dirname(fileURLToPath(import.meta.url));
assert.equal(upstream.toLowerCase(),path.resolve('D:/Temp/surui/threejs-pinned-11738a9-20261101').toLowerCase(),'Mapped source URI is the actual pinned file, not a basename alias');
assert.match(output,/^D:[\\/]/i);assert(!fs.existsSync(output));assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--','src'],{encoding:'utf8'}).trim(),'');
fs.mkdirSync(output,{recursive:true});const checks=[],processes=[],sha=p=>createHash('sha256').update(fs.readFileSync(p)).digest('hex'),binaryBefore=probe?sha(probe):null;
const summary={status:'requested',fixedUpstreamCommit:'11738a9d55e502fb484cefffcea3a66dc44e9fe9',nodeOracleExecuted:false,nativeExecuted:false,original170Metadata8Lifetime15Preserved:true,strict:true,identityNormalization:false,receiverBrandAndMethodNameClaimed:false,fullCallSiteProtocolClaimed:false,checks};
function update(){summary.nodeOracleExecuted=processes.some(p=>p.label.startsWith('source')&&p.executed);summary.nativeExecuted=processes.some(p=>p.nativeExecuted)||Boolean(summary.originalNativeExecuted);fs.writeFileSync(output+'/processes.json',JSON.stringify(processes,null,2)+'\n');fs.writeFileSync(output+'/summary.json',JSON.stringify(summary,null,2)+'\n');}
function launch(label,binary,args,input){
 const child=spawnSync(binary,args,{...(input===undefined?{}:{input}),encoding:'utf8',timeout:180000,maxBuffer:128*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}),executed=Number.isInteger(child.pid)&&child.pid>0;
 processes.push({label,binary,args,pid:child.pid??null,executed,nativeExecuted:!!probe&&binary===probe&&executed,status:child.status,signal:child.signal,error:child.error?String(child.error):null});
 fs.writeFileSync(output+'/'+label+'.stdout.log',child.stdout??'');fs.writeFileSync(output+'/'+label+'.stderr.log',child.stderr??'');update();assert(executed);assert.equal(child.error,undefined);assert.equal(child.status,0,label+': '+child.stderr);return child.stdout;
}
function difference(name,expected,actual){const differences=strictDifferences(expected,actual,name);fs.writeFileSync(output+'/'+name+'-differences.json',JSON.stringify(differences,null,2)+'\n');assert.equal(differences.length,0,'All original fields/type/ordered keys/numbers remain strict');}
// Independent checks continue after a semantic failure, including native GC.
for(const mode of ['trace','construct','heap','original']){
 const row={name:mode,status:'requested'};checks.push(row);
 try{
  if(mode==='original'){
   launch('original-comparator',process.execPath,[path.join(here,'compare-function-overloading-node.mjs'),'--upstream',upstream,'--output',path.join(output,'original'),...(probe?['--probe',probe]:['--node-only'])]);
   const original=JSON.parse(fs.readFileSync(output+'/original/summary.json','utf8'));summary.originalNativeExecuted=original.nativeExecuted;assert.equal(original.status,probe?'native-passed':'node-only-passed');assert.deepEqual(original.checks.map(x=>[x.name,x.status]),[['main170','passed'],['metadata8entries','passed'],['lifetime15','passed']]);
  }else if(mode==='heap'){
   if(!probe){row.status='not-executed-native-only';continue;}
   const actual=JSON.parse(launch('native-heap',probe,['--heap-lifetime']));
   difference('heap',{heaps:3,retired:3,moduleExpired:3,constructorExpired:3,closureExpired:3,nodeExpired:3},actual);
  }else{
   const input=fs.readFileSync(path.join(here,`function-overloading-extra-${mode}-fixtures.json`),'utf8');fs.writeFileSync(output+'/'+mode+'-input.json',input);row.cases=JSON.parse(input).length;
   const args=['--expose-gc',path.join(here,'function-overloading-extra-oracle.mjs'),upstream,mode],expectedRaw=launch('source-'+mode,process.execPath,args,input);fs.writeFileSync(output+'/'+mode+'-expected.json',expectedRaw);
   const replayRaw=launch('source-'+mode+'-replay',process.execPath,args,input);assert.equal(replayRaw,expectedRaw,'Two independent actual-source processes must emit the same bytes');
   if(probe){const actualRaw=launch('native-'+mode,probe,['--extra-'+mode],input);fs.writeFileSync(output+'/'+mode+'-actual.json',actualRaw);difference(mode,JSON.parse(expectedRaw),JSON.parse(actualRaw));}
  }
  row.status='passed';
 }catch(error){row.status='failed';row.error={name:error.name,message:error.message};}
 finally{update();}
}
if(probe){const after=sha(probe);fs.writeFileSync(output+'/binary.json',JSON.stringify({path:probe,beforeSHA256:binaryBefore,afterSHA256:after},null,2)+'\n');assert.equal(after,binaryBefore);}
summary.status=checks.every(x=>x.status==='passed'||options.nodeOnly&&x.status==='not-executed-native-only')?(probe?'native-passed':'node-only-passed'):'failed';update();console.log(JSON.stringify(summary));assert.notEqual(summary.status,'failed','Failures are retained; independent checks completed');
