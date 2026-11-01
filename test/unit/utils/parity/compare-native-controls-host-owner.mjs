import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {spawnSync} from 'node:child_process';
import {strictDifferences,inspectNumberTransport} from './strict-parity.mjs';
const options={};for(let i=2;i<process.argv.length;i++){const flag=process.argv[i];assert(['--probe','--upstream','--output','--oracle-only'].includes(flag));assert(!Object.hasOwn(options,flag));if(flag==='--oracle-only')options[flag]=true;else options[flag]=process.argv[++i];}
assert.equal(process.version,'v26.5.0');assert(options['--upstream']&&options['--output']);assert(Boolean(options['--probe'])!==Boolean(options['--oracle-only']));
process.env.THREEJS_SOURCE_DIR=path.resolve(options['--upstream']);const {upstream,upstreamCommit}=await import('./threejs-source.mjs');
for(const name of ['TEMP','TMP'])assert.equal(path.resolve(process.env[name]).toLowerCase(),path.resolve('D:/Temp/surui').toLowerCase());
const output=path.resolve(options['--output']);assert(path.parse(output).root[0].toLowerCase()==='d');assert(!fs.existsSync(output));fs.mkdirSync(output,{recursive:true});
const sha=file=>createHash('sha256').update(fs.readFileSync(file)).digest('hex'),write=(name,value)=>fs.writeFileSync(path.join(output,name),JSON.stringify(value,null,2)+'\n');
function run(binary,args,label){const env={...process.env,THREEJS_SOURCE_DIR:upstream};delete env.NODE_V8_COVERAGE;const child=spawnSync(binary,args,{encoding:'utf8',env,input:'',maxBuffer:32*1024*1024});fs.writeFileSync(path.join(output,label+'.stdout.log'),child.stdout??'');fs.writeFileSync(path.join(output,label+'.stderr.log'),child.stderr??'');write(label+'.process.json',{binary,args,pid:child.pid,status:child.status,error:child.error?.message});assert(Number.isInteger(child.pid)&&child.pid>0);assert.equal(child.status,0,label+': '+child.stderr);return JSON.parse(child.stdout);}
const oracle=path.join(import.meta.dirname,'native-controls-host-owner-oracle.mjs');
const expected=run(process.execPath,['--expose-gc',oracle],'node01'),replay=run(process.execPath,['--expose-gc',oracle],'node02');write('expected.json',expected);write('node-replay-differences.json',strictDifferences(expected,replay));assert.equal(strictDifferences(expected,replay).length,0);assert.equal(inspectNumberTransport(expected).rawSpecialNumberCount,0);
const audit={node:process.version,upstream,upstreamCommit,rounds:3,sourceGCExecuted:true,nativeExecuted:false,osInputExecuted:false};
if(options['--probe']){const binary=path.resolve(options['--probe']),before=sha(binary);const actual=run(binary,[],'native01');write('actual.json',actual);const differences=strictDifferences(expected,actual);write('differences.json',differences);assert.equal(differences.length,0);const lifetime=run(binary,['--lifetime'],'native-lifetime01');write('native-lifetime.json',lifetime);write('lifetime-differences.json',strictDifferences(expected,lifetime));assert.equal(strictDifferences(expected,lifetime).length,0);assert.equal(sha(binary),before);audit.nativeExecuted=true;audit.binary={path:binary,sha256:before};}
write('audit.json',audit);console.log(JSON.stringify(audit));
