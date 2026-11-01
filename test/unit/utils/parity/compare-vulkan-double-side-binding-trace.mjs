import assert from 'node:assert/strict';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';

const values=process.argv.slice(2),options={};
for(let i=0;i<values.length;i+=2){assert.ok(values[i].startsWith('--')&&values[i+1]);assert.ok(['--upstream','--output'].includes(values[i]));options[values[i]]=values[i+1];}
assert.ok(options['--upstream']&&options['--output'],'Usage: --upstream PINNED_THREEJS --output FRESH_DIRECTORY');
const here=dirname(fileURLToPath(import.meta.url)),upstream=resolve(options['--upstream']),output=resolve(options['--output']);
mkdirSync(output,{recursive:false});
const write=(name,data)=>writeFileSync(resolve(output,name),data);
const json=(name,value)=>write(name,JSON.stringify(value,null,2)+'\n');
const sha=bytes=>createHash('sha256').update(bytes).digest('hex');
const pin=JSON.parse(readFileSync(resolve(here,'vulkan-double-side-binding-source-sha.json'),'utf8'));
const input=readFileSync(resolve(here,'vulkan-double-side-additional-fixtures.json'));
const expected=readFileSync(resolve(here,'vulkan-double-side-binding-expected.json'));
write('input.json',input);write('expected.json',expected);json('pin.json',pin);
const processes=[],results=[],guardResults=[];let failure=null;
const env={...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'};
function execute(label,binary,args){
 const timeoutMS=label==='upstream-head'?30000:180000;
 const result=spawnSync(binary,args,{timeout:timeoutMS,maxBuffer:64*1024*1024,env});
 processes.push({label,binary,args,pid:result.pid??null,executed:Boolean(result.pid),nativeExecuted:false,status:result.status,signal:result.signal,timeoutMS,error:result.error?String(result.error):null});
 write(`${label}.stdout.${label==='upstream-head'?'log':'json'}`,result.stdout??Buffer.alloc(0));write(`${label}.stderr.log`,result.stderr??Buffer.alloc(0));json('processes.json',processes);return result;
}
try{
 const head=execute('upstream-head','git',['-C',upstream,'rev-parse','HEAD']);
 assert.equal(head.error,undefined);assert.equal(head.status,0);assert.equal(head.stdout.toString('utf8').trim(),pin.fixedUpstreamCommit,'Wrong upstream commit');
 for(const item of pin.sources){const actual=sha(readFileSync(resolve(upstream,item.path)));guardResults.push({path:item.path,expected:item.sha256,actual,unchanged:actual===item.sha256});}
 for(const item of pin.localFiles){const actual=sha(readFileSync(resolve(here,item.path)));guardResults.push({path:item.path,expected:item.sha256,actual,unchanged:actual===item.sha256});}
 json('source-guards.json',guardResults);assert.ok(guardResults.every(x=>x.unchanged),'Pinned source/input/oracle/expected bytes changed');
 for(let i=1;i<=2;++i)results.push(execute(`source-${i}`,process.execPath,[resolve(here,'vulkan-double-side-binding-trace.mjs'),'--upstream',upstream,'--input',resolve(here,'vulkan-double-side-additional-fixtures.json')]));
 const differences=results.flatMap((run,i)=>Buffer.compare(run.stdout??Buffer.alloc(0),expected)===0?[]:[{run:i+1,kind:'raw-output-bytes',expectedSHA256:sha(expected),actualSHA256:sha(run.stdout??Buffer.alloc(0))}]);
 json('differences.json',differences);
 for(const run of results){assert.equal(run.error,undefined);assert.equal(run.status,0);assert.ok(run.pid>0);}
 assert.equal(differences.length,0,'Fresh source binding trace differs from preserved exact raw output');
 assert.equal(Buffer.compare(results[0].stdout,results[1].stdout),0,'Two fresh source runs differ');
 for(const run of results){
  const actual=JSON.parse(run.stdout);assert.equal(actual.WebGLGPUExecuted,false);assert.equal(actual.VulkanGPUExecuted,false);assert.equal(actual.WebGLRendererInstantiated,false);assert.equal(actual.renderOwnTryCount,0);assert.deepEqual(actual.renderBindingCalls.map(x=>x.method),['resetDefaultState']);assert.equal(actual.results.length,4);
  const requests=JSON.parse(input);
  for(const item of actual.results){const request=requests.find(x=>x.id===item.id);assert.ok(request);assert.equal(item.frames.length,2);if(request.throwSecond){const retry=item.frames[1];assert.equal(retry.calls,1);assert.equal(retry.draws[0].positionEnabled,false);assert.equal(retry.draws[0].positionBuffer,null);assert.deepEqual(retry.draws[0].indexData,[0,1,2,2,1,3]);assert.notEqual(item.programSelections[0].key,item.programSelections[1].key);assert.equal(item.programSelections[1].key,item.programSelections[2].key);}}
 }
}catch(error){failure=String(error.stack??error);}
const summary={passed:failure===null,sourceExecuted:results.length===2&&results.every(x=>x.pid>0),sourcePIDs:results.map(x=>x.pid??null),sourceExitCodes:results.map(x=>x.status),sourceGPUExecuted:false,nativeExecuted:false,cases:4,framesEach:2,freshRuns:2,pinGuardsUnchanged:guardResults.length===pin.sources.length+pin.localFiles.length&&guardResults.every(x=>x.unchanged),rawExpectedByteIdentical:results.length===2&&results.every(x=>Buffer.compare(x.stdout??Buffer.alloc(0),expected)===0),freshRunsByteIdentical:results.length===2&&Buffer.compare(results[0].stdout??Buffer.alloc(0),results[1].stdout??Buffer.alloc(0))===0,failure};
json('summary.json',summary);
if(failure){process.stderr.write(failure+'\n');process.exitCode=1;}else process.stdout.write(JSON.stringify(summary)+'\n');
