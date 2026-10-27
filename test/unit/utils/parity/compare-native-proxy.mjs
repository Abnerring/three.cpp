import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
const binary=process.argv[2]&&process.argv[2]!=='--oracle'?path.resolve(process.argv[2]):undefined;
const outputDirectory=process.argv[3]&&path.resolve(process.argv[3]);
function normal(v,s){if(v===undefined)return '#undefined';if(typeof v==='symbol')return '#symbol';if(Object.is(v,-0))return '#-0';for(const key of ['proxy','target','handler','receiver','proto','otherProto','newTarget'])if(v!==null&&(typeof v==='object'||typeof v==='function')&&v===s[key])return '@'+key;if(typeof v==='function')return '#function';if(v===null||typeof v!=='object')return v;if(Array.isArray(v))return v.map(x=>normal(x,s));const out={};for(const key of Object.keys(v))Object.defineProperty(out,key,{value:normal(v[key],s),writable:true,enumerable:true,configurable:true});return out;}
function decode(v,s){if(!v||typeof v!=='object')return v;if(v.tag==='undefined')return undefined;if(v.tag==='negativeZero')return -0;if(v.tag==='symbol')return s.symbol;if(['target','proto','otherProto'].includes(v.tag))return s[v.tag];if(v.tag==='getter')return ()=>19;if(v.tag==='badKeys')return ['fixed','fixed',3];return v;}
function descriptor(v,s){const out={};for(const key of ['value','writable','get','set','enumerable','configurable'])if(Object.hasOwn(v,key))out[key]=decode(v[key],s);return out;}
function descriptorObject(d,s){if(!d)return '#undefined';const out={};for(const key of ['value','writable','get','set','enumerable','configurable'])if(Object.hasOwn(d,key))out[key]=normal(d[key],s);return out;}
function run(r){
    if(r.kind==='typedArrayLookup')return typedArrayLookup(r);
    if(r.kind==='ordinarySetPrototype')return ordinarySetPrototype(r);
    if(r.kind==='arrayMethods')return arrayMethods(r);
    const s={r,events:[],symbol:Symbol('key'),proto:{},otherProto:{},receiver:{}};let target=Object.create(s.proto);
    if(r.target==='array'){target=[1];Object.setPrototypeOf(target,s.proto);}
    if(r.target==='function')target=function Target(...args){s.events.push(['targetCall',new.target!==undefined,normal(new.target?undefined:this,s),normal(new.target,s),args]);return new.target?{sum:args[0]+args[1]}:args[0]+args[1];};
    s.target=target;s.newTarget=function NewTarget(){};
    Object.defineProperty(target,'fixed',{value:r.negativeZero?-0:7,writable:!!r.writable,enumerable:true,configurable:!!r.configurable});Object.assign(target,{loose:11,receiver:19});target[s.symbol]=13;
    Object.defineProperty(target,'accessor',{get:r.emptyGetter?undefined:function(){s.events.push(['targetGet',normal(this,s)]);return this.receiver;},set:r.emptySetter?undefined:function(value){s.events.push(['targetSet',normal(this,s),normal(value,s)]);},enumerable:true,configurable:!!r.configurable});
    if(!r.extensible)Object.preventExtensions(target);const handler=s.handler={};
    function trap(...args){const name=r.trap;s.events.push(['trap',name,this===handler,...args.map(x=>normal(x,s))]);
        if(r.mode==='throw')throw new TypeError('trap');
        if(r.mode==='reenter'&&!s.reentered){s.reentered=true;s.events.push(['nested',normal(s.proxy.loose,s)]);}
        if(r.mode==='revokeInTrap')s.revoke();
        if(r.mutateTarget){if(name==='preventExtensions')Object.preventExtensions(target);else if(name==='defineProperty')Reflect.defineProperty(target,args[1],args[2]);else if(name==='deleteProperty')Reflect.deleteProperty(target,args[1]);}
        if(r.mode==='forward')return Reflect[name](...args);
        if(r.mode==='keysObject'){const keys={length:3};for(let i=0;i<3;i++)Object.defineProperty(keys,i,{enumerable:true,configurable:true,get(){s.events.push(['key',i]);if(i===2&&r.lastKeyThrows)throw new TypeError('last key');return i===2?'loose':'fixed';}});return keys;}
        if(r.mode==='descriptorObject'){const result={};for(const key of ['enumerable','configurable','value','writable','get','set'])if(Object.hasOwn(r.result,key))Object.defineProperty(result,key,{enumerable:true,configurable:true,get(){s.events.push(['descriptorGet',key]);return decode(r.result[key],s);}});return result;}
        return decode(r.result,s);
    }
    if(r.mode!=='absent'){const method=r.mode==='noncallable'?3:r.mode==='null'?null:trap;if(r.getter)Object.defineProperty(handler,r.trap,{configurable:true,get(){s.events.push(['trapGet',r.trap,this===handler]);if(r.getterThrow)throw new TypeError('get trap');if(r.revokeGetter)s.revoke();return method;}});else handler[r.trap]=method;}
    let revocable=Proxy.revocable(target,handler);s.proxy=revocable.proxy;s.revoke=revocable.revoke;
    if(r.nested){revocable=Proxy.revocable(s.proxy,{});s.proxy=revocable.proxy;s.revoke=revocable.revoke;}
    if(r.revoked)s.revoke();const proxy=s.proxy,key=r.symbol?s.symbol:r.key??'fixed',value=decode(Object.hasOwn(r,'value')?r.value:7,s);let result;
    try{
        if(r.operation==='get')result=normal(Reflect.get(proxy,key,r.receiver?s.receiver:proxy),s);
        if(r.operation==='set')result=Reflect.set(proxy,key,value,r.receiver?s.receiver:proxy);
        if(r.operation==='setStrict'){proxy[key]=value;result=true;}
        if(r.operation==='has')result=key in proxy;
        if(r.operation==='deleteProperty')result=Reflect.deleteProperty(proxy,key);
        if(r.operation==='getOwnPropertyDescriptor')result=descriptorObject(Object.getOwnPropertyDescriptor(proxy,key),s);
        if(r.operation==='defineProperty')result=Reflect.defineProperty(proxy,key,descriptor(r.descriptor,s));
        if(r.operation==='ownKeys')result=Reflect.ownKeys(proxy).map(x=>normal(x,s));
        if(r.operation==='getPrototypeOf')result=normal(Object.getPrototypeOf(proxy),s);
        if(r.operation==='setPrototypeOf')result=Reflect.setPrototypeOf(proxy,r.sameProto?s.proto:s.otherProto);
        if(r.operation==='isExtensible')result=Object.isExtensible(proxy);
        if(r.operation==='preventExtensions'){Object.preventExtensions(proxy);result=true;}
        if(r.operation==='reflectPreventExtensions')result=Reflect.preventExtensions(proxy);
        if(r.operation==='apply')result=normal(Reflect.apply(proxy,s.receiver,[2,3]),s);
        if(r.operation==='construct')result=normal(Reflect.construct(proxy,[2,3],r.newTarget?s.newTarget:proxy),s);
        if(r.operation==='isArray')result=Array.isArray(proxy);
    }catch(e){result='#'+e.name;}
    return {result,events:s.events,targetLoose:normal(target.loose,s),targetFixed:normal(target.fixed,s),targetExtensible:Object.isExtensible(target),receiverFixed:normal(s.receiver.fixed,s)};
}
function arrayMethods(r){
    const s={r,events:[],receiver:{},proto:{}};const target=s.target=r.shape==='array'?new Array(4):Object.create(s.proto);target[0]=1;target[2]=3;
    if(!Array.isArray(target))Object.defineProperty(target,'length',{configurable:true,get(){s.events.push(['length']);if(r.throwLength)throw new TypeError('length');return Object.hasOwn(r,'length')?r.length:4;}});
    if(r.inherit){s.proto[1]=2;if(Array.isArray(target))Object.setPrototypeOf(target,s.proto);}
    if(r.readonly)Object.defineProperty(target,'2',{value:3,writable:false,enumerable:true,configurable:true});if(r.empty){delete target[0];delete target[2];}
    let actual=target;if(r.shape==='string')actual='abc';if(r.shape==='null')actual=null;if(r.shape==='undefined')actual=undefined;if(r.shape==='number')actual=3;
    const method=Array.prototype[r.method],callback=function(...args){const index=args[r.method==='reduce'?2:1];s.events.push(['callback',normal(this,s),...args.map(x=>normal(x,s))]);if(r.throwCallback)throw new TypeError('callback');
        if(r.mutate){delete target[2];target[3]=8;if(Array.isArray(target))target.length=6;target[4]=99;}
        return r.method==='reduce'?args[0]+args[1]:!(Object.hasOwn(r,'stop')&&index===r.stop);
    };
    let args;if(r.method==='fill'){
        const bound=key=>!r.coerce?decode(r[key],s):{[Symbol.toPrimitive](hint){s.events.push(['bound',key,hint]);if(r.throwBound===key)throw new TypeError('bound');return decode(r[key],s);}};
        args=[9,bound('start'),bound('end')];
    }else{args=[r.noncallable?7:callback];if(r.method==='every')args.push(s.receiver);else if(r.initial)args.push(10);}
    let result;try{result=normal(method.apply(actual,args),s);}catch(e){result='#'+e.name;}
    return {result,events:s.events,fields:Object.getOwnPropertyNames(target).filter(key=>key!=='length').map(key=>{const d=Object.getOwnPropertyDescriptor(target,key);return [key,normal(target[key],s),!!d.writable,!!d.enumerable,!!d.configurable];}),arity:method.length};
}
function ordinarySetPrototype(r){const out={},events=[],target=Object.create(null);if(r.scenario==='ordinaryCycle'){const child=Object.create(target);out.result=Reflect.setPrototypeOf(target,child);out.selfCycle=Reflect.setPrototypeOf(target,target);out.parentUnchanged=Object.getPrototypeOf(target)===null;out.childBacklink=Object.getPrototypeOf(child)===target;out.cleanup=Reflect.setPrototypeOf(child,null);}else{const protoTarget=Object.create(r.scenario==='proxyCycle'?target:null),handler={};if(r.scenario==='proxyTrapThrows')handler.getPrototypeOf=function(){events.push(['getPrototypeOf']);throw new TypeError('prototype trap must not be called');};const proxy=new Proxy(protoTarget,handler);try{out.result=Reflect.setPrototypeOf(target,proxy);}catch(e){out.result='#'+e.name;}out.linked=Object.getPrototypeOf(target)===proxy;out.cleanup=Reflect.setPrototypeOf(target,null);if(r.scenario==='proxyCycle'){out.targetBacklink=Object.getPrototypeOf(protoTarget)===target;out.proxyBacklink=Object.getPrototypeOf(proxy)===target;}}out.events=events;return out;}
function typedArrayLookup(r){
    const s={events:[]},proto=Object.create(null),constructors=[Int8Array,Uint8Array,Int16Array,Uint16Array,Int32Array,Uint32Array,Float32Array,Float64Array,Uint8ClampedArray];
    const target=s.target=r.mode==='spoof'?{isTypedArray:true,type:'Uint8Array'}:new constructors[r.arrayKind]([17]);Object.setPrototypeOf(target,proto);
    const keys=['0','1','2','-0','NaN','Infinity','-Infinity','-1','0.5','9007199254740992','1e+21','1e-7','01','+0','1.0','1e0','1e21','0.0000001',' 1 ','','foo'];
    for(const key of keys)Object.defineProperty(proto,key,{enumerable:true,configurable:true,get(){s.events.push(['inheritedGet',key,normal(this,s)]);return 'inherited:'+key;}});
    let actual=target;
    if(r.mode==='proxy'||r.mode==='forward'){
        const handler=r.mode==='forward'?{get(target,key,receiver){s.events.push(['forwardGet',key,normal(receiver,s)]);return Reflect.get(target,key,receiver);},has(target,key){s.events.push(['forwardHas',key]);return Reflect.has(target,key);}}:{};
        actual=s.proxy=new Proxy(target,handler);
    }else if(r.mode==='child')actual=s.receiver=Object.create(target);
    const rows=keys.map(key=>{let value;try{value=normal(Reflect.get(actual,key),s);}catch(e){value='#'+e.name;}return [key,value,key in actual,Object.hasOwn(actual,key)];});
    return {rows,events:s.events};
}
const cases=[];
const operations=['get','set','has','deleteProperty','getOwnPropertyDescriptor','defineProperty','ownKeys','getPrototypeOf','setPrototypeOf','isExtensible','preventExtensions','apply','construct'];
for(const operation of operations)for(const mode of ['absent','null','forward','noncallable','throw'])for(const getter of [false,true])cases.push({operation,trap:operation,mode,getter,target:operation==='apply'||operation==='construct'?'function':'plain',extensible:true,configurable:true,writable:true,descriptor:{value:8,writable:true,configurable:true,enumerable:true}});
for(const operation of ['get','set','has','deleteProperty'])for(const configurable of [false,true])for(const extensible of [false,true])for(const writable of [false,true])for(const result of [undefined,false,true,7,8])cases.push({operation,trap:operation,mode:'return',result:result===undefined?{tag:'undefined'}:result,configurable,extensible,writable});
for(const operation of ['get','set'])for(const receiver of [false,true])for(const mode of ['absent','forward','return'])for(const key of ['accessor','loose'])cases.push({operation,trap:operation,mode,receiver,key,result:17,value:23,extensible:true,configurable:true,writable:true});
for(const operation of ['get','set'])for(const emptyGetter of [false,true])for(const emptySetter of [false,true])cases.push({operation,trap:operation,key:'accessor',mode:'return',emptyGetter,emptySetter,result:17,extensible:true});
for(const operation of ['get','set','has','deleteProperty','getOwnPropertyDescriptor'])cases.push({operation,trap:operation,mode:'forward',symbol:true,extensible:true});
for(const result of [undefined,null,3,{value:7},{value:7,writable:false,configurable:false,enumerable:true},{value:7,writable:true,configurable:true,enumerable:true},{get:{tag:'getter'},configurable:true},{value:7,get:{tag:'getter'}}])for(const configurable of [false,true])for(const extensible of [false,true])cases.push({operation:'getOwnPropertyDescriptor',trap:'getOwnPropertyDescriptor',mode:'return',result:result===undefined?{tag:'undefined'}:result,configurable,extensible});
for(const result of [{value:7,writable:false,enumerable:true,configurable:false},{value:8,writable:true,enumerable:true,configurable:true},{get:{tag:'undefined'},set:{tag:'undefined'},enumerable:true,configurable:true}])cases.push({operation:'getOwnPropertyDescriptor',trap:'getOwnPropertyDescriptor',mode:'descriptorObject',result,extensible:true});
for(const descriptor of [{},{value:8},{writable:false},{configurable:false},{value:7,writable:false,configurable:false,enumerable:true},{get:{tag:'undefined'}}])for(const key of ['fixed','missing'])for(const extensible of [false,true])for(const configurable of [false,true])cases.push({operation:'defineProperty',trap:'defineProperty',mode:'return',result:true,descriptor,key,extensible,configurable,writable:true});
for(const result of [[],['fixed'],['fixed','loose','receiver','accessor'],['fixed','fixed'],[3],{tag:'badKeys'},null,3])for(const extensible of [false,true])for(const configurable of [false,true])cases.push({operation:'ownKeys',trap:'ownKeys',mode:'return',result,extensible,configurable});
for(const lastKeyThrows of [false,true])cases.push({operation:'ownKeys',trap:'ownKeys',mode:'keysObject',extensible:true,lastKeyThrows});
for(const operation of ['getPrototypeOf','setPrototypeOf','isExtensible','preventExtensions'])for(const extensible of [false,true])for(const result of [false,true,null,{tag:'proto'},{tag:'otherProto'},3])cases.push({operation,trap:operation,mode:'return',extensible,result,sameProto:false});
for(const operation of ['get','set','ownKeys','apply','construct','getPrototypeOf','isArray'])for(const target of ['plain','array','function'])cases.push({operation,trap:operation,mode:'absent',target,extensible:true,revoked:true});
for(const operation of ['get','set','apply','construct'])for(const getterThrow of [false,true])for(const revokeGetter of [false,true])cases.push({operation,trap:operation,mode:'return',getter:true,getterThrow,revokeGetter,result:7,target:operation==='apply'||operation==='construct'?'function':'plain',extensible:true});
for(const operation of ['get','set','has','getOwnPropertyDescriptor','ownKeys','isArray'])cases.push({operation,trap:operation,mode:'absent',nested:true,target:'array',extensible:true});
for(const target of ['array','plain','function'])cases.push({operation:'isArray',trap:'get',mode:'throw',target,extensible:true});
for(const value of [0,{tag:'negativeZero'}])for(const result of [0,{tag:'negativeZero'}])cases.push({operation:'get',trap:'get',mode:'return',negativeZero:true,value,result,extensible:true});
for(const operation of ['get','has','getOwnPropertyDescriptor'])cases.push({operation,trap:operation,mode:'reenter',result:operation==='getOwnPropertyDescriptor'?{value:7,configurable:true,enumerable:true}:7,configurable:true,extensible:true});
for(const operation of ['apply','construct'])for(const newTarget of [false,true])for(const mode of ['forward','return'])cases.push({operation,trap:operation,mode,result:{sum:27},newTarget,target:'function',extensible:true});
const proxyCases=cases.length;
for(const method of ['reduce','every'])for(const shape of ['array','object','string','null','undefined','number'])for(const initial of [false,true])for(const inherit of [false,true])cases.push({kind:'arrayMethods',method,shape,initial,inherit});
for(const method of ['reduce','every'])for(const empty of [false,true])for(const inherit of [false,true])for(const initial of [false,true])cases.push({kind:'arrayMethods',method,shape:'array',empty,inherit,initial});
for(const method of ['reduce','every'])for(const mutate of [false,true])for(const throwCallback of [false,true])for(const stop of [0,1,2])cases.push({kind:'arrayMethods',method,shape:'array',initial:true,inherit:true,mutate,throwCallback,stop});
for(const method of ['reduce','every'])for(const throwLength of [false,true])for(const noncallable of [false,true])for(const length of [0,-1,2.5,'3',null])cases.push({kind:'arrayMethods',method,shape:'object',throwLength,noncallable,length});
for(const shape of ['array','object','string','null','undefined','number'])for(const start of [undefined,-3,0,1,5])for(const end of [undefined,-1,2,7])cases.push({kind:'arrayMethods',method:'fill',shape,start,end});
for(const readonly of [false,true])for(const start of [-2,0,2])for(const end of [undefined,2,4])cases.push({kind:'arrayMethods',method:'fill',shape:'array',readonly,start,end});
for(const start of [undefined,-1,0,2])for(const end of [undefined,-1,3])for(const throwBound of [undefined,'start','end'])cases.push({kind:'arrayMethods',method:'fill',shape:'object',coerce:true,start,end,throwBound});
const arrayCases=cases.length-proxyCases,preventExtensionsStart=cases.length;
for(const nested of [false,true])for(const extensible of [false,true])for(const result of [false,true])for(const getter of [false,true])cases.push({operation:'reflectPreventExtensions',trap:'preventExtensions',mode:'return',nested,extensible,result,getter});
for(const nested of [false,true])for(const mode of ['absent','null','forward'])cases.push({operation:'reflectPreventExtensions',trap:'preventExtensions',mode,nested,extensible:true,getter:true});
for(const nested of [false,true])for(const getterThrow of [false,true])cases.push({operation:'reflectPreventExtensions',trap:'preventExtensions',mode:'throw',nested,extensible:true,getter:true,getterThrow});
for(const nested of [false,true])cases.push({operation:'reflectPreventExtensions',trap:'preventExtensions',mode:'return',nested,extensible:true,result:true,mutateTarget:true});
const preventExtensionsCases=cases.length-preventExtensionsStart;
assert.equal(cases.length,883,'Preserve the original Proxy/Array protocol prefix');
for(const scenario of ['proxyTrapThrows','ordinaryCycle','proxyCycle'])cases.push({kind:'ordinarySetPrototype',scenario});
assert.equal(cases.length,886,'Preserve the OrdinarySetPrototypeOf regression prefix');
for(let arrayKind=0;arrayKind<9;arrayKind++)for(const mode of ['raw','proxy','forward','child'])cases.push({kind:'typedArrayLookup',arrayKind,mode});
cases.push({kind:'typedArrayLookup',arrayKind:1,mode:'spoof'});
const expected=cases.map(run);
if(outputDirectory){fs.mkdirSync(outputDirectory,{recursive:true});fs.writeFileSync(path.join(outputDirectory,'input.json'),JSON.stringify(cases));fs.writeFileSync(path.join(outputDirectory,'expected.json'),JSON.stringify(expected,null,2));}
if(!binary){console.log(`Prepared ${cases.length} native Proxy oracle scenarios using ${process.version}`);process.exit(0);}
const actual=JSON.parse(execFileSync(binary,[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:128*1024*1024}));
if(outputDirectory)fs.writeFileSync(path.join(outputDirectory,'actual.json'),JSON.stringify(actual,null,2));
assert.equal(actual.length,expected.length);const differences=[];for(let i=0;i<cases.length;i++)try{assert.deepEqual(actual[i],expected[i]);}catch{differences.push({index:i,input:cases[i],expected:expected[i],actual:actual[i]});}
if(outputDirectory)fs.writeFileSync(path.join(outputDirectory,'differences.json'),JSON.stringify(differences,null,2));
assert.equal(differences.length,0,JSON.stringify(differences.slice(0,4),null,2));
const gc=JSON.parse(execFileSync(binary,['gc'],{encoding:'utf8'}));assert.deepEqual(gc,[[true,true,true],[false,false,false]]);if(outputDirectory)fs.writeFileSync(path.join(outputDirectory,'native-gc.json'),JSON.stringify(gc));
console.log(`Native Proxy: ${proxyCases+preventExtensionsCases} real ECMAScript scenarios; Array reduce/every/fill: ${arrayCases} real ECMAScript scenarios; OrdinarySetPrototypeOf: 3 real ECMAScript scenarios; IntegerIndexed Get/Has: 37 real ECMAScript scenarios; + 1 native GC snapshot passed`);
