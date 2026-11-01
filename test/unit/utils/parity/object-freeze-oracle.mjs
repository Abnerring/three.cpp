import fs from 'node:fs';
import assert from 'node:assert/strict';
assert.equal(process.version,'v26.5.0');assert.equal(typeof gc,'function');
const fixtures=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));
const freeze=Object.freeze,descriptors=Object.getOwnPropertyDescriptor;
const key=k=>typeof k==='symbol'?{$symbol:k.description??null}:k;
function encode(v){if(v===undefined)return {$type:'undefined'};if(typeof v==='symbol')return key(v);if(typeof v==='function')return {$type:'function'};if(typeof v==='object'&&v!==null)return {$type:'object'};if(typeof v==='number'){if(Number.isNaN(v))return {$number:'NaN'};if(Object.is(v,-0))return {$number:'-0'};if(!Number.isFinite(v))return {$number:v<0?'-Infinity':'Infinity'};}return v;}
function descriptor(d){if(!d)return null;const out={};for(const field of ['value','writable','get','set','enumerable','configurable'])if(Object.hasOwn(d,field))out[field]=encode(d[field]);return out;}
const snapshot=o=>({extensible:Reflect.isExtensible(o),entries:Reflect.ownKeys(o).map(k=>[key(k),descriptor(descriptors(o,k))])});
function attempt(body,reason){try{return {returned:true,same:body()};}catch(e){if(e===reason)return {thrownOriginal:true};return {error:e.name};}}
function metadata(){const d=descriptors(Object,'freeze'),fn=d.value;return {name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn),descriptor:{writable:d.writable,enumerable:d.enumerable,configurable:d.configurable},ownKeys:Reflect.ownKeys(fn).map(key),nameDescriptor:descriptor(descriptors(fn,'name')),lengthDescriptor:descriptor(descriptors(fn,'length')),construct:attempt(()=>{new fn();return false;})};}
function primitive(label){const symbol=Symbol('fixture'),values={undefined,null:null,false:false,true:true,zero:0,'negative-zero':-0,number:3,NaN:NaN,Infinity:Infinity,'negative-Infinity':-Infinity,'empty-string':'',string:'ab','astral-string':'a😀b',symbol};const v=values[label];return {same:Object.is(freeze.call({unused:1},v,{extra:true}),v),value:encode(v)};}
function basic(mode){let target;const events=[],child={value:1},symbol=Symbol('fixture');
 if(mode==='array-empty')target=[];else if(mode==='array-sparse'){target=new Array(5);target[1]=2;target[4]=5;target.extra=3;target[symbol]=4;}
 else if(mode==='function')target=()=>7;else if(mode==='string-wrapper')target=Object('a😀b');else if(mode==='number-wrapper')target=Object(3);else if(mode==='boolean-wrapper')target=Object(true);else if(mode==='symbol-wrapper')target=Object(symbol);else target=mode==='null-prototype'?Object.create(null):{};
 if(['properties','accessor','non-extensible','repeat','shallow'].includes(mode)){Object.defineProperty(target,'hidden',{value:child,writable:true,configurable:true});target.a=1;target[symbol]=2;}
 if(mode==='accessor'){Object.defineProperty(target,'access',{get(){events.push(['get',this===target]);return child;},set(v){events.push(['set',this===target,v]);},enumerable:true,configurable:true});}
 if(mode==='non-extensible')Object.preventExtensions(target);
 const result=freeze(target);if(mode==='repeat')freeze(target);
 const after=snapshot(target);const setResult=Reflect.set(target,'new',8),deleteResult=Reflect.deleteProperty(target,'a'),replaceResult=Reflect.defineProperty(target,'a',{value:9});
 if(mode==='accessor'){target.access=6;}
 if(mode==='shallow')child.value=2;
 return {same:result===target,snapshot:after,setResult,deleteResult,replaceResult,events,childMutable:child.value};
}
function proxyCase(mode,isArray,accessor=false){const base=isArray?[1,2]:{a:1,b:2},symbol=Symbol('fixture'),events=[],reason={reason:'original'},state={entered:false};base[symbol]=3;if(accessor)Object.defineProperty(base,'a',{get(){events.push(['unexpected-accessor-get']);return 1;},set(){events.push(['unexpected-accessor-set']);},enumerable:true,configurable:true});
 const first=isArray?'0':'a',second=isArray?'1':'b';let revoke;
 function make(target,label){const handler={};for(const name of ['preventExtensions','ownKeys','getOwnPropertyDescriptor','defineProperty'])Object.defineProperty(handler,name,{configurable:true,get(){events.push([label,'get',name]);gc();if(mode==='null-traps')return null;if(mode==='trap-getter-throw'&&name==='ownKeys')throw reason;return function(...args){events.push([label,'call',name,this===handler,...(name==='getOwnPropertyDescriptor'||name==='defineProperty'?[key(args[1])]:[]),...(name==='defineProperty'?[descriptor(args[2])]:[])]);gc();
 if(name==='preventExtensions'){if(mode==='prevent-false')return false;if(mode==='prevent-throw')throw reason;const ok=Reflect.preventExtensions(target);if(mode==='revoke-after-prevent')revoke();return mode==='truthy-prevent'?{valueOf(){throw reason;}}:ok;}
 if(name==='ownKeys'){if(mode==='ownkeys-throw')throw reason;if(mode==='ownkeys-delete')Reflect.deleteProperty(base,second);if(mode==='reenter'&&!state.entered){state.entered=true;freeze(base);events.push(['inner-frozen']);}if(mode==='replace-alias')Object.freeze=()=>{throw reason;};const keys=Reflect.ownKeys(target);return mode==='reverse'?keys.reverse():keys;}
 if(name==='getOwnPropertyDescriptor'){if(mode==='descriptor-throw')throw reason;if(mode==='descriptor-delete'&&args[1]===first)Reflect.deleteProperty(base,second);if(mode==='missing-descriptor')return undefined;return Reflect.getOwnPropertyDescriptor(target,args[1]);}
 if(mode==='define-throw')throw reason;if(mode==='define-false')return false;if(mode==='define-delete'&&args[1]===first)Reflect.deleteProperty(base,second);return Reflect.defineProperty(target,args[1],args[2]);};}});return new Proxy(target,handler);}
 let proxy=make(base,mode==='nested'?'inner':'outer');if(mode==='nested')proxy=make(proxy,'outer');if(mode==='revoked'||mode==='revoke-after-prevent'){const rev=Proxy.revocable(base,mode==='revoked'?{}:{preventExtensions(){events.push(['outer','call','preventExtensions',true]);gc();Reflect.preventExtensions(base);rev.revoke();return true;}});proxy=rev.proxy;revoke=rev.revoke;if(mode==='revoked')revoke();}
 let result;try{result=attempt(()=>freeze(proxy)===proxy,reason);}finally{Object.freeze=freeze;}
 return {result,events,target:snapshot(base)};
}
function typed(length){const target=new Uint8Array(length);return {result:attempt(()=>freeze(target)===target),snapshot:snapshot(target)};}
const rows=fixtures.map(input=>({input,result:input.kind==='metadata'?metadata():input.kind==='primitive'?primitive(input.label):input.kind==='basic'?basic(input.mode):input.kind==='typed'?typed(input.length):proxyCase(input.mode,input.array,input.accessor)}));
process.stdout.write(JSON.stringify(rows));
