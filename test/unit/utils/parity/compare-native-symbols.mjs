import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {writeFileSync} from 'node:fs';
const encode=v=>v===undefined?'undefined':typeof v==='symbol'?'symbol:'+(v.description??''):typeof v==='function'?'function':typeof v==='number'?Number.isNaN(v)?'NaN':Object.is(v,-0)?'-0':v:v!==null&&typeof v==='object'?'object':v;
const descriptor=d=>!d?null:'value'in d?{value:encode(d.value),writable:d.writable,enumerable:d.enumerable,configurable:d.configurable}:{get:encode(d.get),set:encode(d.set),enumerable:d.enumerable,configurable:d.configurable};
const expected=[],get=()=>7,set=function(v){this.setValue=v};
for(let original=0;original<5;original++)for(let flags=0;flags<8;flags++)for(let update=0;update<13;update++){
 const object={},key=Symbol('key');if(original){const d={configurable:!!(flags&1),enumerable:!!(flags&2)};if(original<3){d.value=original===1?0:NaN;d.writable=!!(flags&4)}else if(original===3)d.get=get;else d.set=set;Object.defineProperty(object,key,d)}
 const change=[{},{value:0},{value:-0},{value:NaN},{writable:false},{writable:true},{enumerable:!(flags&2)},{configurable:true},{get},{get:()=>7},{set},{get:undefined},{get,value:9}][update];
 let outcome;try{outcome=Reflect.defineProperty(object,key,change)}catch(e){outcome=e.name}
 expected.push({kind:'descriptor',original,flags,update,outcome,descriptor:descriptor(Object.getOwnPropertyDescriptor(object,key))});
}
for(let inherited=0;inherited<5;inherited++)for(let receiver=0;receiver<5;receiver++){
 const key=Symbol('key'),prototype={},target=Object.create(prototype);
 if(inherited){const d={configurable:true};if(inherited<3){d.value=1;d.writable=inherited===1}else if(inherited===3)d.get=get;else d.set=set;Object.defineProperty(prototype,key,d)}
 if(receiver===1)Object.preventExtensions(target);if(receiver>=2&&receiver<4)Object.defineProperty(target,key,receiver===2?{value:2,writable:false,configurable:true}:{get,configurable:true});
 let outcome;try{outcome=Reflect.set(prototype,key,9,receiver===4?12:target)}catch(e){outcome=e.name}
 expected.push({kind:'inherited',inherited,receiver,outcome,value:encode(target[key]),setValue:encode(target.setValue),own:Object.hasOwn(target,key)});
}
for(let mode=0;mode<8;mode++){
 const object={},log=[],symbol=Symbol('key'),key={get [Symbol.toPrimitive](){log.push('get');if(mode===1)throw RangeError('get');if(mode===2)return 3;if(mode===3)return null;
  return hint=>{log.push(hint);if(mode===4)throw RangeError('call');if(mode===5)return {};if(mode===6)return 'string-key';return symbol};
 }};
 let outcome;try{object[key]=9;outcome='ok'}catch(e){outcome=e.name}
 expected.push({kind:'coercion',mode,outcome,keys:Reflect.ownKeys(object).map(encode),log});
}
const object={},a=Symbol('same'),b=Symbol('same');object[a]=1;object.b=2;object[1]=3;object[b]=4;object.a=5;delete object[a];object[a]=6;
expected.push({kind:'order',keys:Reflect.ownKeys(object).map(encode),identity:a!==b,first:Object.getOwnPropertySymbols(object)[0]===b,second:Object.getOwnPropertySymbols(object)[1]===a,json:JSON.stringify(object)});
for(let operation=0;operation<10;operation++)for(let receiver=0;receiver<9;receiver++)for(let mode=0;mode<6;mode++){
 if(operation===9&&receiver>=2&&receiver<=5)continue;
 const target=[undefined,null,false,0,'',Symbol('receiver'),{},[],()=>7][receiver];
 const log=[],symbol=Symbol('key'),key={get [Symbol.toPrimitive](){log.push('get');if(mode===2)throw RangeError('get');if(mode===4)return 7;
  return hint=>{log.push(hint);if(mode===3)throw RangeError('call');if(mode===5)return {};return mode===0?'__collection_key__':symbol};
 }};
 let outcome;
 try{
  switch(operation){
   case 0:outcome=encode(target[key]);break;
   case 1:outcome=key in target;break;
   case 2:outcome=Object.hasOwn(target,key);break;
   case 3:outcome=descriptor(Reflect.getOwnPropertyDescriptor(target,key));break;
   case 4:outcome=Reflect.set(target,key,9,target);break;
   case 5:outcome=Reflect.defineProperty(target,key,{value:9});break;
   case 6:outcome=Reflect.deleteProperty(target,key);break;
   case 7:Reflect.set(target,key,9,target);outcome='ok';break;
   case 8:outcome=Object.prototype.hasOwnProperty.call(target,key);break;
   case 9:outcome=Object.prototype.propertyIsEnumerable.call(target,key);break;
  }
 }catch(e){outcome=e.name}
 expected.push({kind:'receiver-key-order',operation,receiver,mode,outcome,log});
}
const actual=execFileSync(process.argv[2],{encoding:'utf8',maxBuffer:8*1024*1024}).trim().split(/\r?\n/).map(JSON.parse);
if(process.argv[3]){writeFileSync(`${process.argv[3]}-node.json`,JSON.stringify(expected,null,2));writeFileSync(`${process.argv[3]}-native.json`,JSON.stringify(actual,null,2));}
assert.equal(actual.length,expected.length);for(let i=0;i<actual.length;i++)assert.deepEqual(actual[i],expected[i],`scenario ${i}`);
let comparisons=0;const count=x=>{comparisons++;if(x&&typeof x==='object')for(const v of Object.values(x))count(v)};expected.forEach(count);
console.log(`Native Symbol properties: ${expected.length} Node scenarios / ${comparisons} comparisons passed (${process.version}).`);
