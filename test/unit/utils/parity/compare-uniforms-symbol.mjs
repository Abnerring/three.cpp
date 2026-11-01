import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fixtures} from './uniforms-symbol-fixtures.mjs';
import {oracle,upstream,upstreamCommit} from './uniforms-symbol-oracle.mjs';
import {strictDifferences,inspectNumberTransport} from './strict-parity.mjs';
const options={},argv=process.argv.slice(2);
for(let i=0;i<argv.length;i++){const key=argv[i];assert(!Object.hasOwn(options,key),'Duplicate '+key);if(key==='--oracle-only')options[key]=true;else{assert(['--output','--input','--probe'].includes(key),'Unknown '+key);assert(argv[i+1]&&!argv[i+1].startsWith('--'),'Missing '+key);options[key]=argv[++i];}}
assert.equal(process.version,'v26.5.0');
for(const name of ['TEMP','TMP'])assert.equal(process.env[name]?.replaceAll('\\','/').toLowerCase(),'d:/temp/surui');
assert(options['--output']);assert(Boolean(options['--oracle-only'])!==Boolean(options['--probe']),'Use exactly one --oracle-only / --probe');
const output=path.resolve(options['--output']);assert(!fs.existsSync(output),'Output must be fresh');fs.mkdirSync(output,{recursive:true});
const write=(file,value)=>fs.writeFileSync(path.join(output,file),JSON.stringify(value,null,2)+'\n');
const hash=file=>createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const input=options['--input']?JSON.parse(fs.readFileSync(options['--input'],'utf8')):fixtures;assert(Array.isArray(input));
const expected=oracle(input);write('input.json',input);write('expected.json',expected);
const transport=inspectNumberTransport(expected);write('transport.json',transport);assert.equal(transport.rawSpecialNumberCount,0);
const wire=strictDifferences(expected,JSON.parse(JSON.stringify(expected)));write('wire-differences.json',wire);assert.equal(wire.length,0,'Transport must preserve the full normalized observations');
const audit={node:process.version,v8:process.versions.v8,arguments:argv,TEMP:process.env.TEMP,TMP:process.env.TMP,upstream,upstreamCommit,cases:input.length,nativeExecuted:false,processes:[],scope:'Actual pinned UniformsUtils clone/merge with Symbol prototype enumeration, primitive/wrapper getter receivers, repeated reads, abrupt completion, nested clone reentry and private boxing under mutable aliases.'};
const differences=[];
if(options['--probe']){
 const binary=path.resolve(options['--probe']);assert(path.isAbsolute(options['--probe']));const before=hash(binary);
 const execute=(label,args)=>{const result=spawnSync(binary,args,{input:JSON.stringify(input),encoding:'utf8',maxBuffer:64*1024*1024,env:process.env});const executed=Number.isInteger(result.pid)&&result.pid>0;audit.nativeExecuted ||= executed;audit.processes.push({label,args,pid:result.pid??null,executed,status:result.status,signal:result.signal,error:result.error?.message??null});fs.writeFileSync(path.join(output,label+'.stdout'),result.stdout??'');fs.writeFileSync(path.join(output,label+'.stderr'),result.stderr??'');if(result.error||result.status!==0)differences.push({label,kind:'native-process',status:result.status,error:result.error?.message??null});return result;};
 const result=execute('cases',[]);try{const actual=JSON.parse(result.stdout);write('actual.json',actual);differences.push(...strictDifferences(expected,actual,'cases'));}catch(error){differences.push({kind:'native-json',message:error.message});}
 const life=execute('lifetime',['--lifetime']);if(life.status===0&&!/UniformsSymbol lifetime: 3 heaps passed/.test(life.stdout))differences.push({kind:'lifetime-result',actual:life.stdout});
 audit.binary={path:binary,beforeSHA256:before,afterSHA256:hash(binary)};if(before!==audit.binary.afterSHA256)differences.push({kind:'binary-changed'});
}
audit.differences=differences.length;write('differences.json',differences);write('audit.json',audit);console.log(JSON.stringify(audit));if(differences.length)process.exitCode=1;
