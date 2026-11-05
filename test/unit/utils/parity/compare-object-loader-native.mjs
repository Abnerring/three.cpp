import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {createOracle,fixtureBundle,withSchema} from './object-loader-oracle.mjs';

const [upstream,probe,requestedOutput]=process.argv.slice(2);
assert(upstream&&probe,'Usage: compare-object-loader-native.mjs <pinned> <probe|--oracle-only> [fresh-output-directory]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url),'utf8'));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const sourceOnly=['--oracle-only','--source-only'].includes(probe);
const output=requestedOutput??path.join('D:/Temp/surui','object-loader-parity-'+Date.now());
assert(!fs.existsSync(output),'Output must be a fresh directory: '+output);fs.mkdirSync(output,{recursive:true});
const files=['object-loader-fixtures.json','object-loader-oracle.mjs','compare-object-loader-native.mjs'];
const hash=buffer=>createHash('sha256').update(buffer).digest('hex');
const hashes=Object.fromEntries(files.map(name=>[name,hash(fs.readFileSync(new URL('./'+name,import.meta.url)))]));
const requests=fixtureBundle.cases.map(withSchema),oracle=await createOracle(upstream),expected=[];
try{for(const request of requests)expected.push(await oracle.run(request));}finally{oracle.dispose();}
// The serialization protocol itself is JSON. Compare the serialized data, so
// expected objects with a class/null prototype do not weaken deep comparison.
const expectedJSON=JSON.parse(JSON.stringify(expected));
for(let i=0;i<requests.length;++i)assert.deepStrictEqual(expectedJSON[i].error,requests[i].expectedError,'Unexpected source outcome: '+requests[i].case);
fs.writeFileSync(path.join(output,'requests.json'),JSON.stringify(requests,null,2));
fs.writeFileSync(path.join(output,'expected.json'),JSON.stringify(expectedJSON,null,2));
let actual,failed=[];
if(!sourceOnly){
    const executable=path.resolve(probe);assert(fs.statSync(executable).isFile());
    hashes.probe=hash(fs.readFileSync(executable));
    const stdout=execFileSync(executable,{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:64*1024*1024,timeout:300000,
        env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});
    actual=JSON.parse(stdout.trim().split(/\r?\n/).at(-1));
    fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));
    assert(Array.isArray(actual));assert.equal(actual.length,expectedJSON.length);
    for(let i=0;i<requests.length;++i){try{assert.deepStrictEqual(actual[i],expectedJSON[i]);}catch(error){failed.push({case:requests[i].case,index:i,difference:error.message});}}
}
for(const name of files)assert.equal(hash(fs.readFileSync(new URL('./'+name,import.meta.url))),hashes[name],'Fixture source changed during comparison: '+name);
const errors=requests.flatMap((request,i)=>expectedJSON[i].error?[{case:request.case,error:expectedJSON[i].error}]:[]);
const summary={upstreamCommit:lock.commit,sourceOnly,cases:requests.length,successfulResults:requests.length-errors.length,errors,hashes,failed};
fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));
process.stdout.write(JSON.stringify({output,cases:requests.length,successfulResults:summary.successfulResults,expectedErrors:errors.length,failures:failed.length})+'\n');
assert.equal(failed.length,0,failed.slice(0,3).map(row=>row.case+'\n'+row.difference).join('\n'));
