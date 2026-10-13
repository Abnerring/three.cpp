import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
const encode=v=>v===undefined?'undefined':v===null?null:typeof v==='number'?(Number.isNaN(v)?'NaN':Object.is(v,-0)?'-0':v):typeof v==='string'||typeof v==='boolean'?v:typeof v==='function'?'function':Object.fromEntries(Object.getOwnPropertyNames(v).map(k=>{const d=Object.getOwnPropertyDescriptor(v,k);return [k,'value'in d?encode(d.value):'accessor']}));
function run(mode){const left=new Array(4),right=new Array(2),events=[];left[0]=1;left[2]=undefined;left[3]=4;right[1]=8;let receiver=left,args=[right,7];const concat=left.concat;
 if(mode===1)Object.setPrototypeOf(left,Object.assign(Object.create(Object.getPrototypeOf(left)),{1:3}));
 if(mode===2)Object.setPrototypeOf(right,Object.assign(Object.create(Object.getPrototypeOf(right)),{0:6}));
 if(mode>=3&&mode<=8)Object.defineProperty(left,'0',{enumerable:true,configurable:true,get(){events.push(this===left?'left:0':'wrong');
  if(mode===3)left.length=1;if(mode===4){delete left[2];left[1]=5;left[4]=10;}if(mode===5){right.length=3;right[2]=9;}if(mode===6)throw RangeError('getter');
  if(mode===7){events.push(JSON.stringify(encode([])));global.gc();}if(mode===8){const p=Object.assign(Object.create(Object.getPrototypeOf(left)),{2:22});delete left[2];Object.setPrototypeOf(left,p);}return 2;}});
 if(mode>=9&&mode<=13)Object.defineProperty(left,'constructor',{configurable:true,get(){events.push('constructor');if(mode===10)throw TypeError('constructor');if(mode===11)return 3;if(mode===12)left.length=1;return mode===13?null:undefined;}});
 if(mode===14)receiver={length:3,0:42};if(mode===15)receiver='xy';if(mode===16)receiver=42;if(mode===17)receiver=true;if(mode===18)receiver=null;if(mode===19)receiver=undefined;
 if(mode===20)args=[left];if(mode===21){left.length=0;right.length=0;args=[right];}if(mode===22){left[0]=-0;left[3]=NaN;}if(mode===23)right[0]=[42];
 let pollutedPrototype,oldZero;if(mode===24){pollutedPrototype=Object.getPrototypeOf(left);oldZero=Object.getOwnPropertyDescriptor(pollutedPrototype,'0');Object.defineProperty(pollutedPrototype,'0',{configurable:true,set(){throw TypeError('prototype setter');}});}
 if(mode===25)Object.defineProperty(left,'constructor',{configurable:true,get(){events.push('constructor');right.length=4;right[3]=17;return undefined;}});
 if(mode===26)Object.defineProperty(left,'0',{enumerable:true,configurable:true,get(){events.push('reenter');const nested=right.concat(19);events.push(nested.length);return 2;}});
 if(mode===27)Object.defineProperty(right,'constructor',{configurable:true,get(){throw RangeError('argument constructor must not be read');}});
 if(mode===28||mode===30){const target=mode===28?right:left;if(mode===30)target.length=4294967295;Object.defineProperty(target,'0',{enumerable:true,configurable:true,get(){events.push('throw:first');throw RangeError('index');}});}
 if(mode===29){const readonly=Object.create(Object.getPrototypeOf(left));Object.defineProperty(readonly,'1',{value:5,writable:false,configurable:true});Object.setPrototypeOf(left,readonly);}
 if(mode===31){const argument={};Object.defineProperty(argument,'length',{configurable:true,get(){throw RangeError('non-array length must not be read');}});args=[argument,'abc'];}
 let result,error='';try{result=concat.apply(receiver,args);}catch(e){error=e.name;}finally{if(pollutedPrototype){if(oldZero)Object.defineProperty(pollutedPrototype,'0',oldZero);else delete pollutedPrototype[0];}}return {mode,result:encode(result),error,trace:encode(events),newIdentity:result!==receiver};}
const expected=Array.from({length:32},(_,i)=>run(i));const fn=Array.prototype.concat,d=Object.getOwnPropertyDescriptor(Array.prototype,'concat');let constructible;try{Reflect.construct(Object,[],fn);constructible=true;}catch{constructible=false;}
expected.push({kind:'metadata',name:fn.name,length:fn.length,constructible,flags:[d.writable,d.enumerable,d.configurable]});
const actual=JSON.parse(execFileSync(process.argv[2],[],{encoding:'utf8'}));assert.deepEqual(actual,expected);console.log(`${expected.length} actual Node concat scenarios passed; holes, mutation, constructor timing, exceptions, wrappers and metadata exact`);
