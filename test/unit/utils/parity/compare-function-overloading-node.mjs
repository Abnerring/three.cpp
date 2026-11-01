import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync,execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {fixtures} from './function-overloading-node-fixtures.mjs';
import {strictDifferences} from './strict-parity.mjs';
const options={};
for(let i=2;i<process.argv.length;i++){const key=process.argv[i];if(key==='--node-only'){assert.equal(options.nodeOnly,undefined);options.nodeOnly=true;continue;}assert(['--upstream','--probe','--output','--input'].includes(key)&&process.argv[i+1]);assert.equal(options[key],undefined);options[key]=process.argv[++i];}
assert.equal(process.version,'v26.5.0');assert(options['--upstream']&&options['--output']);assert(options.nodeOnly?!options['--probe']:options['--probe']);
const upstream=path.resolve(options['--upstream']),output=path.resolve(options['--output']),probe=options['--probe']&&path.resolve(options['--probe']),here=path.dirname(fileURLToPath(import.meta.url));
assert.match(output,/^D:[\\/]/i);assert(!fs.existsSync(output));assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--','src'],{encoding:'utf8'}).trim(),'');
fs.mkdirSync(output,{recursive:true});
const requests=options['--input']?JSON.parse(fs.readFileSync(options['--input'],'utf8')):fixtures(),input=JSON.stringify(requests),processes=[],checks=[];
fs.writeFileSync(output+'/input.json',input);
const sha=p=>createHash('sha256').update(fs.readFileSync(p)).digest('hex');
const binaryBefore=probe?sha(probe):null;
const summary={status:'requested',cases:requests.length,fixedUpstreamCommit:'11738a9d55e502fb484cefffcea3a66dc44e9fe9',nodeOracleExecuted:false,nativeExecuted:false,nativeSuccessfulProcesses:0,strict:true,absoluteIDs:true,identityNormalization:false,productionASTRuntime:false,fullNodesTSLBarrelEvaluationComplete:false,checks};
const update=()=>{summary.nodeOracleExecuted=processes.some(x=>x.label.startsWith('source')&&x.executed);summary.nativeExecuted=processes.some(x=>x.nativeExecuted);summary.nativeSuccessfulProcesses=processes.filter(x=>x.nativeExecuted&&x.status===0&&!x.error).length;fs.writeFileSync(output+'/processes.json',JSON.stringify(processes,null,2)+'\n');fs.writeFileSync(output+'/summary.json',JSON.stringify(summary,null,2)+'\n');};
function run(label,binary,args,stdin){const child=spawnSync(binary,args,{...(stdin===undefined?{}:{input:stdin}),encoding:'utf8',timeout:180000,maxBuffer:128*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui',NODE_V8_COVERAGE:output+'/'+label+'-coverage'}});const executed=Number.isInteger(child.pid)&&child.pid>0;processes.push({label,binary,args,pid:child.pid??null,executed,nativeExecuted:!!probe&&binary===probe&&executed,status:child.status,signal:child.signal,error:child.error?String(child.error):null});fs.writeFileSync(output+'/'+label+'.stdout.json',child.stdout??'');fs.writeFileSync(output+'/'+label+'.stderr.log',child.stderr??'');update();assert.equal(child.error,undefined);assert.equal(child.status,0,label+': '+child.stderr);assert(executed);return JSON.parse(child.stdout);}
const suites=[
 {name:'main170',source:['--expose-gc','function-overloading-node-oracle.mjs',upstream,'cases'],native:[],input},
 {name:'metadata8entries',source:['function-overloading-node-oracle.mjs',upstream,'metadata'],native:['--metadata']},
 {name:'lifetime15',source:['--expose-gc','function-overloading-node-lifetime-oracle.mjs',upstream],native:['--lifetime']}
];
for(const suite of suites){const row={name:suite.name,status:'requested'};checks.push(row);try{const args=suite.source.map(x=>x.endsWith('.mjs')?path.join(here,x):x),expected=run('source-'+suite.name,process.execPath,args,suite.input);if(!options.nodeOnly){const actual=run('native-'+suite.name,probe,suite.native,suite.input),differences=strictDifferences(expected,actual,suite.name);row.differences=differences.length;fs.writeFileSync(output+'/'+suite.name+'-differences.json',JSON.stringify(differences,null,2)+'\n');assert.equal(differences.length,0,'Strict differences retained without normalization');}row.status='passed';}catch(error){row.status='failed';row.error={name:error.name,message:error.message};}finally{update();}}
if(probe){const after=sha(probe);fs.writeFileSync(output+'/binary.json',JSON.stringify({path:probe,beforeSHA256:binaryBefore,afterSHA256:after},null,2)+'\n');assert.equal(after,binaryBefore);}
summary.status=checks.every(x=>x.status==='passed')?(options.nodeOnly?'node-only-passed':'native-passed'):'failed';update();console.log(JSON.stringify(summary));assert(checks.every(x=>x.status==='passed'),'All original FunctionOverloadingNode checks must pass; failures retained');
