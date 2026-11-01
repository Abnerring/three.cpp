import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {readFileSync,writeFileSync,mkdirSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {fileURLToPath} from 'node:url';
import {strictDifferences} from './strict-parity.mjs';

const values=process.argv.slice(2),options={};
for(let i=0;i<values.length;i+=2){assert.ok(values[i].startsWith('--')&&values[i+1]);options[values[i]]=values[i+1];}
assert.ok(options['--probe']&&options['--upstream']&&options['--output'],'Usage: --probe EXE --upstream PATH --output FRESH [--input JSON]');
const here=dirname(fileURLToPath(import.meta.url)),output=resolve(options['--output']);mkdirSync(output);
const fixture=resolve(options['--input']??resolve(here,'vulkan-double-side-fixtures.json')),input=readFileSync(fixture);writeFileSync(resolve(output,'input.json'),input);
const processes=[];
function execute(label,binary,args,stdin){
  const result=spawnSync(binary,args,{input:stdin,encoding:'utf8',timeout:180000,maxBuffer:64*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});
  const receipt={label,binary,args,pid:result.pid??null,executed:!!result.pid,nativeExecuted:label==='native'&&!!result.pid,status:result.status,signal:result.signal,error:result.error?String(result.error):null};
  processes.push(receipt);writeFileSync(resolve(output,`${label}.stdout.json`),result.stdout??'');writeFileSync(resolve(output,`${label}.stderr.log`),result.stderr??'');writeFileSync(resolve(output,'processes.json'),JSON.stringify(processes,null,2));return result;
}
// Always attempt both source and actual native GPU entrypoints. No skip or
// parser-error recovery is allowed; raw outputs/process results precede asserts.
const source=execute('source',process.execPath,[resolve(here,'vulkan-double-side-oracle.mjs'),'--upstream',resolve(options['--upstream']),'--input',fixture,'--audit','true']);
const native=execute('native',resolve(options['--probe']),[],input);
let expected,actual,parseError=null;
try{expected=JSON.parse(source.stdout);actual=JSON.parse(native.stdout);}catch(error){parseError=String(error);}
const differences=parseError?[]:strictDifferences(expected,actual?.cases);
writeFileSync(resolve(output,'differences.json'),JSON.stringify(differences,null,2));
const summary={sourceExecuted:!!source.pid,nativeExecuted:!!native.pid,sourceExitCode:source.status,nativeExitCode:native.status,parseError,cases:expected?.length??null,differenceCount:differences.length,actualGPUExecuted:actual?.actualGPUExecuted??false,deviceName:actual?.deviceName??null,nativeAssertions:actual?.assertions??null,sourceScope:'exact fixed renderObject + real WebGLAttributes command protocol only; source GPU not executed',passed:source.status===0&&native.status===0&&!parseError&&differences.length===0&&actual?.actualGPUExecuted===true};
writeFileSync(resolve(output,'summary.json'),JSON.stringify(summary,null,2));
assert.equal(source.status,0,'Source process failed; raw evidence preserved');assert.equal(native.status,0,'Native GPU process failed; raw evidence preserved');assert.equal(parseError,null,'Malformed JSON is a failure');assert.equal(actual.actualGPUExecuted,true);assert.ok(actual.deviceName.length>0);assert.ok(actual.assertions>0);assert.equal(actual.validationMessages,0);assert.equal(differences.length,0,'Strict source/native differences are preserved');
process.stdout.write(JSON.stringify(summary)+'\n');
