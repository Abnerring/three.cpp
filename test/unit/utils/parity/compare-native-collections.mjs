import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
const encode=value=>value===undefined?'@undefined':typeof value==='symbol'?'@symbol:'+(value.description??''):
 typeof value==='number'?Number.isNaN(value)?'@NaN':!Number.isFinite(value)?value<0?'@-Infinity':'@Infinity':Object.is(value,-0)?'@-0':value:
 typeof value==='function'?'@function':Array.isArray(value)?Array.from(value,encode):value&&typeof value==='object'?'@object':value;
const attempt=fn=>{try{return encode(fn())}catch(e){return {error:e.name}}};
const step=iterator=>{const r=iterator.next();return {value:encode(r.value),done:r.done}};
const contents=object=>Array.from(object,encode);
const keys=()=>[undefined,null,false,true,0,-0,1,1.5,NaN,Infinity,-Infinity,'','1',{},[],function(){},Symbol('x'),Symbol('x')];
const expected=[];
function constructors(map,failure,close) {
 const log=[],entry={get 0(){log.push('key');if(failure===8)throw RangeError('key');return 'k'},get 1(){log.push('value');if(failure===9)throw RangeError('value');return 6}};
 const iterator={index:0,get next(){
  log.push('getNext');if(failure===3)throw RangeError('getNext');
  return function(){log.push('next');if(failure===4)throw RangeError('next');if(failure===5)return 1;
   const index=this.index++;Object.defineProperty(this,'next',{value:17,writable:true,configurable:true});
   return {get done(){log.push('done');if(failure===6)throw RangeError('done');return index>0},get value(){log.push('item');if(failure===7)throw RangeError('item');return failure===10?12:entry}};
  };
 },get return(){log.push('getReturn');if(close===1)throw TypeError('getReturn');if(close===2)return 3;if(close===3)return null;if(close===4)return undefined;
  return function(){log.push('return');if(close===5)throw TypeError('return');return close===6?3:{}};
 }};
 const iterable={get [Symbol.iterator](){log.push('getIterator');if(failure===1)throw RangeError('getIterator');if(failure===2)return 3;return function(){log.push('iterator');return iterator}}};
 const C=map?Map:Set,adder=C.prototype[map?'set':'add'];function Target(){};Target.prototype=Object.create(C.prototype);
 Object.defineProperty(Target.prototype,map?'set':'add',{configurable:true,get(){log.push('getAdder');if(failure===11)throw RangeError('getAdder');if(failure===12)return 1;
  return function(...args){log.push('add');if(failure===13)throw RangeError('adder');return adder.apply(this,args)};
 }});
 let outcome;try{outcome=contents(Reflect.construct(C,[iterable],Target))}catch(e){outcome={error:e.name}}
 expected.push({kind:'constructor',map,failure,close,outcome,log});
}
for(const map of [true,false]) {
 const all=keys(),object=map?new Map():new Set(),add=map?'set':'add';
 for(let index=0;index<all.length;index++) {
  const returned=object[add](all[index],index),has=all.map(k=>object.has(k)),values=map?all.map(k=>encode(object.get(k))):[];
  expected.push({kind:'keys',map,index,self:returned===object,size:object.size,has,values,contents:contents(object)});
 }
 for(let index=0;index<all.length;index++)expected.push({kind:'delete',map,index,result:object.delete(all[index]),size:object.size,contents:contents(object)});
 for(let seed=1;seed<=16;seed++) {
  const object=map?new Map():new Set();for(let i=0;i<160;i++)object[add](i,i+1000);
  const iterators=[object.keys(),object.values(),object.entries()];let random=seed;
  for(let operation=0;operation<350;operation++) {
   random^=random<<13;random^=random>>>17;random^=random<<5;random>>>=0;const key=random%180,mode=random%11;let result;
   if(mode<4)result=object.delete(key);else if(mode<7)result=object[add](key,operation)===object;else if(mode<10)result=step(iterators[mode-7]);
   else if(operation%13===0){object.clear();result='clear'}else{iterators[key%3]=object[key%3===0?'keys':key%3===1?'values':'entries']();result='iterator'}
   expected.push({kind:'mutation',map,seed,operation,result,size:object.size});
  }
  expected.push({kind:'mutationFinal',map,seed,contents:contents(object)});
 }
 for(let mode=0;mode<9;mode++) {
  const object=map?new Map():new Set(),log=[],context={};let mutated=false;for(let i=0;i<3;i++)object[add](i,i*10);
  const outcome=attempt(()=>object.forEach(function(value,key,collection){
   log.push([value,key,collection===object,this===context]);if(mode===7&&key===1)throw RangeError('callback');
   if(!mutated){mutated=true;if(mode===1)object.delete(1);if(mode===2){object.delete(0);object[add](0,70)}
    if(mode===3){object.clear();object[add](7,70)}if(mode===4)object[add](1,999);if(mode===5)object[add](7,70);if(mode===6)object[add](0,900);
    if(mode===8)object.forEach((v,k)=>log.push(['nested',v,k]));
   }
  },context));
  expected.push({kind:'forEach',map,mode,outcome,log,contents:contents(object)});
 }
 for(let failure=0;failure<14;failure++)for(let close=0;close<7;close++)constructors(map,failure,close);
 const C=map?Map:Set,p=C.prototype;
 for(const method of ['has','delete','clear','entries','forEach','keys','values']) {
  const fn=p[method],d=Object.getOwnPropertyDescriptor(p,method);
  expected.push({kind:'metadata',map,method,name:fn.name,length:fn.length,writable:d.writable,enumerable:d.enumerable,configurable:d.configurable,constructible:false});
  for(let index=0;index<all.length;index++)expected.push({kind:'brand',map,method,index,result:attempt(()=>fn.call(all[index],function(){}))});
 }
 expected.push({kind:'prototype',map,tag:p[Symbol.toStringTag],iterAlias:p[Symbol.iterator]===p[map?'entries':'values'],species:C[Symbol.species]===C,call:attempt(()=>C()),sizeBrand:attempt(()=>p.size),keysValues:p.keys===p.values});
}
for(let mode=0;mode<3;mode++) {
 const events=[];let busy=false,iterator;const object={0:'a',1:'b',get length(){events.push('length');if(!busy){busy=true;events.push(iterator.next().value)}return 2}};
 iterator=Array.prototype[['keys','values','entries'][mode]].call(object);const results=[step(iterator),step(iterator),step(iterator)];
 expected.push({kind:'arrayReentry',mode,results,events:encode(events)});
}
for(const input of ['', 'ascii','A😀B','\ud800a','\udc00'])expected.push({kind:'string',input,units:Array.from(input)});
const actual=execFileSync(process.argv[2],{encoding:'utf8',maxBuffer:30*1024*1024}).trim().split(/\r?\n/).map(JSON.parse);
assert.equal(actual.length,expected.length);
for(let i=0;i<actual.length;i++)assert.deepEqual(actual[i],expected[i],`scenario ${i}: ${JSON.stringify(expected[i])}`);
let comparisons=0;const count=value=>{comparisons++;if(value&&typeof value==='object')for(const child of Object.values(value))count(child)};expected.forEach(count);
console.log(`Native collections: ${expected.length} Node scenarios / ${comparisons} comparisons passed (${process.version}).`);
