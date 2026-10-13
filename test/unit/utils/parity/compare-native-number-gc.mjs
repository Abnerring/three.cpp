import { spawnSync } from 'node:child_process';
import assert from 'node:assert/strict';
import fs from 'node:fs';
assert.equal(typeof gc,'function');
async function collect(){for(let i=0;i<6;i++){await new Promise(resolve=>setImmediate(resolve));gc();}}
const expected=[];let weakBox,weakPayload,kept;
function setupBox(){const box=new Number(1.25),payload={};box.self=box;box.payload=payload;weakBox=new WeakRef(box);weakPayload=new WeakRef(payload);kept=box;}setupBox();
await collect();expected.push({step:'boxed-kept',box:!!weakBox.deref(),payload:!!weakPayload.deref()});
kept=undefined;await collect();expected.push({step:'boxed-drop',box:!!weakBox.deref(),payload:!!weakPayload.deref()});
function setupPrototype(){const payload={};weakPayload=new WeakRef(payload);Number.prototype.payload=payload;}setupPrototype();
await collect();expected.push({step:'prototype-kept',payload:!!weakPayload.deref()});
delete Number.prototype.payload;await collect();expected.push({step:'prototype-delete',payload:!!weakPayload.deref()});
let weakMethod;
function setupMethod(){const method=Number.prototype.toFixed,payload={};weakMethod=new WeakRef(method);weakPayload=new WeakRef(payload);method.payload=payload;kept=method;delete Number.prototype.toFixed;}setupMethod();
await collect();expected.push({step:'method-kept',method:!!weakMethod.deref(),payload:!!weakPayload.deref(),result:kept.call(1.25,1)});
kept=undefined;await collect();expected.push({step:'method-drop',method:!!weakMethod.deref(),payload:!!weakPayload.deref()});
const proc=spawnSync(process.argv[2],[],{encoding:'utf8'});assert.equal(proc.status,0,proc.stderr);const actual=JSON.parse(proc.stdout);
if(process.env.NUMBER_SAVE_PREFIX){fs.writeFileSync(process.env.NUMBER_SAVE_PREFIX+'-node.json',JSON.stringify(expected));fs.writeFileSync(process.env.NUMBER_SAVE_PREFIX+'-native.json',JSON.stringify(actual));}
assert.deepEqual(actual,expected);console.log(`Native Number GC: ${expected.length} actual Node snapshots passed (${process.version}); GC scheduling is host-specific`);
