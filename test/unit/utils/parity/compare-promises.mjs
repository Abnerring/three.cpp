import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
if(process.argv.length!==3) throw new Error('Usage: node compare-promises.mjs <probe>');
const actual=JSON.parse(execFileSync(process.argv[2],{encoding:'utf8'}));
const expected=[];
const normalized=value=>value&&typeof value==='object'&&typeof value.name==='string'?value.name:value;
for(let fixture=0;fixture<34;fixture++) {
    const events=[];
    const observe=(promise,label='ok')=>promise.then(value=>{events.push([label,normalized(value)]);return value;},reason=>{events.push(['error',normalized(reason)]);return reason;});
    const thenable=mode=>({value:41,get then(){events.push('get then');if(mode===3)throw 'getter failure';if(mode===4)return 1;
        return function(resolve,reject){events.push('call then');if(mode===1){reject('first rejection');resolve(42);}else if(mode===2)throw 'then failure';else{resolve(this.value);reject('ignored');throw 'ignored throw';}};}});
    if(fixture===0) observe(new Promise((resolve,reject)=>{events.push('executor');resolve(7);reject(8);events.push('resolved');}));
    else if(fixture>=1&&fixture<=5){const value=thenable(fixture-1),promise=Promise.resolve(value);if(fixture===5)delete value.then;observe(promise);}
    else if(fixture===6){const d=Promise.withResolvers();observe(d.promise);d.resolve(d.promise);d.reject(9);}
    else if(fixture===7)observe(Promise.resolve(2).then(value=>{events.push(value);return value+1;}));
    else if(fixture===8)observe(Promise.resolve(2).then(()=>{throw 'handler failure';}));
    else if(fixture===9)observe(Promise.reject('original').catch(reason=>{events.push(reason);return 'recovered';}));
    else if(fixture>=10&&fixture<=15){const promise=fixture%2===0?Promise.resolve(17):Promise.reject('original');observe(promise.finally(function(){events.push('finally',arguments.length);if(fixture>=14)throw 'finally failure';return fixture>=12?Promise.reject('finally rejection'):Promise.resolve(99);}));}
    else if(fixture===16){const promise=Promise.resolve(1);events.push(Promise.resolve(promise)===promise);promise.constructor=undefined;const wrapped=Promise.resolve(promise);events.push(wrapped===promise);observe(wrapped);}
    else if(fixture===17){const d=Promise.withResolvers();observe(d.promise,'first');observe(d.promise,'second');queueMicrotask(()=>events.push('queued before'));d.resolve(12);queueMicrotask(()=>events.push('queued after'));observe(d.promise,'third');}
    else if(fixture===18){const value=thenable(0),d=Promise.withResolvers();d.resolve(value);d.reject('ignored before adoption');value.value=53;observe(d.promise);}
    else if(fixture===19){const promise=Promise.resolve(1);promise.constructor=null;try{promise.then();}catch(error){events.push(error.name);}}
    else if(fixture===20){const generic={then(a,b){events.push(this===generic,a===undefined,b);return 42;}};events.push(Promise.prototype.catch.call(generic,7));}
    else if(fixture===21){const d=Promise.withResolvers();d.resolve(61);observe(d.promise);}
    else if(fixture===22||fixture===23)observe(Promise.try(function(a,b){events.push(this===undefined,a);if(fixture===23)throw 'try failure';return b;},'argument',92));
    else if(fixture===24){let next;next=Promise.resolve(1).then(()=>next);observe(next);}
    else if(fixture===25)observe(Promise.reject('reason').then(3,4));
    else if(fixture===26)observe(new Promise(()=>{throw 'executor failure';}));
    else if(fixture===27)observe(new Promise(resolve=>{resolve(13);throw 'ignored executor failure';}));
    else if(fixture===28||fixture===29){function Target(){};const prototype=fixture===28?Promise.prototype:Object.create(Promise.prototype);Target.prototype=fixture===28?17:prototype;const promise=Reflect.construct(Promise,[resolve=>resolve(73)],Target);events.push(Object.getPrototypeOf(promise)===prototype,Object.getOwnPropertyNames(promise).length);observe(promise);}
    else if(fixture>=30&&fixture<=33){const prototype=fixture===33?Array.prototype:Object.prototype;const key=fixture===30?'resolve':fixture===31?'value':fixture===32?'handler':'0';const previous=Object.getOwnPropertyDescriptor(prototype,key),previousLength=Object.getOwnPropertyDescriptor(prototype,'length');Object.defineProperty(prototype,key,{__proto__:null,value:17,writable:false,configurable:true});let record,failure='';try{record=Promise.withResolvers();observe(record.promise);record.resolve(81);}catch(error){failure=error.message;}if(previous)Object.defineProperty(prototype,key,previous);else delete prototype[key];if(previousLength)Object.defineProperty(prototype,'length',previousLength);events.push(failure==='');if(failure)events.push(failure);else events.push(Object.hasOwn(record,'resolve')&&Object.getOwnPropertyDescriptor(record,'resolve').writable);}
    events.push('end');await new Promise(setImmediate);expected.push(events);
}
assert.deepEqual(actual,expected);
console.log(`Promise parity: ${expected.length} scenarios; native microtask/thenable/reaction traces match Node ${process.version}.`);
