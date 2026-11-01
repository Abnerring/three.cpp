import StackTrace from 'file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/core/StackTrace.js';
import * as mapped from './stacktrace-mapped-scenarios.mjs';
const call=(self,name,args=[])=>self[name](...args);
const decode=value=>value&&typeof value==='object'&&Object.hasOwn(value,'$tag')?({'undefined':undefined,NaN:NaN,Infinity:Infinity,'-Infinity':-Infinity,'-0':-0})[value.$tag]:value;
export function encode(value){
 if(value===undefined)return ['undefined'];
 if(typeof value==='number'){if(Number.isNaN(value))return ['number','NaN'];if(!Number.isFinite(value))return ['number',value>0?'Infinity':'-Infinity'];if(Object.is(value,-0))return ['number','-0'];}
 if(value===null||typeof value!=='object')return value;
 const keys=Reflect.ownKeys(value);return {kind:Array.isArray(value)?'array':'object',keys,values:keys.map(key=>encode(value[key]))};
}
function attempt(fn){try{return {value:encode(fn())};}catch(error){return error instanceof Error?{error:error.name,nativeError:true,typeError:error instanceof TypeError}:{thrown:encode(error)};}}
function inspect(frames){return frames.map(frame=>Object.fromEntries(['getColumnNumber','getEnclosingColumnNumber','getEnclosingLineNumber','getEvalOrigin','getFileName','getFunction','getFunctionName','getLineNumber','getMethodName','getPosition','getPromiseIndex','getScriptNameOrSourceURL','getScriptHash','getThis','getTypeName','isAsync','isConstructor','isEval','isNative','isPromiseAll','isToplevel','toString'].map(name=>[name,frame[name]()]))) }
async function source(request){
 const savedPrepare=Object.getOwnPropertyDescriptor(Error,'prepareStackTrace'),savedLimit=Object.getOwnPropertyDescriptor(Error,'stackTraceLimit');Error.stackTraceLimit=3;delete Error.prepareStackTrace;
 try{
  if(request.operation==='structured')Error.prepareStackTrace=(error,frames)=>inspect(frames);
  if(request.operation==='reentry')Error.prepareStackTrace=mapped.formatter;
  if(request.operation==='lazy'){const error=mapped.outer(request);Error.prepareStackTrace=(error,frames)=>inspect(frames);return {value:encode(error.stack)};}
  if(request.operation==='throw-unwind'){const first=attempt(()=>mapped.outer({...request,operation:'throw'}));const second=attempt(()=>mapped.outer({...request,operation:'frames'}));return {first,second};}
  if(request.operation==='microtask'){Error.stackTraceLimit=1;return {value:encode(await mapped.schedule())};}
  return attempt(()=>mapped.outer(request));
 }finally{if(savedPrepare)Object.defineProperty(Error,'prepareStackTrace',savedPrepare);else delete Error.prepareStackTrace;Object.defineProperty(Error,'stackTraceLimit',savedLimit);}
}
function stack(request){
 const events=[],globalError=Object.getOwnPropertyDescriptor(globalThis,'Error'),parseIntProperty=Object.getOwnPropertyDescriptor(globalThis,'parseInt'),match=Object.getOwnPropertyDescriptor(String.prototype,'match');
 try{
  let input=request.text;
  if(request.operation==='input-split'||request.operation==='input-split-throw')input={get split(){events.push(['get split']);return function(separator){events.push(['split',this===input,separator]);if(request.operation==='input-split-throw')throw 19;return request.text.split(separator);};}};
  if(request.operation==='parseInt-reentry'||request.operation==='parseInt-throw')globalThis.parseInt=function(value,radix){events.push(['parseInt',value,radix]);if(request.operation==='parseInt-throw')throw 23;if(events.length===1){const nested=new StackTrace('at nested (nested.js:7:2)');events.push(['nested',nested.getLocation()]);}return parseIntProperty.value(value,radix);};
  if(request.operation==='parseInt-getter-order')Object.defineProperty(globalThis,'parseInt',{configurable:true,get(){events.push(['get parseInt']);return function(value,radix){events.push(['parseInt',value,radix]);return parseIntProperty.value(value,radix);};}});
  if(request.operation==='match-protocol')Object.defineProperty(String.prototype,'match',{configurable:true,writable:true,value:function(regex){events.push(['match',typeof this,this.toString()]);return Reflect.apply(match.value,this,[regex]);}});
  if(request.operation==='global-error'||request.operation==='empty-input-error'){globalThis.Error=class CustomError{constructor(){events.push(['new Error']);this.stack='at replacement (replacement.js:29:8)';}};input=request.operation==='empty-input-error'?'':null;}
  const result=attempt(()=>{const trace=new StackTrace(input);return {stack:trace.stack,location:trace.getLocation(),error:trace.getError('message')};});return {result,events:encode(events)};
 }finally{Object.defineProperty(globalThis,'Error',globalError);Object.defineProperty(globalThis,'parseInt',parseIntProperty);Object.defineProperty(String.prototype,'match',match);}
}
function integer(request){
 const events=[];const operation=request.operation;
 if(operation==='metadata')return {value:encode({name:parseInt.name,length:parseInt.length,keys:Reflect.ownKeys(parseInt),constructible:attempt(()=>Reflect.construct(parseInt,[])),global: Object.getOwnPropertyDescriptor(globalThis,'parseInt').enumerable,number:parseInt===Number.parseInt})};
 if(operation==='alias')return {value:encode(parseInt===Number.parseInt)};
 let value=decode(request.value),radix=decode(request.radix);
 if(operation==='coercion-order'||operation==='string-throw'||operation==='radix-throw'){
  value={[Symbol.toPrimitive](hint){events.push(['string',hint]);if(operation==='string-throw')throw 31;return '  -0x1f trailing';}};
  radix={[Symbol.toPrimitive](hint){events.push(['radix',hint]);if(operation==='radix-throw')throw 37;return 16;}};
 }
 if(operation==='symbol-input')value=Symbol('input');if(operation==='symbol-radix'){value='17';radix=Symbol('radix');}
 return {result:attempt(()=>parseInt(value,radix)),events:encode(events)};
}
export async function oracle(input){const output=[];for(const request of input)output.push({kind:request.kind,operation:request.operation??null,value:request.kind==='source'?await source(request):request.kind==='stack'?stack(request):integer(request)});return output;}
