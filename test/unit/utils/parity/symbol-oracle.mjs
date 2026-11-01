const SymbolIntrinsic=Symbol,prototype=Symbol.prototype,forIntrinsic=Symbol.for,keyForIntrinsic=Symbol.keyFor,valueIntrinsic=Symbol.prototype.valueOf;
const undefinedTag=['undefined'];
export function encode(value){
 if(value===undefined)return undefinedTag;
 if(typeof value==='number')return Number.isNaN(value)?['number','NaN']:value===Infinity?['number','Infinity']:value===-Infinity?['number','-Infinity']:Object.is(value,-0)?['number','-0']:value;
 if(typeof value==='symbol')return ['symbol',value.description===undefined?undefinedTag:value.description,keyForIntrinsic(value)===undefined?undefinedTag:keyForIntrinsic(value)];
 if(typeof value==='function')return ['function',value.name,value.length,Function.prototype.toString.call(value)];
 if(value===null||typeof value!=='object')return value;
 if(Array.isArray(value))return value.map(encode);
 const out={};for(const key of Object.keys(value))out[key]=encode(value[key]);return out;
}
function failure(error){return error instanceof Error?{error:error.name,nativeError:Error.isError(error),typeError:error instanceof TypeError}:{thrown:encode(error)};}
function attempt(callback){try{return {value:encode(callback())};}catch(error){return failure(error);}}
function decode(value,state){
 if(value&&typeof value==='object'){
  if('$undefined'in value)return undefined;
  if('$number'in value)return value.$number==='-0'?-0:Number(value.$number);
  if('$symbolUndefined'in value)return SymbolIntrinsic();
  if('$symbol'in value)return SymbolIntrinsic(value.$symbol);
  if('$registered'in value)return forIntrinsic(value.$registered);
  if('$wellKnown'in value)return SymbolIntrinsic[value.$wellKnown];
  if('$boxed'in value)return Object(SymbolIntrinsic(value.$boxed));
  if('$prototype'in value)return prototype;
  if('$inherits'in value)return Object.create(prototype);
  if('$proxy'in value)return new Proxy(Object(SymbolIntrinsic('proxy')),{});
  if('$coerce'in value)return {[SymbolIntrinsic.toPrimitive](hint){state.events.push(['coerce',hint]);if(value.reenter)state.reentered=forIntrinsic(value.$coerce);if('throw'in value)throw value.throw;return value.$coerce;}};
 }
 return value;
}
function descriptors(object){return Reflect.ownKeys(object).map(key=>{const d=Object.getOwnPropertyDescriptor(object,key);let value=d.value;if(value===SymbolIntrinsic)value='#Symbol';else if(value===prototype)value='#Symbol.prototype';return {key:encode(key),value:'value'in d?encode(value):null,writable:d.writable??null,enumerable:d.enumerable,configurable:d.configurable,get:d.get?encode(d.get):null,set:d.set?encode(d.set):null};});}
function metadata(){return {name:SymbolIntrinsic.name,length:SymbolIntrinsic.length,source:Function.prototype.toString.call(SymbolIntrinsic),constructible:attempt(()=>Reflect.construct(Object,[],SymbolIntrinsic)),constructor:descriptors(SymbolIntrinsic),prototype:descriptors(prototype),prototypeIsObjectPrototype:Object.getPrototypeOf(prototype)===Object.prototype,prototypeValue:attempt(()=>valueIntrinsic.call(prototype))};}
export function run(request){const state={events:[]};
 if(request.kind==='metadata')return metadata();
 if(request.kind==='constructor'){const args=request.omit?[]:[decode(request.value,state)];const result=attempt(()=>request.construct?Reflect.construct(SymbolIntrinsic,args):SymbolIntrinsic(...args));return {result,events:state.events};}
 if(request.kind==='method'){const receiver=decode(request.receiver,state),method=request.method;let callback;if(method==='description')callback=Object.getOwnPropertyDescriptor(prototype,'description').get;else callback=prototype[method==='toPrimitive'?SymbolIntrinsic.toPrimitive:method];return {result:attempt(()=>Reflect.apply(callback,receiver,method==='toPrimitive'?[decode(request.hint,state)]:[])),events:state.events};}
 if(request.kind==='boxing'){const receiver=decode(request.receiver,state),primitive=typeof receiver==='symbol'?receiver:valueIntrinsic.call(receiver),boxed=Object(receiver),other=Object(primitive);return {primitive:encode(primitive),sameObject:boxed===receiver,sameValue:valueIntrinsic.call(boxed)===primitive,prototype:Object.getPrototypeOf(boxed)===prototype,differentBox:other!==boxed,keys:Reflect.ownKeys(boxed).map(encode),tag:Object.prototype.toString.call(boxed),description:encode(boxed.description)};}
 if(request.kind==='registry'){const key=decode(request.key,state);let symbol;const result=attempt(()=>{symbol=forIntrinsic(key);return symbol;});return {result,repeat:typeof symbol==='symbol'?forIntrinsic(keyForIntrinsic(symbol))===symbol:false,reentered:state.reentered?state.reentered===symbol:false,events:state.events};}
 if(request.kind==='keyFor')return {result:attempt(()=>keyForIntrinsic(decode(request.value,state))),events:state.events};
 if(request.kind==='property'){
  const receiver=SymbolIntrinsic('receiver'),key=request.symbolKey?SymbolIntrinsic('property'):'symbolFixtureProperty',saved=Object.getOwnPropertyDescriptor(prototype,key),events=state.events;
  const getter=function(){events.push(['get',this===receiver,typeof this]);if(request.descriptor==='getterReentry')events.push(['nested',forIntrinsic('nested')===forIntrinsic('nested'),valueIntrinsic.call(Object(receiver))===receiver]);return 11;};
  const setter=function(value){events.push(['set',this===receiver,typeof this,value]);if(request.descriptor==='setterReentry')events.push(['nested',forIntrinsic('nested')===forIntrinsic('nested'),valueIntrinsic.call(Object(receiver))===receiver]);if(request.descriptor==='setterThrows')throw 17;};
  try{if(request.descriptor!=='absent')Object.defineProperty(prototype,key,['writable','readonly'].includes(request.descriptor)?{value:7,writable:request.descriptor==='writable',configurable:true}:{get:request.descriptor==='setterOnly'?undefined:getter,set:request.descriptor==='getterOnly'?undefined:setter,configurable:true});
   const get=attempt(()=>receiver[key]),set=attempt(()=>{receiver[key]=23;return 'assigned';}),reflect=attempt(()=>Reflect.set(prototype,key,31,receiver));return {get,set,reflect,own:Object.hasOwn(Object(receiver),key),events};
  }finally{if(saved)Object.defineProperty(prototype,key,saved);else delete prototype[key];}
 }
 if(request.kind==='weak'){const receiver=decode(request.receiver,state);return {result:attempt(()=>{if(request.operation==='map'){const map=new WeakMap();map.set(receiver,9);return [map.has(receiver),map.get(receiver),map.delete(receiver),map.has(receiver)];}if(request.operation==='ref')return new WeakRef(receiver).deref()===receiver;const registry=new FinalizationRegistry(()=>{});if(request.operation==='token'){const target={};registry.register(target,{},receiver);return [registry.unregister(receiver),registry.unregister(receiver)];}registry.register(receiver,{});return 'registered';})};}
 if(request.kind==='private'){
  const receiver=SymbolIntrinsic('private'),globalDescriptor=Object.getOwnPropertyDescriptor(globalThis,'Symbol'),constructorDescriptor=Object.getOwnPropertyDescriptor(prototype,'constructor'),forDescriptor=Object.getOwnPropertyDescriptor(SymbolIntrinsic,'for'),keyDescriptor=Object.getOwnPropertyDescriptor(SymbolIntrinsic,'keyFor');
  try{if(request.mode==='getter')Object.defineProperty(globalThis,'Symbol',{configurable:true,get(){state.events.push('global Symbol getter');throw 19;}});else if(request.mode==='data')globalThis.Symbol=function FakeSymbol(){throw 21;};else if(request.mode==='prototypeConstructor')prototype.constructor=null;else{SymbolIntrinsic.for=null;SymbolIntrinsic.keyFor=null;}
   const boxed=Object(receiver);return {samePrototype:Object.getPrototypeOf(boxed)===prototype,value:valueIntrinsic.call(boxed)===receiver,description:encode(boxed.description),registry:forIntrinsic('private-registry')===forIntrinsic('private-registry'),registryKey:keyForIntrinsic(forIntrinsic('private-registry')),events:state.events};
  }finally{Object.defineProperty(globalThis,'Symbol',globalDescriptor);Object.defineProperty(prototype,'constructor',constructorDescriptor);Object.defineProperty(SymbolIntrinsic,'for',forDescriptor);Object.defineProperty(SymbolIntrinsic,'keyFor',keyDescriptor);}
 }
 if(request.kind==='objectTag'){const symbol=SymbolIntrinsic('tag'),receiver=request.boxed?Object(symbol):symbol,key=SymbolIntrinsic.toStringTag,saved=Object.getOwnPropertyDescriptor(prototype,key);try{if(request.mode==='deleted')delete prototype[key];else if(request.mode==='customString')Object.defineProperty(prototype,key,{value:'CustomSymbol',configurable:true});else if(request.mode==='customGetter')Object.defineProperty(prototype,key,{configurable:true,get(){state.events.push(['tag',this===receiver,typeof this]);return 'ObservedSymbol';}});return {value:Object.prototype.toString.call(receiver),events:state.events};}finally{Object.defineProperty(prototype,key,saved);}}
 if(request.kind==='conversion'){const receiver=decode(request.receiver,state);return {result:attempt(()=>{if(request.operation==='String')return String(receiver);if(request.operation==='Number')return Number(receiver);if(request.operation==='Boolean')return Boolean(receiver);if(request.operation==='propertyKey')return Reflect.ownKeys({[receiver]:1})[0];return JSON.stringify(receiver);}),events:state.events};}
 if(request.kind==='identity'){const a=SymbolIntrinsic('identity'),b=SymbolIntrinsic('identity'),registered=forIntrinsic('identity'),iterator=SymbolIntrinsic.iterator;return {localDistinct:a!==b,localRegistryDistinct:a!==registered,registeredSame:registered===forIntrinsic('identity'),wellKnownRegistryDistinct:iterator!==forIntrinsic('Symbol.iterator'),wellKnownKey:encode(keyForIntrinsic(iterator)),boxedValue:valueIntrinsic.call(Object(a))===a,boxedDistinct:Object(a)!==Object(a),instanceOf:Object(a) instanceof SymbolIntrinsic};}
 if(request.kind==='symbolKeys'){const a=SymbolIntrinsic('key'),b=SymbolIntrinsic('key'),registered=forIntrinsic('key'),object={[a]:1,[b]:2,[registered]:3,plain:4};const keys=Reflect.ownKeys(object);return {distinct:a!==b&&a!==registered&&b!==registered,keys:keys.map(encode),values:keys.map(key=>object[key]),json:JSON.stringify(object),prototype:Object.getPrototypeOf(Object(a))===prototype};}
 if(request.kind==='dynamicPrimitive'){const symbol=SymbolIntrinsic('conversion'),receiver=request.boxed?Object(symbol):symbol,key=SymbolIntrinsic.toPrimitive,saved=Object.getOwnPropertyDescriptor(prototype,key);try{Object.defineProperty(prototype,key,{configurable:true,value:function(hint){state.events.push(['primitive',this===receiver,typeof this,hint]);if(request.mode==='throws')throw 17;return request.mode==='object'?{}:'converted';}});return {result:attempt(()=>request.operation==='String'?String(receiver):request.operation==='Number'?Number(receiver):SymbolIntrinsic(receiver)),events:state.events};}finally{Object.defineProperty(prototype,key,saved);}}
 throw Error('Unknown Symbol fixture '+request.kind);
}

export async function gcEvidence(){
 if(typeof gc!=='function')throw Error('Use --expose-gc');
 const tick=()=>new Promise(setImmediate),settle=async()=>{for(let i=0;i<4;i++){await tick();gc();}},rows=[];
 let {boxed,weakSymbol,weakBox}=(()=>{const symbol=SymbolIntrinsic('boxed-only'),boxed=Object(symbol);return {boxed,weakSymbol:new WeakRef(symbol),weakBox:new WeakRef(boxed)};})();
 await settle();rows.push((()=>({stage:'boxed retains primitive',symbol:!!weakSymbol.deref(),box:!!weakBox.deref(),same:valueIntrinsic.call(boxed)===weakSymbol.deref()}))());boxed=null;
 await settle();rows.push({stage:'dropped box and primitive',symbol:!!weakSymbol.deref(),box:!!weakBox.deref()});
 let registered=forIntrinsic('gc-registry');const registeredBox=new WeakRef(Object(registered));registered=null;await settle();rows.push({stage:'registry retains symbol, not box',key:keyForIntrinsic(forIntrinsic('gc-registry')),box:!!registeredBox.deref()});
 let map=new WeakMap();const {weakKey,weakPayload}=(()=>{const key=SymbolIntrinsic('weak-key'),payload={};map.set(key,payload);return {weakKey:new WeakRef(key),weakPayload:new WeakRef(payload)};})();
 await settle();rows.push({stage:'weak symbol key and conditional value',key:!!weakKey.deref(),payload:!!weakPayload.deref()});map=null;
 return rows;
}
