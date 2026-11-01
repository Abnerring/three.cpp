import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {createHash} from 'node:crypto';
import {fixtures} from './symbol-fixtures.mjs';
import {run,gcEvidence} from './symbol-oracle.mjs';
import {strictDifferences} from './strict-parity.mjs';
const options={},argv=process.argv.slice(2);
for(let i=0;i<argv.length;i++){const key=argv[i];assert(!Object.hasOwn(options,key),'Duplicate '+key);if(key==='--oracle-only')options[key]=true;else{assert(['--output','--input','--probe'].includes(key),'Unknown '+key);assert(argv[i+1]&&!argv[i+1].startsWith('--'),'Missing '+key);options[key]=argv[++i];}}
assert.equal(process.version,'v26.5.0');assert.equal(typeof gc,'function','Use --expose-gc');
for(const name of ['TEMP','TMP'])assert.equal(process.env[name]?.replaceAll('\\','/').toLowerCase(),'d:/temp/surui');
assert(options['--output']);assert(Boolean(options['--oracle-only'])!==Boolean(options['--probe']),'Use exactly one --oracle-only / --probe');
const output=path.resolve(options['--output']);assert(!fs.existsSync(output),'Output must be fresh');fs.mkdirSync(output,{recursive:true});
const write=(file,value)=>fs.writeFileSync(path.join(output,file),JSON.stringify(value,null,2)+'\n'),hash=file=>createHash('sha256').update(fs.readFileSync(file)).digest('hex');
const input=options['--input']?JSON.parse(fs.readFileSync(options['--input'],'utf8')):fixtures;assert(Array.isArray(input));
const expected=input.map(request=>({name:request.name,value:run(request)})),expectedGC=await gcEvidence();
write('input.json',input);write('expected.json',expected);write('gc-expected.json',expectedGC);
const roundTrip=strictDifferences(expected,JSON.parse(JSON.stringify(expected)));write('wire-differences.json',roundTrip);assert.equal(roundTrip.length,0,'Symbol output transport must preserve its normalized snapshot');
const audit={node:process.version,v8:process.versions.v8,arguments:argv,cwd:process.cwd(),TEMP:process.env.TEMP,TMP:process.env.TMP,cases:input.length,gcPhases:expectedGC.length,nativeExecuted:false,processes:[],scope:'Real ECMAScript Node Symbol intrinsic, primitive access/strict write, boxing, registry/weak eligibility, metadata, private authorities and collection; no embedded engine, browser, CEF or GPU.'};
const differences=[];
if(options['--probe']){
 const binary=path.resolve(options['--probe']);assert(path.isAbsolute(options['--probe']));const before=hash(binary);
 const execute=(label,args,input)=>{const result=spawnSync(binary,args,{input,encoding:'utf8',maxBuffer:128*1024*1024,env:process.env});const executed=Number.isInteger(result.pid)&&result.pid>0;audit.nativeExecuted ||= executed;audit.processes.push({label,args,pid:result.pid??null,executed,status:result.status,signal:result.signal,error:result.error?.message??null});fs.writeFileSync(path.join(output,label+'.stdout'),result.stdout??'');fs.writeFileSync(path.join(output,label+'.stderr'),result.stderr??'');if(result.error||result.status!==0)differences.push({label,kind:'native-process',status:result.status,error:result.error?.message??null});return result;};
 const compare=(label,result,value)=>{let actual;try{actual=JSON.parse(result.stdout);}catch(error){differences.push({label,kind:'invalid-json',message:error.message});return;}write(label+'-actual.json',actual);differences.push(...strictDifferences(value,actual,label));};
 compare('cases',execute('cases',[],JSON.stringify(input)),expected);
 compare('gc',execute('gc',['--gc']),expectedGC);
 const life=execute('lifetime',['--lifetime']);if(life.status===0&&!/SymbolRuntime lifetime: 3 heaps passed/.test(life.stdout))differences.push({kind:'lifetime-result',actual:life.stdout});
 audit.binary={path:binary,beforeSHA256:before,afterSHA256:hash(binary)};if(audit.binary.beforeSHA256!==audit.binary.afterSHA256)differences.push({kind:'binary-changed'});
}
audit.differences=differences.length;write('differences.json',differences);write('audit.json',audit);console.log(JSON.stringify(audit));if(differences.length)process.exitCode=1;
