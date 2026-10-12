import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
import path from 'node:path';
import fs from 'node:fs';
const [root,probe]=process.argv.slice(2);
if(!root)throw Error('Usage: node compare-native-buffer-attribute-upload.mjs <three.js-root> <probe.exe | --expected>');
const {BufferAttribute}=await import(pathToFileURL(path.join(root,'src/core/BufferAttribute.js')));
const {WebGLAttributes}=await import(pathToFileURL(path.join(root,'src/renderers/webgl/WebGLAttributes.js')));
const encode=v=>v===undefined?'@undefined':typeof v==='function'?'@function':typeof v==='number'?(Number.isNaN(v)?'@NaN':Object.is(v,-0)?'@-0':!Number.isFinite(v)?String(v):v):ArrayBuffer.isView(v)?{typed:v.constructor.name,values:Array.from(v,encode)}:Array.isArray(v)?Array.from(v,encode):v&&typeof v==='object'?Object.fromEntries(Object.keys(v).map(k=>[k,encode(v[k])])):v;
function attempt(fn){try{return {value:encode(fn())}}catch(e){return {error:e.name}}}
function fixture(half=false){
 const attribute=new BufferAttribute(half?new Uint16Array([1,2,3,4,5,6,7,8]):new Float32Array([1,2,3,4,5,6,7,8]),2),trace=[],storage=new Map();let next=0,bound,subCalls=0;
 const f={attribute,trace,storage,behavior:0,busy:false,calls:0,clearCalls:0};
 const gl={FLOAT:5126,HALF_FLOAT:5131,UNSIGNED_SHORT:5123,SHORT:5122,UNSIGNED_INT:5125,INT:5124,BYTE:5120,UNSIGNED_BYTE:5121,
 createBuffer(){trace.push(['create',this===gl]);return ++next},bindBuffer(type,buffer){trace.push(['bind',this===gl,type,buffer]);bound=buffer},
 bufferData(type,array,usage){trace.push(['data',this===gl,type,encode(array),usage]);storage.set(bound,Array.from(new Uint8Array(array.buffer,array.byteOffset,array.byteLength)))},
 bufferSubData(type,offset,array,start,count){trace.push(['sub',this===gl,type,offset,encode(array),encode(start),encode(count)]);subCalls++;if(f.subThrow===subCalls)throw RangeError('sub');const first=start??0,n=count===undefined||count===0?array.length-first:count,bytes=new Uint8Array(array.buffer,array.byteOffset+first*array.BYTES_PER_ELEMENT,n*array.BYTES_PER_ELEMENT),dest=storage.get(bound);for(let i=0;i<bytes.length;i++)dest[offset+i]=bytes[i];if(f.onSub)f.onSub(subCalls)},
 deleteBuffer(buffer){trace.push(['delete',this===gl,buffer]);storage.delete(buffer);if(f.onDelete)f.onDelete()}};
 f.gl=gl;f.manager=WebGLAttributes(gl);f.update=()=>f.manager.update(attribute,34962);f.cache=()=>encode(f.manager.get(attribute));
 f.gpu=()=>Array.from(storage,([buffer,bytes])=>({buffer,bytes:[...bytes]}));
 f.upload=function(){f.calls++;trace.push(['upload',this===attribute,f.calls,attribute.version,attribute.updateRanges.length,attribute.array.constructor.name]);if(f.busy)return;const behavior=f.behavior;
 if(behavior===1)attribute.array=new Uint16Array(half?8:16).fill(9);
 if(behavior===2)attribute.array=new Float32Array(9).fill(10);
 if(behavior===3)attribute.version+=2;
 if(behavior===4)attribute.isFloat16BufferAttribute=true;
 if(behavior===5)attribute.onUploadCallback=null;
 if(behavior===6)throw RangeError('upload');
 if(behavior===7)f.manager.remove(attribute);
 if(behavior===8){f.busy=true;f.manager.update(attribute,77);f.busy=false;}
 if(behavior===9)attribute.dispose();
 if(behavior===10)attribute.onUploadCallback=function(){f.calls++;trace.push(['replacement',this===attribute,f.calls])};
 if(behavior===11){attribute.array=new Uint16Array(half?8:16).fill(11);attribute.needsUpdate=true;}
 if(behavior===12){attribute.isFloat16BufferAttribute=true;attribute.version+=.5;}
 if(behavior===13){attribute.clearUpdateRanges();attribute.addUpdateRange(6,1);}
 if(behavior===14){attribute.onUploadCallback=function(){f.calls++;trace.push(['replacement',this===attribute,f.calls])};f.busy=true;f.manager.update(attribute,77);f.busy=false;}
 if(behavior===15){attribute.dispatchEvent({type:'dispose'});}
 };
 attribute.onUpload(f.upload);return f;
}
function nativeUpload(behavior,updating){
 const f=fixture(behavior===4||behavior===12),a=f.attribute;a.addUpdateRange(0,1);let initial='not-run';if(updating){initial=attempt(f.update);f.trace.length=0;a.array[0]=33;a.needsUpdate=true;}
 if(behavior===15)a.addEventListener('dispose',function(){f.trace.push(['dispose-listener',this===a]);f.manager.remove(a)});
 const original=a.array,originalRanges=a.updateRanges;f.behavior=behavior;const result=attempt(f.update),after=f.cache(),firstTrace=encode(f.trace),firstGPU=f.gpu();
 f.behavior=0;const noVersion=attempt(f.update);a.needsUpdate=true;const nextVersion=attempt(f.update);
 return {kind:'native-upload',behavior,updating,initial,result,after,firstTrace,firstGPU,noVersion,nextVersion,cache:f.cache(),array:encode(a.array),sameArray:a.array===original,sameRanges:a.updateRanges===originalRanges,ranges:encode(a.updateRanges),version:encode(a.version),half:encode(a.isFloat16BufferAttribute),trace:encode(f.trace),gpu:f.gpu()};
}
function nativeRanges(mode){
 const f=fixture(),a=f.attribute;f.update();f.trace.length=0;a.array.set([8,7,6,5,4,3,2,1]);a.addUpdateRange(6,1);a.addUpdateRange(1,2);a.addUpdateRange(3,2);const ranges=a.updateRanges,handles=[...ranges];let replacement;
 if(mode===1)Object.defineProperty(ranges,'sort',{configurable:true,get(){f.trace.push(['sort-get']);return function(compare){f.trace.push(['sort-call',this===ranges,compare(handles[0],handles[1])]);return Array.prototype.sort.call(this,compare)}}});
 if(mode===2)ranges.sort=function(){f.trace.push(['sort-reverse',this===ranges]);return Array.prototype.reverse.call(this)};
 if(mode===3)a.clearUpdateRanges=function(){f.clearCalls++;f.trace.push(['clear-noop',this===a])};
 if(mode===4)a.clearUpdateRanges=function(){f.trace.push(['clear-throw',this===a]);throw RangeError('clear')};
 if(mode===5){f.onSub=count=>{if(count===1){replacement=[{start:0,count:1}];a.updateRanges=replacement;f.trace.push(['replace-ranges'])}};}
 if(mode===6){a.clearUpdateRanges=function(){f.trace.push(['clear-version',this===a]);a.version+=2;BufferAttribute.prototype.clearUpdateRanges.call(this)}};
 if(mode===7)Object.defineProperty(handles[1],'count',{value:2,writable:false});
 if(mode===8){Object.defineProperty(handles[1],'start',{get(){f.trace.push(['start-get']);return 1},configurable:true});}
 if(mode===9){Object.defineProperty(handles[1],'count',{get(){f.trace.push(['count-get']);return 2},set(v){f.trace.push(['count-set',v]);throw RangeError('count')},configurable:true});}
 if(mode===10){ranges.length=0;a.addUpdateRange(0,1);a.addUpdateRange(4,1);f.subThrow=2;}
 if(mode===11){ranges.length=0;a.addUpdateRange(0,1);a.addUpdateRange(4,1);f.onSub=count=>{if(count===1)a.onUploadCallback=null};}
 if(mode===12){ranges.sort=function(compare){f.trace.push(['sort-replace',this===ranges]);a.updateRanges=[];return Array.prototype.sort.call(this,compare)}};
 if(mode===13){a.clearUpdateRanges=null;}
 a.needsUpdate=true;const result=attempt(f.update),cache=f.cache(),snapshot=encode(ranges),handleValues=encode(handles),live=encode(a.updateRanges);
 return {kind:'native-ranges',mode,result,cache,ranges:snapshot,handles:handleValues,live,sameRanges:ranges===a.updateRanges,replacementActive:replacement===a.updateRanges,version:encode(a.version),trace:encode(f.trace),gpu:f.gpu()};
}
function inherited(mode){
 const f=fixture(mode===7),a=f.attribute,original=a.array,originalUsage=a.usage,originalRanges=a.updateRanges;const proto=Object.create(Object.getPrototypeOf(a));Object.setPrototypeOf(a,proto);let gets=0;
 if(mode===0||mode===1){delete a.array;Object.defineProperty(proto,'array',{get(){gets++;f.trace.push(['array-get',this===a,gets]);return mode===1&&gets>1?new Float32Array(8).fill(9):original}});}
 if(mode===2){delete a.usage;Object.defineProperty(proto,'usage',{get(){f.trace.push(['usage-get',this===a]);return originalUsage}})}
 if(mode===3){delete a.onUploadCallback;Object.defineProperty(proto,'onUploadCallback',{get(){f.trace.push(['upload-get',this===a]);return f.upload}})}
 if(mode===4){delete a.updateRanges;Object.defineProperty(proto,'updateRanges',{get(){f.trace.push(['ranges-get',this===a]);return originalRanges}})}
 if(mode===5){delete a.version;Object.defineProperty(proto,'version',{get(){gets++;f.trace.push(['version-get',this===a,gets]);return gets},set(v){f.trace.push(['version-set',this===a,v])}})}
 if(mode===6){Object.defineProperty(proto,'clearUpdateRanges',{get(){f.trace.push(['clear-get',this===a]);return function(){f.trace.push(['clear-call',this===a]);BufferAttribute.prototype.clearUpdateRanges.call(this)}}})}
 if(mode===7){Object.defineProperty(proto,'isFloat16BufferAttribute',{get(){f.trace.push(['half-get',this===a]);return true}})}
 if(mode===8){delete a.array;Object.defineProperty(proto,'array',{get(){f.trace.push(['array-throw',this===a]);throw RangeError('array')}})}
 if(mode===9){delete a.onUploadCallback;Object.defineProperty(proto,'onUploadCallback',{get(){f.trace.push(['upload-throw',this===a]);throw RangeError('upload')}})}
 const initial=attempt(f.update);original[0]=9;originalRanges.push({start:0,count:1});a.needsUpdate=true;const update=attempt(f.update);return {kind:'native-inherited',mode,initial,update,cache:f.cache(),original:encode(original),ranges:encode(originalRanges),trace:encode(f.trace),gpu:f.gpu()};
}
function disposal(mode){const f=fixture(),a=f.attribute;f.update();f.trace.length=0;const first=f.manager.get(a);if(mode===1)a.addEventListener('dispose',()=>f.manager.remove(a));if(mode===2)f.onDelete=()=>{if(!f.busy){f.busy=true;a.needsUpdate=true;f.update();f.busy=false}};const action=attempt(()=>mode===2?f.manager.remove(a):a.dispose());const after=f.cache(),again=attempt(f.update);return {kind:'native-dispose',mode,action,after,again,same:first===f.manager.get(a),cache:f.cache(),trace:encode(f.trace),gpu:f.gpu()}}
const expected=[];for(let behavior=0;behavior<16;behavior++)for(const updating of [false,true])expected.push(nativeUpload(behavior,updating));for(let mode=0;mode<14;mode++)expected.push(nativeRanges(mode));for(let mode=0;mode<10;mode++)expected.push(inherited(mode));for(let mode=0;mode<3;mode++)expected.push(disposal(mode));
if(probe==='--expected'||!probe){fs.writeFileSync(path.join(import.meta.dirname,'native-buffer-attribute-upload.expected.json'),JSON.stringify(expected,null,2)+'\n');console.log(`Saved ${expected.length} native BufferAttribute scenarios from ${root}.`);}else{
 const actual=JSON.parse(execFileSync(probe,{encoding:'utf8',maxBuffer:32*1024*1024}));assert.equal(actual.length,expected.length);let comparisons=0;const compare=(a,b,p)=>{comparisons++;if(a===null||b===null||typeof a!=='object'||typeof b!=='object'){assert.deepEqual(a,b,p);return}assert.deepEqual(Object.keys(a),Object.keys(b),p+' keys');for(const k of Object.keys(b))compare(a[k],b[k],p+'.'+k)};expected.forEach((v,i)=>compare(actual[i],v,`${i}:${v.kind}`));console.log(`Native BufferAttribute upload: ${expected.length} scenarios / ${comparisons} comparisons passed.`);
}
