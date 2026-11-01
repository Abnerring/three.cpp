import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {pathToFileURL} from 'node:url';
import {execFileSync,spawnSync} from 'node:child_process';
import {strictDifferences} from './strict-parity.mjs';

const [upstream,executable,directory]=process.argv.slice(2);
assert(upstream&&executable&&directory,'Usage: node --expose-gc compare-tsl-chaining-source.mjs <fixed-upstream> <--oracle|probe.exe> <new-output>');
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--untracked-files=no','--','src'],{encoding:'utf8'}).trim(),'');
assert.equal(typeof globalThis.gc,'function','Real explicit GC requires --expose-gc');
fs.mkdirSync(directory,{recursive:false});
const {addMethodChaining}=await import(pathToFileURL(path.join(upstream,'src/nodes/tsl/TSLCore.js')));
const {default:Node}=await import(pathToFileURL(path.join(upstream,'src/nodes/core/Node.js')));
const names=['sourceFixtureAlpha','sourceFixtureBeta','sourceFixtureGamma'],states=new Map();
const normal=(v,state)=>v===undefined?'#undefined':v===state.node?'#receiver':v===state.result?'#result':v;
for(const name of names){
 const state={};states.set(name,state);
 const element=function(...args){globalThis.gc();state.events.push(['element',name,this===undefined,...args.map(v=>normal(v,state))]);if(state.throwElement)throw new TypeError('fixture-element');return state.result;};
 addMethodChaining(name,element);
}
function metadata(name,assign){
 const key=name+(assign?'Assign':''),fn=Node.prototype[key],d=Object.getOwnPropertyDescriptor(Node.prototype,key),p=Object.getOwnPropertyDescriptor(fn,'prototype');
 return {name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn),ownKeys:Object.getOwnPropertyNames(fn),hasOwnToString:Object.hasOwn(fn,'toString'),prototypeDescriptor:[!!p.writable,!!p.enumerable,!!p.configurable,fn.prototype.constructor===fn],methodDescriptor:[!!d.writable,!!d.enumerable,!!d.configurable],stableIdentity:fn===Node.prototype[key],distinctAssign:Node.prototype[name]!==Node.prototype[name+'Assign']};
}
function run(r){
 const state=states.get(r.name);state.events=[];state.result={};state.throwElement=r.throwElement;state.node=new Node();
 Object.defineProperty(state.node,'isStackNode',{configurable:true,get(){globalThis.gc();state.events.push(['stack.get']);return r.stack;}});
 for(const key of ['assign','addToStack'])Object.defineProperty(state.node,key,{configurable:true,get(){globalThis.gc();state.events.push(['get',key]);return function(...args){globalThis.gc();state.events.push(['call',key,this===state.node,...args.map(v=>normal(v,state))]);return state.result;};}});
 const out={metadata:metadata(r.name,r.assign)};
 try{out.result=normal(Reflect.apply(Node.prototype[r.name+(r.assign?'Assign':'')],state.node,r.args),state);}catch(e){out.result='#'+e.name;}
 out.events=state.events;
 return out;
}
const requests=[];
for(const name of names)for(const assign of [false,true])for(const stack of [false,true])for(const args of [[],[7,9]])for(const throwElement of [false,true])requests.push({name,assign,stack,args,throwElement});
const expected=requests.map(run);
const summaryMetadata={names,methodCount:6,allDifferent: new Set(names.flatMap(name=>[Node.prototype[name],Node.prototype[name+'Assign']])).size===6,builtins:['toFloat','element'].map(name=>({name,normal:metadata(name,false),assign:metadata(name,true)}))};
assert(summaryMetadata.allDifferent);
const save=(name,v)=>fs.writeFileSync(path.join(directory,name),JSON.stringify(v,null,2)+'\n');
save('input.json',requests);save('expected.json',expected);save('metadata.json',summaryMetadata);
if(executable==='--oracle'){save('result.json',{node:process.version,caseCount:requests.length,sourceNames:names,nativeExecuted:false});console.log(JSON.stringify({caseCount:requests.length,nativeExecuted:false}));process.exit(0);}
const native=spawnSync(executable,[],{input:JSON.stringify(requests),encoding:'utf8',windowsHide:true,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});
fs.writeFileSync(path.join(directory,'native.stdout.log'),native.stdout??'');fs.writeFileSync(path.join(directory,'native.stderr.log'),native.stderr??'');
assert.equal(native.status,0,'Native probe failed');
const actual=JSON.parse(native.stdout);save('actual.json',actual);
const differences=[...strictDifferences(expected,actual.cases,'cases'),...strictDifferences(summaryMetadata,actual.metadata,'metadata')];save('differences.json',differences);
const lifetime=spawnSync(executable,['--lifetime'],{encoding:'utf8',windowsHide:true,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}});
fs.writeFileSync(path.join(directory,'lifetime.stdout.log'),lifetime.stdout??'');fs.writeFileSync(path.join(directory,'lifetime.stderr.log'),lifetime.stderr??'');
assert.equal(lifetime.status,0,'Native closure lifetime probe failed');
const lifetimeActual=JSON.parse(lifetime.stdout);save('lifetime.json',lifetimeActual);
const lifetimeDifferences=strictDifferences(Array.from({length:3},()=>[true,true,true,true]),lifetimeActual,'lifetime');save('lifetime-differences.json',lifetimeDifferences);
assert.equal(lifetimeDifferences.length,0,JSON.stringify(lifetimeDifferences));
save('result.json',{node:process.version,caseCount:requests.length,differenceCount:differences.length,nativeExecuted:true,lifetimeExecuted:true});
assert.equal(differences.length,0,JSON.stringify(differences.slice(0,8)));
console.log(JSON.stringify({caseCount:requests.length,differenceCount:0,lifetimeExecuted:true}));
