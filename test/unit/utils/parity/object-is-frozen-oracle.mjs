import fs from 'node:fs';
import assert from 'node:assert/strict';
assert.equal(process.version,'v26.5.0');
assert.equal(typeof gc,'function');
const fixtures=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));
const isFrozen=Object.isFrozen,freeze=Object.freeze,descriptors=Object.getOwnPropertyDescriptor;
const key=k=>typeof k==='symbol'?{$symbol:k.description??null}:k;
function encode(v){if(v===undefined)return {$type:'undefined'};if(typeof v==='symbol')return key(v);if(typeof v==='function')return {$type:'function'};if(typeof v==='object'&&v!==null)return {$type:'object'};if(typeof v==='number'){if(Number.isNaN(v))return {$number:'NaN'};if(Object.is(v,-0))return {$number:'-0'};if(!Number.isFinite(v))return {$number:v<0?'-Infinity':'Infinity'};}return v;}
function descriptor(d){if(!d)return null;const out={};for(const field of ['value','writable','get','set','enumerable','configurable'])if(Object.hasOwn(d,field))out[field]=encode(d[field]);return out;}
function snapshot(target){const extensible=Reflect.isExtensible(target);const entries=Reflect.ownKeys(target).map(k=>[key(k),descriptor(descriptors(target,k))]);return {extensible,entries};}
function attempt(body,reason){try{return {returned:true,same:body()};}catch(error){if(reason!==undefined&&error===reason)return {thrownOriginal:true};return {error:error.name};}}
function metadata(){const d=descriptors(Object,'isFrozen'),fn=d.value;return {name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn),descriptor:{writable:d.writable,enumerable:d.enumerable,configurable:d.configurable},ownKeys:Reflect.ownKeys(fn).map(key),nameDescriptor:descriptor(descriptors(fn,'name')),lengthDescriptor:descriptor(descriptors(fn,'length')),construct:attempt(()=>{new fn();return false;}),noArguments:isFrozen()};}
function primitive(label){const symbol=Symbol('fixture'),values={undefined,null:null,false:false,true:true,zero:0,'negative-zero':-0,number:3,NaN:NaN,Infinity:Infinity,'negative-Infinity':-Infinity,'empty-string':'',string:'ab','astral-string':'a😀b',symbol};const value=values[label];return {value:encode(value),result:isFrozen.call({unused:1},value,{extra:true})};}
function setup(target,state,first,second){
 if(state==='extensible')return;
 if(state==='frozen'){freeze(target);return;}
 Object.preventExtensions(target);
 if(state==='sealed'){for(const k of Reflect.ownKeys(target))Reflect.defineProperty(target,k,{configurable:false});return;}
 if(state==='mixed'){for(const k of Reflect.ownKeys(target)){if(k===second)continue;const d=Reflect.getOwnPropertyDescriptor(target,k);const update={configurable:false};if(Object.hasOwn(d,'value'))update.writable=false;Reflect.defineProperty(target,k,update);}return;}
 assert.equal(state,'nonextensible');
}
function basic(shape,state){const events=[],symbol=Symbol('fixture'),child={value:1};let target;
 if(shape==='empty')target={};
 else if(shape==='data'){target={a:1,b:2};Object.defineProperty(target,'hidden',{value:child,writable:true,configurable:true});target[symbol]=3;}
 else if(shape==='accessor'){target={};Object.defineProperty(target,'a',{get(){events.push(['unexpected-accessor-get']);return child;},set(v){events.push(['unexpected-accessor-set',encode(v)]);},enumerable:true,configurable:true});target[symbol]=3;}
 else if(shape==='array'){target=new Array(4);target[1]=2;target.extra=3;target[symbol]=4;}
 else if(shape==='function')target=()=>7;
 else if(shape==='string-wrapper')target=Object('a😀b');
 else if(shape==='typed-empty')target=new Uint8Array(0);
 else if(shape==='typed-full')target=new Uint8Array(2);
 else throw new Error('Unknown basic shape '+shape);
 let setupError=null;try{setup(target,state);}catch(error){setupError={error:error.name};}
 return {setupError,result:isFrozen(target),snapshot:snapshot(target),events};
}
function proxyCase(state,mode,isArray){const events=[],symbol=Symbol('fixture'),reason={reason:'original'},control={entered:false};const base=isArray?[1,2]:{a:1,b:2};base[symbol]=3;
 const first=isArray?'0':'a',second=isArray?'1':'b';
 if(mode==='accessor')Object.defineProperty(base,first,{get(){events.push(['unexpected-accessor-get']);return 1;},set(){events.push(['unexpected-accessor-set']);},enumerable:true,configurable:true});
 setup(base,state,first,second);
 function make(target,label){const handler={};for(const name of ['isExtensible','ownKeys','getOwnPropertyDescriptor'])Object.defineProperty(handler,name,{configurable:true,get(){
  events.push([label,'get',name]);gc();
  if(mode==='null-traps')return null;
  if((mode==='isextensible-getter-throw'&&name==='isExtensible')||(mode==='ownkeys-getter-throw'&&name==='ownKeys'))throw reason;
  return function(...args){events.push([label,'call',name,this===handler,...(name==='getOwnPropertyDescriptor'?[key(args[1])]:[])]);gc();
   if(name==='isExtensible'){
    if(mode==='isextensible-throw')throw reason;
    if(mode==='false-isextensible')return false;
    if(mode==='true-isextensible')return true;
    if(mode==='truthy-isextensible')return {valueOf(){events.push(['unexpected-valueOf']);throw reason;}};
    if(mode==='reenter'&&!control.entered){control.entered=true;events.push(['inner-frozen',isFrozen(base)]);}
    if(mode==='replace-alias')Object.isFrozen=()=>{throw reason;};
    return Reflect.isExtensible(target);
   }
   if(name==='ownKeys'){
    if(mode==='ownkeys-throw')throw reason;
    if(mode==='delete-in-ownkeys')Reflect.deleteProperty(base,second);
    const keys=Reflect.ownKeys(target);
    if(mode==='hide-key')return keys.slice(1);
    return mode==='reverse'?keys.reverse():keys;
   }
   if(mode==='descriptor-throw')throw reason;
   if(mode==='delete-later'&&args[1]===first)Reflect.deleteProperty(base,second);
   if(mode==='missing-descriptor')return undefined;
   const current=Reflect.getOwnPropertyDescriptor(target,args[1]);
   if(current&&(mode==='descriptor-getters'||mode==='descriptor-getter-throw')){const wrapped={};for(const field of Reflect.ownKeys(current))Object.defineProperty(wrapped,field,{enumerable:true,configurable:true,get(){events.push([label,'descriptor-get',key(args[1]),field]);gc();if(mode==='descriptor-getter-throw'&&field==='writable')throw reason;return current[field];}});return wrapped;}
   return current;
  };
 }});return new Proxy(target,handler);}
 let proxy=make(base,mode==='nested'?'inner':'outer');if(mode==='nested')proxy=make(proxy,'outer');
 if(mode==='revoked'){const rev=Proxy.revocable(base,{});proxy=rev.proxy;rev.revoke();}
 const saved=descriptors(Object,'isFrozen');let result;try{result=attempt(()=>isFrozen(proxy),reason);}finally{Object.defineProperty(Object,'isFrozen',saved);}
 return {result,events,target:snapshot(base)};
}
const rows=fixtures.map(input=>({input,result:input.kind==='metadata'?metadata():input.kind==='primitive'?primitive(input.label):input.kind==='basic'?basic(input.shape,input.state):proxyCase(input.state,input.mode,input.array)}));
process.stdout.write(JSON.stringify(rows));
