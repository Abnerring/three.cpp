import { spawnSync } from 'node:child_process';
import assert from 'node:assert/strict';
import fs from 'node:fs';
const executable=process.argv[2];
const buffer=new ArrayBuffer(8), view=new DataView(buffer);
function bits(x){view.setFloat64(0,x);return view.getBigUint64(0).toString(16);}
function number(x){view.setBigUint64(0,x);return view.getFloat64(0);}
const cases=[];
const values=[0,-0,NaN,Infinity,-Infinity,Number.MIN_VALUE,-Number.MIN_VALUE,Number.MAX_VALUE,-Number.MAX_VALUE,1,-1,.5,-.5,1.25,-1.25,2.5,-2.5,2.55,2.35,1.005,1.015,.1,.3,1000000000000000128,1e21,1e21-131072,-1e21,999999999999999999999,1e-100,1e-101,1e-102,1e-20,1e20,0.10000000000000002,0.9999999999999999];
for(const value of values)for(let f=0;f<=100;f++)cases.push([value,f]);
for(const value of values)for(const f of [-Infinity,-101,-1,-.9,-0,NaN,100.9,101,Infinity])cases.push([value,f]);
for(let f=0;f<20;f++)for(let n=0;n<100;n++)for(const sign of [-1,1])cases.push([sign*(n+.5)/10**f,f]);
let state=0x95b37ad21cf083e7n;
function random(){state^=state<<13n;state^=state>>7n;state^=state<<17n;state=BigInt.asUintN(64,state);return state;}
for(let i=0;i<20000;i++){const value=number(random());cases.push([value,Number(random()%101n)]);}
for(let i=0;i<10000;i++){const value=number((random()&0x800fffffffffffffn)|(BigInt(900+Number(random()%194n))<<52n));cases.push([value,Number(random()%101n)]);}
const expected=cases.map(([n,f])=>{try{return n.toFixed(f);}catch(e){return {error:e.name};}});
const proc=spawnSync(executable,[],{input:cases.map(([n,f])=>bits(n)+' '+bits(f)).join('\n')+'\n',encoding:'utf8',maxBuffer:64*1024*1024});
assert.equal(proc.status,0,proc.stderr);
const actual=proc.stdout.trimEnd().split(/\r?\n/).map(JSON.parse);
assert.equal(actual.length,expected.length);
const failures=[];
for(let i=0;i<actual.length;i++){try{assert.deepEqual(actual[i],expected[i]);}catch{failures.push({index:i,bits:bits(cases[i][0]),number:cases[i][0],fractionDigits:cases[i][1],expected:expected[i],actual:actual[i]});}}
if(process.env.NUMBER_SAVE_PREFIX){fs.writeFileSync(process.env.NUMBER_SAVE_PREFIX+'-summary.json',JSON.stringify({node:process.version,cases:cases.length,failures:failures.length,examples:failures.slice(0,20)},null,2));}
assert.equal(failures.length,0,JSON.stringify(failures.slice(0,8),null,2));
console.log(`Native Number.toFixed oracle: ${cases.length} exact strings/errors against ${process.version}`);
