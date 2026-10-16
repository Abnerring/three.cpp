import {spawnSync} from 'node:child_process';
import assert from 'node:assert/strict';
import fs from 'node:fs';
assert.equal(typeof global.gc,'function');
const snapshots=[];const job=()=>new Promise(resolve=>setImmediate(resolve));
async function collect(){for(let i=0;i<4;i++){await job();global.gc();}}
async function take(name,weak){await collect();snapshots.push({name,alive:weak.deref()!==undefined});}
function cycle(){const rx=/a/,value={rx};rx.lastIndex=value;return new WeakRef(rx);}
await take('instance-cycle',cycle());
const rx=/a/;function slot(){const value={};rx.lastIndex=value;return new WeakRef(value);}const weak=slot();await take('lastIndex-retained',weak);rx.lastIndex=0;await take('lastIndex-released',weak);
function intrinsic(proto,key){const value={},method=proto[key];method.payload=value;delete proto[key];return new WeakRef(value);}
await take('deleted-exec-intrinsic',intrinsic(RegExp.prototype,'exec'));
await take('deleted-string-intrinsic',intrinsic(String.prototype,'replace'));
function ordinary(){const value={};function fn(){}fn.payload=value;return new WeakRef(value);}await take('ordinary-function-control',ordinary());
const proc=spawnSync(process.argv[2],[],{encoding:'utf8'});assert.equal(proc.status,0,proc.stderr);const actual=JSON.parse(proc.stdout);
if(process.env.REGEXP_SAVE_PREFIX){fs.writeFileSync(process.env.REGEXP_SAVE_PREFIX+'-node.json',JSON.stringify(snapshots,null,2));fs.writeFileSync(process.env.REGEXP_SAVE_PREFIX+'-native.json',JSON.stringify(actual,null,2));}
assert.deepEqual(actual,snapshots);console.log(`RegExp/String GC: ${snapshots.length} actual Node ${process.version} snapshots`);
