// Regression for the previous NaN-hole sorting divergence. The native adapter
// follows Node v26.5.0 / V8 14.6.202.34-node.24's comparison order, including
// this nontransitive comparator. Also covered by compare-earcut.mjs.
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync,spawnSync} from 'node:child_process';
import {strict as assert} from 'node:assert';

const upstream=path.resolve(process.argv[2]),binary=path.resolve(process.argv[3]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {default:earcut}=await import(pathToFileURL(path.join(upstream,'src/extras/lib/earcut.js')).href);
const data=[
    0,0,100,0,100,100,0,100,
    NaN,34,NaN,34,NaN,35,
    70,85,71,85,70,86,
    NaN,34,NaN,34,NaN,35,
    57,81,58,81,57,82,
    26,3,27,3,26,4,
    56,21,57,21,56,22,
    63,60,64,60,63,61,
    15,22,16,22,15,23
];
const holes=[4,7,10,13,16,19,22,25];
const js=earcut(data,holes);
const input=['T',2,data.length,...data,holes.length,...holes].join(' ')+'\n';
const run=spawnSync(binary,[],{input,encoding:'utf8'});
if(run.error)throw run.error;
assert.equal(run.status,0,run.stderr);
const cpp=JSON.parse(run.stdout)[0].triangles;
const firstMismatch=js.findIndex((value,index)=>value!==cpp[index]);
const differs=firstMismatch!==-1||js.length!==cpp.length;
console.log(JSON.stringify({node:process.version,v8:process.versions.v8,accepted:!differs,
    firstMismatch,cpp:cpp[firstMismatch],js:js[firstMismatch],cppLength:cpp.length,jsLength:js.length},null,2));
process.exitCode=differs?1:0;
