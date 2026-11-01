import {upstreamModule,upstream,upstreamCommit} from './threejs-source.mjs';
export {upstream,upstreamCommit};
const {cloneUniforms,mergeUniforms}=await upstreamModule('src/renderers/shaders/UniformsUtils.js');
const IntrinsicSymbol=Symbol,prototype=Symbol.prototype,IntrinsicError=Error,IntrinsicTypeError=TypeError;
const own=Reflect.ownKeys,descriptor=Object.getOwnPropertyDescriptor,define=Object.defineProperty;
export function encode(value){
 if(value===undefined)return ['undefined'];
 if(typeof value==='number'){
  if(Number.isNaN(value))return ['number','NaN'];
  if(!Number.isFinite(value))return ['number',value>0?'Infinity':'-Infinity'];
  if(Object.is(value,-0))return ['number','-0'];
 }
 if(typeof value==='symbol')return ['symbol',encode(value.description),encode(IntrinsicSymbol.keyFor(value))];
 if(value===null||typeof value!=='object')return value;
 const keys=own(value);return {kind:Array.isArray(value)?'array':'object',keys:keys.map(encode),values:keys.map(key=>encode(value[key]))};
}
function attempt(callback){try{return {value:encode(callback())};}catch(error){return error instanceof IntrinsicError?{error:error.name,nativeError:true,typeError:error instanceof IntrinsicTypeError}:{thrown:encode(error)};}}
export function run(request){
 const symbol=IntrinsicSymbol(request.name),receiver=request.boxed?Object(symbol):symbol,key=request.level==='outer'?'uniform':'value';
 const events=[],savedProperty=descriptor(prototype,key),savedConstructor=descriptor(prototype,'constructor'),savedGlobal=descriptor(globalThis,'Symbol');
 let reads=0;
 const uniform=value=>({value}),source=request.level==='outer'?receiver:{uniform:receiver};
 const value=()=>request.level==='outer'?uniform(request.value+reads):request.value+reads;
 const getter=function(){
  events.push(['get',key,typeof this,Object.is(this,receiver),++reads]);
  if(request.action==='throwNumber')throw 17;
  if(request.action==='throwTypeError')throw new IntrinsicTypeError('uniform getter');
  if(request.action==='reenter'&&reads===1){const nested=cloneUniforms({nested:uniform(request.value+100)});events.push(['nested',nested.nested.value]);}
  return value();
 };
 try{
  define(prototype,key,request.accessor?{get:getter,enumerable:true,configurable:true}:{value:value(),writable:true,enumerable:true,configurable:true});
  if(request.alias==='data'){
   const fake=function FakeSymbol(){events.push(['global Symbol call']);throw 19;};
   define(globalThis,'Symbol',{value:fake,writable:true,enumerable:false,configurable:true});
   define(prototype,'constructor',{value:fake,writable:true,enumerable:false,configurable:true});
  }else if(request.alias==='getters'){
   define(globalThis,'Symbol',{get(){events.push(['global Symbol getter']);throw 19;},enumerable:false,configurable:true});
   define(prototype,'constructor',{get(){events.push(['prototype constructor getter']);throw 23;},enumerable:false,configurable:true});
  }
  const result=attempt(()=>request.operation==='merge'?mergeUniforms([source]):cloneUniforms(source));
  return {result,events:encode(events)};
 }finally{
  if(savedProperty)define(prototype,key,savedProperty);else delete prototype[key];
  define(prototype,'constructor',savedConstructor);define(globalThis,'Symbol',savedGlobal);
 }
}
export function oracle(requests){return requests.map(request=>({name:request.name,value:run(request)}));}
