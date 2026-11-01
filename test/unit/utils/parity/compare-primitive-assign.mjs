import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {writeFileSync} from 'node:fs';
import {strictDifferences} from './strict-parity.mjs';

const expected=[],encode=v=>v===undefined?'undefined':typeof v==='symbol'?'symbol:'+(v.description??''):v!==null&&typeof v==='object'?'object':v;
const primitive=i=>[3,'abc',false,Symbol('receiver')][i];
for(let receiver=0;receiver<2;receiver++){
 const target=receiver===0?3:Symbol('boxed-target'),symbol=Symbol('assigned'),input={a:11,b:12,c:13,[symbol]:14};
 const box=Object.assign(target,input),other=Object.assign(target,input),keys=Reflect.ownKeys(box).map(encode),descriptors=Reflect.ownKeys(box).map(key=>{
  const d=Object.getOwnPropertyDescriptor(box,key);return {key:encode(key),value:encode(d.value),writable:d.writable,enumerable:d.enumerable,configurable:d.configurable};
 });
 expected.push({kind:'assign-box',receiver,object:typeof box==='object',distinctFromPrimitive:box!==target,valueOfSame:Object.is(box.valueOf(),target),prototypeSame:Object.getPrototypeOf(box)===Object.getPrototypeOf(Object(target)),freshBox:box!==other,existingIdentity:Object.assign(box,{},null,undefined)===box,keys,descriptors});
}
for(let receiver=0;receiver<2;receiver++){
 const log=[],input={get a(){log.push('read-source');return 9}};let outcome='ok';try{Object.assign(receiver===0?undefined:null,input)}catch(e){outcome=e.name}
 expected.push({kind:'nullish-assign',receiver,outcome,log});
}
for(let receiver=0;receiver<4;receiver++)for(let keyKind=0;keyKind<3;keyKind++){
 const target=primitive(receiver),prototype=Object.getPrototypeOf(Object(target)),key=keyKind===2?Symbol('setter'):'__threecpp_primitive_assign_probe__',log=[];
 assert.equal(Object.hasOwn(prototype,key),false);Object.defineProperty(prototype,key,{configurable:true,set(v){log.push(Object.is(this,target),v)}});
 let outcome='ok';try{target[key]=9}catch(e){outcome=e.name}
 const noOwn=!Object.hasOwn(Object(target),key);Reflect.deleteProperty(prototype,key);
 expected.push({kind:'inherited-setter',receiver,keyKind,outcome,log,noOwn});
}
for(let keyKind=0;keyKind<2;keyKind++){
 const target='abc',prototype=String.prototype,log=[];assert.equal(Object.hasOwn(prototype,'0'),false);Object.defineProperty(prototype,'0',{configurable:true,set(){log.push('unexpected-setter')}});
 let outcome='ok';try{target['0']=9}catch(e){outcome=e.name}
 const index=Object(target)['0'];Reflect.deleteProperty(prototype,'0');expected.push({kind:'readonly-string-index',keyKind,outcome,log,index});
}
for(let receiver=0;receiver<2;receiver++)for(let operation=0;operation<2;operation++)for(let mode=0;mode<6;mode++){
 const target=primitive(receiver),log=[],symbol=Symbol('key'),key={get [Symbol.toPrimitive](){log.push('get');if(mode===2)throw RangeError('get');if(mode===4)return 7;
  return hint=>{log.push(hint);if(mode===3)throw RangeError('call');if(mode===5)return {};return mode===0?'__collection_key__':symbol};
 }};
 let outcome='ok';try{if(operation===0)target[key]=9;else outcome=Reflect.set(target,key,9,target)}catch(e){outcome=e.name}
 expected.push({kind:'strict-vs-reflect',receiver,operation,mode,outcome,log});
}
assert.match(process.version,/^v26\./);assert.equal(expected.length,42);
if(process.argv[2]==='--oracle'){
 writeFileSync(process.argv[3],JSON.stringify({node:process.version,cases:expected,nativeExecuted:false},null,2)+'\n');
 console.log(JSON.stringify({node:process.version,scenarios:expected.length,nativeExecuted:false}));
}else{
 assert.ok(process.argv[2],'native probe executable required');
 const actual=execFileSync(process.argv[2],{encoding:'utf8',maxBuffer:8*1024*1024}).trim().split(/\r?\n/).map(JSON.parse);
 const differences=strictDifferences(expected,actual);
 if(process.argv[3]){writeFileSync(process.argv[3]+'-node.json',JSON.stringify(expected,null,2)+'\n');writeFileSync(process.argv[3]+'-native.json',JSON.stringify(actual,null,2)+'\n');writeFileSync(process.argv[3]+'-differences.json',JSON.stringify(differences,null,2)+'\n');}
 assert.equal(differences.length,0,JSON.stringify(differences.slice(0,12)));console.log(`Primitive assignment: ${expected.length} strict Node scenarios passed (${process.version}).`);
}
