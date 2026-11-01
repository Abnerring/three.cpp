import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { spawnSync } from 'node:child_process';
const options={};for(let i=2;i<process.argv.length;++i){const key=process.argv[i];if(key==='--validation'){assert.equal(options.validation,undefined);options.validation=true;continue;}assert.ok(['--probe','--output','--expected'].includes(key)&&process.argv[i+1]);assert.equal(options[key.slice(2)],undefined);options[key.slice(2)]=process.argv[++i];}
assert.ok(options.probe&&options.output&&options.expected);assert.match(path.resolve(options.output),/^D:[\\/]/i);assert.equal(fs.existsSync(options.output),false);fs.mkdirSync(options.output,{recursive:true});
const sha=filename=>createHash('sha256').update(fs.readFileSync(filename)).digest('hex'),binarySHA256=sha(options.probe),expectedSHA256=sha(options.expected);
const run=spawnSync(options.probe,options.validation?['--validation']:[],{encoding:'utf8',timeout:300000,maxBuffer:32*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});
fs.writeFileSync(options.output+'/stdout.json',run.stdout??'');fs.writeFileSync(options.output+'/stderr.log',run.stderr??'');fs.writeFileSync(options.output+'/process.json',JSON.stringify({binary:options.probe,binarySHA256,expected:options.expected,expectedSHA256,pid:run.pid??null,status:run.status,signal:run.signal,error:run.error?String(run.error):null,args:options.validation?['--validation']:[]},null,2)+'\n');
assert.equal(run.error,undefined);assert.equal(run.status,0,run.stderr);assert.equal(sha(options.probe),binarySHA256);assert.equal(sha(options.expected),expectedSHA256);
const actual=JSON.parse(run.stdout),expected=JSON.parse(fs.readFileSync(options.expected));
assert.equal(actual.actualGPUExecuted,true);assert.ok(typeof actual.deviceName==='string'&&actual.deviceName.length>0);assert.equal(actual.validationMessages,0);if(options.validation)assert.equal(actual.validationEnabled,true);
assert.equal(actual.format,expected.format);assert.equal(actual.bytesPerPixel,expected.bytesPerPixel);assert.equal(JSON.stringify(actual.clearSamples),JSON.stringify(expected.clearSamples),'Strict ordered clear samples differ from independent real pinned Node encoding oracle');
assert.equal(JSON.stringify(actual.cases),JSON.stringify(['half-clear-readback','rgba8-interleave','half-texture-rows','equirect-six-faces','half-blur','lifecycle-format-change','recording-fence','recording-abort','unsupported-transaction']));assert.ok(actual.assertions>0);
fs.writeFileSync(options.output+'/summary.json',JSON.stringify({...actual,binarySHA256,expectedSHA256,strictNodeClearOraclePassed:true},null,2)+'\n');console.log(JSON.stringify({output:options.output,device:actual.deviceName,cases:actual.cases.length,assertions:actual.assertions,actualGPUExecuted:true}));
