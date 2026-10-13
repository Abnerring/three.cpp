import { spawnSync } from 'node:child_process';
import fs from 'node:fs';
import assert from 'node:assert/strict';
const executable=process.argv[2];
function receiver(kind,log){switch(kind){case 0:return 1.25;case 1:return -0;case 2:return NaN;case 3:return Infinity;case 4:return -Infinity;case 5:{const o=new Number(1.25);o.valueOf=()=>{log.push('receiver-valueOf');throw new TypeError('unexpected');};return o;}case 6:return Number.prototype;case 7:return {valueOf(){log.push('receiver-valueOf');return 1.25;}};case 8:return Object.create(Number.prototype);case 9:return true;case 10:return '1.25';case 11:return null;case 12:return undefined;case 13:return Symbol('receiver');case 14:return [];case 15:return function(){};default:return new String('1.25');}}
function input(kind,log){switch(kind){case 0:return undefined;case 1:return null;case 2:return false;case 3:return true;case 4:return '2';case 5:return '100.9';case 6:return '-0.9';case 7:return '-1';case 8:return '101';case 9:return Symbol('fraction');case 18:return new Number(2);case 22:return 10;case 24:return NaN;case 25:return Infinity;case 26:return -Infinity;}
 const o={};
 function action(...args){if(kind===23){log.push('toString');return '4';}if(kind===11){log.push('primitive:'+args[0]);return 3;}if(kind===13){log.push('valueOf');return {};}if(kind===14){log.push('valueOf');throw new RangeError('fraction');}if(kind===15){log.push('primitive:'+args[0]);throw new TypeError('fraction');}if(kind===16){log.push('valueOf');log.push('inner:'+(1.25).toFixed(1));return 2;}if(kind===17){log.push('valueOf');Number.prototype.toFixed=()=> 'replaced';return 2;}if(kind===19){log.push('valueOf');return Symbol('fraction');}if(kind===20){log.push('primitive:'+args[0]);return {};}log.push('valueOf');return 2;}
 if([11,15,20].includes(kind))Object.defineProperty(o,Symbol.toPrimitive,{get(){log.push('get-primitive');return action;}});
 else if(kind===12){o.valueOf=2;Object.defineProperty(o,Symbol.toPrimitive,{get(){log.push('get-primitive');return 1;}});}
 else if(kind===21){Object.defineProperty(o,'valueOf',{get(){log.push('get-valueOf');return 0;}});Object.defineProperty(o,'toString',{get(){log.push('get-toString');return ()=>{log.push('toString');return '4';};}});}
 else {o.valueOf=action;if(kind===13)o.toString=()=>{log.push('toString');return '4';};}
 return o;
}
const encode=v=>typeof v+':'+(Object.is(v,-0)?'-0':String(v));
const results=[];
for(const method of ['toFixed','valueOf','toString'])for(let r=0;r<17;r++)for(let f=0;f<27;f++){
 const original=Object.getOwnPropertyDescriptor(Number.prototype,'toFixed'),fn=Number.prototype[method],trace=[];
 const self=receiver(r,trace),fraction=input(f,trace),result={method,receiver:r,fraction:f};
 try{result.value=encode(fn.call(self,fraction));}catch(e){result.error=e.name;}
 Object.defineProperty(Number.prototype,'toFixed',original);result.trace=trace;results.push(result);
}
{
 const methods=['toFixed','toString','valueOf'].map(name=>{const d=Object.getOwnPropertyDescriptor(Number.prototype,name),fn=d.value;let constructible=true;try{Reflect.construct(fn,[]);}catch(e){assert.equal(e.name,'TypeError');constructible=false;}return {name:fn.name,length:fn.length,writable:d.writable,enumerable:d.enumerable,configurable:d.configurable,constructible,ownPrototype:Object.hasOwn(fn,'prototype'),source:fn.toString()};});
 const boxed=new Number(-0);results.push({metadata:methods,prototypeValue:encode(Number.prototype.valueOf()),prototypeBrand:Object.prototype.toString.call(Number.prototype),boxedBrand:Object.prototype.toString.call(boxed),boxedValue:encode(boxed.valueOf()),boxedOwnNames:Reflect.ownKeys(boxed)});
}
{
 const original=Object.getOwnPropertyDescriptor(Number.prototype,'toFixed'),log=[],symbol=Symbol('dynamic');
 const read=function(){log.push('get:'+encode(this));return original.value;};
 Object.defineProperty(Number.prototype,'toFixed',{get:read,configurable:true});Object.defineProperty(Number.prototype,symbol,{get:read,configurable:true});
 const number=1.25,fixed=number.toFixed;log.push('result:'+fixed.call(number,1));const symbolMethod=number[symbol];log.push('symbol:'+symbolMethod.call(number,2));const other=Reflect.get(Number.prototype,'toFixed',-2.5);log.push('other:'+other.call(-2.5,0));
 Object.defineProperty(Number.prototype,'toFixed',original);delete Number.prototype[symbol];results.push({mutation:log});
}
const proc=spawnSync(executable,[],{encoding:'utf8',maxBuffer:32*1024*1024});assert.equal(proc.status,0,proc.stderr);
const native=JSON.parse(proc.stdout);assert.equal(native.length,results.length);
let boundaries=0;
for(let i=0;i<results.length;i++){
 if(native[i].boundary){const actual=results[i];assert.equal(actual.method,'toString');assert.ok([0,1,5,6].includes(actual.receiver));assert.ok([4,10,11,13,16,17,18,21,23].includes(actual.fraction));assert.ok(actual.value?.startsWith('string:'));assert.ok(!['string:NaN','string:Infinity','string:-Infinity'].includes(actual.value));assert.deepEqual(native[i].trace,actual.trace);boundaries++;}
 else assert.deepEqual(native[i],results[i],JSON.stringify(results[i]));
}
if(process.env.NUMBER_SAVE_PREFIX){fs.writeFileSync(process.env.NUMBER_SAVE_PREFIX+'-node.json',JSON.stringify(results));fs.writeFileSync(process.env.NUMBER_SAVE_PREFIX+'-native.json',JSON.stringify(native));}
console.log(`Native Number protocol: ${results.length-boundaries} Node cases exact; ${boundaries} explicitly unsupported finite non-decimal radix cases verified separately`);
