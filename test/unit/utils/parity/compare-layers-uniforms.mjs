import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import { execFileSync } from 'node:child_process';
const upstream=process.argv[2],probe=process.argv[3];
if(!upstream||!probe)throw Error('node compare-layers-uniforms.mjs <pinned-three.js> <probe>');
const commit=execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim();
assert.equal(commit,'11738a9d55e502fb484cefffcea3a66dc44e9fe9');
const {Layers}=await import(pathToFileURL(path.join(upstream,'src/core/Layers.js')));
const {Object3D}=await import(pathToFileURL(path.join(upstream,'src/core/Object3D.js')));
const {UniformsUtils,cloneUniforms,mergeUniforms,cloneUniformsGroups,getUnlitUniformColorSpace}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/UniformsUtils.js')));
const {setConsoleFunction}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
const {ColorManagement}=await import(pathToFileURL(path.join(upstream,'src/math/ColorManagement.js')));
const encode=v=>v===undefined?'@undefined':typeof v==='symbol'?'@symbol':typeof v==='function'?'@function':typeof v==='number'?(Number.isNaN(v)?'@NaN':v===Infinity?'@Infinity':v===-Infinity?'@-Infinity':Object.is(v,-0)?'@-0':v):Array.isArray(v)?Array.from(v,encode):v&&typeof v==='object'?(Object.hasOwn(v,'tag')?{tag:encode(v.tag)}:{keys:[]}):v;
const desc=d=>{const o={enumerable:d.enumerable,configurable:d.configurable};if('value'in d){o.value=encode(d.value);o.writable=d.writable;}else {o.get=typeof d.get==='function';o.set=typeof d.set==='function';}return o;};
const snapshot=v=>v!==null&&(typeof v==='object'||typeof v==='function')?Object.fromEntries(Object.getOwnPropertyNames(v).map(k=>[k,desc(Object.getOwnPropertyDescriptor(v,k))])):{};
const attempt=fn=>{try{fn();return '';}catch(e){return e.name;}};
const value=i=>[undefined,null,false,true,0,-0,1,-1,31,32,33,-33,4294967295,4294967296,NaN,Infinity,-Infinity,' 2.9 ','invalid',[],[1],{},Symbol('k'),1.9][i];
const methods=['set','enable','enableAll','toggle','disable','disableAll','test','isEnabled'];
function metadata(){const m={};for(const name of methods){const fn=Layers.prototype[name];m[name]={name:encode(fn.name),length:encode(fn.length),own:snapshot(fn),desc:desc(Object.getOwnPropertyDescriptor(Layers.prototype,name)),new:attempt(()=>Reflect.construct(fn,[]))};}return {kind:'metadata',ctor:snapshot(Layers),prototype:Object.getOwnPropertyNames(Layers.prototype),methods:m,instance:snapshot(new Layers()),call:attempt(()=>Layers())};}
function construct(mode){const events=[],prototype={};function Other(){}Other.prototype=mode===4?7:prototype;if([1,2,5].includes(mode))Object.defineProperty(prototype,'mask',{enumerable:true,configurable:true,get(){return 12;},set(v){events.push(['setter',v,Object.getPrototypeOf(this)===prototype]);if(mode===2)throw RangeError('setter');if(mode===5)Object.defineProperty(this,'tag',{value:'changed',writable:true,enumerable:true,configurable:true});}});if(mode===3)Object.defineProperty(prototype,'mask',{value:12,writable:false,enumerable:true,configurable:true});let result;const error=attempt(()=>result=Reflect.construct(Layers,[],Other));return {kind:'construct',mode,error,result:snapshot(result),samePrototype:result!==undefined&&Object.getPrototypeOf(result)===prototype,events:encode(events)};}
function numeric(method,m,n){const self=new Layers();self.mask=value(m);let arg=value(n);if(method==='test')arg={mask:arg};let result;const error=attempt(()=>result=Layers.prototype[method].call(self,arg));return {kind:'numeric',method,m,n,error,mask:encode(self.mask),result:encode(result)};}
function dynamic(method,left,argMode){const self={},other={},events=[],box={tag:'mask-conversion',valueOf(){events.push('mask.valueOf');if(left===7)throw RangeError('mask conversion');return 5;}};Object.defineProperty(self,'mask',{enumerable:true,configurable:true,get(){events.push('mask.get');if(left===1)throw RangeError('mask getter');if(left===2)other.mask=1;if(left===3)Object.setPrototypeOf(self,{});if(left===4)delete self.mask;if(left===5)Layers.prototype.enableAll.call(self);return left>=6?box:5;},set(v){events.push(['mask.set',v]);if(left===5)throw RangeError('setter');}});const arg={tag:'arg',valueOf(){events.push('arg.valueOf');if(argMode===1)throw RangeError('arg conversion');if(argMode===2)Object.defineProperty(self,'mask',{value:99,writable:true,enumerable:true,configurable:true});if(argMode===3)Layers.prototype.disableAll.call(self);if(argMode===4)return {};if(argMode===5)return Symbol('x');return 2;}};other.mask=arg;if(argMode===6)Object.defineProperty(other,'mask',{enumerable:true,configurable:true,get(){events.push('other.mask.get');throw RangeError('right getter');}});let result;const error=attempt(()=>result=Layers.prototype[method].call(self,method==='test'?other:arg));return {kind:'dynamic',method,left,arg:argMode,error,result:encode(result),own:snapshot(self),events:encode(events)};}
function native(mode){const owner=new Object3D(),original=owner.layers,other=new Layers();other.mask=5;if(mode===0)owner.layers.mask=-1;if(mode===1)original.mask='9';if(mode===2||mode===3)owner.layers=other;if(mode===4){owner.layers=other;original.mask=3;}if(mode===5)owner.layers=null;if(mode===6)owner.layers=7;if(mode===7)owner.layers={mask:'3'};if(mode===8){owner.layers.enable(31);owner.layers.enable(0);}let mask;const error=attempt(()=>mask=owner.layers.mask);return {kind:'native',mode,error,mask:encode(mask),original:encode(original.mask),sameOriginal:owner.layers===original,sameOther:owner.layers===other};}
function uniformsMetadata(){return {kind:'uniforms-metadata',legacy:snapshot(UniformsUtils),keys:Object.getOwnPropertyNames(UniformsUtils),cloneIdentity:UniformsUtils.clone===cloneUniforms,mergeIdentity:UniformsUtils.merge===mergeUniforms,functions:Object.fromEntries(Object.entries({cloneUniforms,mergeUniforms,cloneUniformsGroups,getUnlitUniformColorSpace}).map(([n,f])=>[n,snapshot(f)]))};}
function uniforms(mode){const old=UniformsUtils.clone,uniform={value:7},source={u:uniform},events=[];let fn=cloneUniforms,result;if(mode===1)fn=mergeUniforms;if(mode===2)fn=cloneUniformsGroups;if(mode===3||mode===4)fn=getUnlitUniformColorSpace;if(mode===5||mode===6){UniformsUtils.clone=function replacement(...args){events.push(['replacement',this===UniformsUtils,args.length]);return 'replace';};fn=mode===5?UniformsUtils.clone:cloneUniforms;}if(mode>=7){uniform.value={isTexture:true,isRenderTargetTexture:true};setConsoleFunction((...args)=>{events.push(...args);if(mode===8)throw RangeError('warning');if(mode===9)uniform.extra=9;});}let input=source;if(mode===1)input=[source];if(mode===2)input=[{clone(){return 19;}}];if(mode===3||mode===4)input={getRenderTarget(){return null;},outputColorSpace:'test-space'};const error=attempt(()=>result=mode===4?Reflect.construct(fn,[input]):fn.call(UniformsUtils,input));setConsoleFunction(null);UniformsUtils.clone=old;return {kind:'uniforms',mode,error,result:result&&typeof result==='object'&&'u'in result?encode(result.u.value):encode(result),events:encode(events)};}
function borrow(method,mode){const self=value(mode);let result;const error=attempt(()=>result=Layers.prototype[method].call(self,method==='test'?{}:2));return {kind:'borrow',method,mode,error,result:encode(result)};}
function namespaceCall(mode){const legacy=UniformsUtils,events=[],source={u:{value:9}},key=mode<2?'clone':'merge',original=Object.getOwnPropertyDescriptor(legacy,key);Object.defineProperty(legacy,key,{enumerable:true,configurable:true,get(){events.push(['getter',this===legacy]);if(mode===1)throw RangeError('getter');if(mode===2)legacy.clone=()=>{throw TypeError('replacement must not be called');};return mode<2?cloneUniforms:mergeUniforms;}});let result;const error=attempt(()=>result=legacy[key](mode<2?source:[source]));Object.defineProperty(legacy,key,original);legacy.clone=cloneUniforms;return {kind:'namespace',mode,error,result:result&&typeof result==='object'?encode(result.u.value):encode(result),events:encode(events)};}
function dispatch(mode){const left=new Object3D(),right=new Object3D();left.layers.mask=2;right.layers.mask=1;const original=left.layers,events=[],replacement=new Layers();replacement.mask=2;Object.defineProperty(original,'test',{enumerable:true,configurable:true,get(){events.push('test.get');right.layers=replacement;if(mode===1)left.layers=new Layers();if(mode===2)throw RangeError('getter');return Layers.prototype.test;}});let result;const error=attempt(()=>result=left.layers.test(right.layers));return {kind:'dispatch',mode,error,result:encode(result),leftMask:encode(left.layers.mask),rightMask:encode(right.layers.mask),events:encode(events)};}
function copyReentry(mode){const source=new Object3D(),destination=new Object3D(),oldTarget=destination.layers,replacement=new Layers(),events=[];replacement.mask=5;Object.defineProperty(source.layers,'mask',{enumerable:true,configurable:true,get(){events.push('source.mask.get');if(mode>0)destination.layers=mode===2?null:replacement;if(mode===3)throw RangeError('mask getter');return 19;}});const error=attempt(()=>destination.copy(source,false));let current;const read=attempt(()=>current=destination.layers.mask);return {kind:'copy-reentry',mode,error,read,oldMask:encode(oldTarget.mask),current:encode(current),events:encode(events)};}
const expected=[metadata()];for(let m=0;m<6;m++)expected.push(construct(m));for(const method of ['set','enable','toggle','disable','test','isEnabled'])for(let m=0;m<24;m++)for(let n=0;n<24;n++)expected.push(numeric(method,m,n));for(const method of ['set','enable','toggle','disable','test','isEnabled'])for(let left=0;left<8;left++)for(let a=0;a<7;a++)expected.push(dynamic(method,left,a));for(let mode=0;mode<9;mode++)expected.push(native(mode));for(const method of methods)for(let mode=0;mode<24;mode++)expected.push(borrow(method,mode));for(let mode=0;mode<3;mode++)expected.push(namespaceCall(mode));for(let mode=0;mode<3;mode++)expected.push(dispatch(mode));for(let mode=0;mode<4;mode++)expected.push(copyReentry(mode));expected.push(uniformsMetadata());for(let mode=0;mode<10;mode++)expected.push(uniforms(mode));
function unlitWorking(branch,mode,index){
 const original=Object.getOwnPropertyDescriptor(ColorManagement,'workingColorSpace'),events=[],renderer={},target={},texture={},replacement={tag:'replacement'},state={reads:0};
 const data=(o,k,v)=>Object.defineProperty(o,k,{value:v,writable:true,enumerable:true,configurable:true});
 const getter=(o,k,get)=>Object.defineProperty(o,k,{get,enumerable:true,configurable:true});
 let working=value(index);
 if(index===24)working=function(){throw RangeError('working value must not be called');};
 if(index===25)working={tag:'working',valueOf(){events.push('valueOf');throw RangeError('working value must not be converted');},toString(){events.push('toString');throw RangeError('working value must not be converted');}};
 try{
  data(ColorManagement,'workingColorSpace',working);
  if(mode!==0)getter(ColorManagement,'workingColorSpace',function(){
   const count=state.reads++;events.push(['working.get',this===ColorManagement,count]);
   if(mode===2)throw RangeError('working getter');
   if(mode>=3&&mode<=5&&count===0){
    const nested={getRenderTarget(){events.push('nested.target');return mode===4?null:{isXRRenderTarget:mode===5,texture:{colorSpace:replacement}};},outputColorSpace:replacement};
    const result=getUnlitUniformColorSpace(nested);events.push(['nested.result',result===replacement]);
   }
   if(mode===6)data(ColorManagement,'workingColorSpace',replacement);
   return mode===3&&count>0?replacement:working;
  });
  getter(renderer,'getRenderTarget',()=>{events.push('target.method.get');return function(){events.push(['target.call',this===renderer]);if(mode===7)data(ColorManagement,'workingColorSpace',replacement);return branch===1?null:target;};});
  getter(target,'isXRRenderTarget',()=>{events.push('xr.get');return branch===3?1:branch===2;});
  getter(target,'texture',()=>{events.push('texture.get');return texture;});
  getter(texture,'colorSpace',()=>{events.push('texture.colorSpace.get');return replacement;});
  getter(renderer,'outputColorSpace',()=>{events.push('output.get');return replacement;});
  const calls=[];for(let i=0;i<2;i++){let result;const error=attempt(()=>result=getUnlitUniformColorSpace(renderer));calls.push({error,value:encode(result),workingIdentity:Object.is(result,working),replacementIdentity:Object.is(result,replacement)});}
  return {kind:'unlit-working',branch,mode,index,calls,events:encode(events)};
 }finally{Object.defineProperty(ColorManagement,'workingColorSpace',original);}
}
assert.equal(expected.length,4021,'preserve all original scenarios');
for(let branch=0;branch<4;branch++){
 for(let mode=0;mode<2;mode++)for(let index=0;index<26;index++)expected.push(unlitWorking(branch,mode,index));
 for(let mode=2;mode<8;mode++)expected.push(unlitWorking(branch,mode,25));
}
const actual=JSON.parse(execFileSync(probe,{encoding:'utf8',maxBuffer:256*1024*1024,timeout:300000}));let comparisons=0;
function compare(a,b,p){comparisons++;if(a===null||b===null||typeof a!=='object'||typeof b!=='object'){assert.deepEqual(a,b,p);return;}assert.deepEqual(Object.keys(a),Object.keys(b),p+' keys');for(const k of Object.keys(b))compare(a[k],b[k],p+'.'+k);}
try{assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`${i}:${expected[i].kind}:${expected[i].method??expected[i].mode??''}`);}catch(e){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-layers-uniforms-failure-'));fs.writeFileSync(path.join(dir,'actual.json'),JSON.stringify(actual,null,2));fs.writeFileSync(path.join(dir,'expected.json'),JSON.stringify(expected,null,2));console.error(dir);throw e;}
console.log(`Layers/UniformsUtils exports parity: ${expected.length} scenarios / ${comparisons} comparisons passed (${process.version}, ${commit}).`);
