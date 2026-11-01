import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {spawnSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const args=process.argv.slice(2),options={};for(let i=0;i<args.length;i++){const key=args[i];if(key==='--lifetime')options.lifetime=true;else if(['--output','--probe','--upstream'].includes(key)){assert.ok(args[i+1],key+' requires a value');options[key.slice(2)]=args[++i]}else throw Error('Unknown argument: '+key)}
if(options.upstream)process.env.THREEJS_SOURCE_DIR=path.resolve(options.upstream);
assert.equal(process.env.TEMP?.replaceAll('\\','/').toLowerCase(),'d:/temp/surui');assert.equal(process.env.TMP?.replaceAll('\\','/').toLowerCase(),'d:/temp/surui');
const output=path.resolve(options.output??'D:/Temp/surui/array-some-parity');fs.mkdirSync(output,{recursive:true});
const {oracle,lifetime,upstream,upstreamCommit}=await import('./array-some-oracle.mjs');const {strictDifferences,inspectNumberTransport}=await import('./strict-parity.mjs');
const input={suite:'array-some',lifetime:!!options.lifetime,upstreamCommit,node:process.version};const expected=JSON.parse(JSON.stringify(options.lifetime?await lifetime():oracle()));
fs.writeFileSync(path.join(output,'input.json'),JSON.stringify(input,null,2)+'\n');fs.writeFileSync(path.join(output,'expected.json'),JSON.stringify(expected,null,2)+'\n');
const transport=inspectNumberTransport(expected);assert.equal(transport.rawSpecialNumberCount,0);let nativeExecuted=false,pid=null,differences=[];
if(options.probe){const child=spawnSync(path.resolve(options.probe),options.lifetime?['--lifetime']:[],{encoding:'utf8',maxBuffer:64*1024*1024,env:{...process.env,THREEJS_SOURCE_DIR:upstream}});pid=child.pid;fs.writeFileSync(path.join(output,'stdout.log'),child.stdout??'');fs.writeFileSync(path.join(output,'stderr.log'),child.stderr??'');assert.equal(child.error,undefined);assert.equal(child.status,0);assert.ok(Number.isInteger(pid)&&pid>0,'Real native pid required');nativeExecuted=true;const actual=JSON.parse(child.stdout);fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2)+'\n');differences=strictDifferences(expected,actual);fs.writeFileSync(path.join(output,'differences.json'),JSON.stringify(differences,null,2)+'\n')}
const report={suite:'array-some',node:process.version,v8:process.versions.v8,upstream,upstreamCommit,nativeExecuted,pid,cases:Array.isArray(expected)?expected.length:undefined,rounds:options.lifetime?expected.rounds:undefined,differences:differences.length,oracleOnly:!options.probe,transport};fs.writeFileSync(path.join(output,'result.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report));assert.equal(differences.length,0,'Strict native parity differs; preserved artifacts at '+output);
