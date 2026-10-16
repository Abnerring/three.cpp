import {pathToFileURL} from 'node:url';
import {resolve} from 'node:path';
import {execFileSync} from 'node:child_process';
import {readFileSync} from 'node:fs';
import {createHash} from 'node:crypto';
const checkout=process.argv[2];
if(execFileSync('git',['-C',checkout,'rev-parse','HEAD'],{encoding:'utf8'}).trim()!=='11738a9d55e502fb484cefffcea3a66dc44e9fe9')throw Error('Pinned checkout mismatch');
if(createHash('sha256').update(readFileSync(resolve(checkout,'src/materials/Material.js'))).digest('hex')!=="360ab342743d8d23600a7caff979d695c96944f11ec933def9c2875c7cf147be")throw Error('Pinned Material source bytes/line endings mismatch');
const {MeshBasicMaterial}=await import(pathToFileURL(resolve(checkout,'src/materials/MeshBasicMaterial.js')));
const {MeshStandardMaterial}=await import(pathToFileURL(resolve(checkout,'src/materials/MeshStandardMaterial.js')));
const basic=new MeshBasicMaterial(),standard=new MeshStandardMaterial(),callback=basic.onBeforeCompile,keyMethod=basic.customProgramCacheKey;
const result={callbackName:callback.name,callbackLength:callback.length,callbackConstructible:false,callbackSource:callback.toString(),keyMethodName:keyMethod.name,keyMethodSource:keyMethod.toString(),defaultKey:keyMethod.call(basic),sameDefaultCallback:callback===standard.onBeforeCompile,sameKeyMethod:keyMethod===standard.customProgramCacheKey};
try{Reflect.construct(String,[],callback);result.callbackConstructible=true;}catch{}
const dynamicLog=[],replacement=function(){};
Object.defineProperty(replacement,'toString',{configurable:true,get(){dynamicLog.push('get-toString');return function(...args){dynamicLog.push('call-toString',this===replacement,args.length);return 'custom-source';};}});
basic.onBeforeCompile=replacement;result.dynamicKey=keyMethod.call(basic);result.dynamicLog=dynamicLog;
result.keyConstructible=false;try{Reflect.construct(String,[],keyMethod);result.keyConstructible=true;}catch{}
result.callbackHasOwnToString=Object.hasOwn(callback,'toString');result.keyHasOwnToString=Object.hasOwn(keyMethod,'toString');
result.callbackHasOwnPrototype=Object.hasOwn(callback,'prototype');result.keyHasOwnPrototype=Object.hasOwn(keyMethod,'prototype');
const descriptor=(fn,name)=>{const d=Object.getOwnPropertyDescriptor(fn,name);return [d.writable,d.enumerable,d.configurable];};
result.callbackNameDescriptor=descriptor(callback,'name');result.keyNameDescriptor=descriptor(keyMethod,'name');
basic.onBeforeCompile=callback;const arbitrary={},prototype=Object.getPrototypeOf(callback),old=Object.getOwnPropertyDescriptor(prototype,'toString'),getterLog=[];
Object.defineProperty(prototype,'toString',{configurable:true,get(){getterLog.push('get-inherited-toString',this===callback);return function(...args){getterLog.push('call-inherited-toString',this===callback,args.length);return arbitrary;};}});
result.inheritedArbitraryIdentity=keyMethod.call(basic)===arbitrary;result.inheritedLog=getterLog;
Object.defineProperty(prototype,'toString',old);result.restoredDefaultKey=keyMethod.call(basic);
const throwLog=[];Object.defineProperty(replacement,'toString',{configurable:true,get(){throwLog.push('throw-getter');throw new TypeError('source-getter');}});basic.onBeforeCompile=replacement;
try{keyMethod.call(basic);result.throwName='none';}catch(e){result.throwName=e.name;}result.throwLog=throwLog;
const reentryLog=[];Object.defineProperty(replacement,'toString',{configurable:true,get(){basic.onBeforeCompile=callback;const snapshot=this;return function(){reentryLog.push(this===snapshot);return 'snapshot-source';};}});
result.reentryKey=keyMethod.call(basic);result.reentryLog=reentryLog;result.reentryNextKey=keyMethod.call(basic);
console.log(JSON.stringify(result));
if(process.argv[3]){const native=JSON.parse(execFileSync(process.argv[3],[],{encoding:'utf8'}));let checks=0;for(const [key,value] of Object.entries(result)){if(JSON.stringify(native[key])!==JSON.stringify(value))throw Error(key+': '+JSON.stringify({actual:native[key],expected:value}));checks++;}console.log('1 actual pinned Material scenario / '+checks+' field comparisons passed');}
