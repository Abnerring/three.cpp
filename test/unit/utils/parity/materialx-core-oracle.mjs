import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {pathToFileURL} from 'node:url';
const [upstream,mode='cases']=process.argv.slice(2);assert(upstream);assert.equal(process.version,'v26.5.0');
let randomCalls=0,randomSeed=0x12345678;
Math.random=()=>{randomCalls++;randomSeed=(Math.imul(randomSeed,1664525)+1013904223)>>>0;return randomSeed/4294967296;};
const load=f=>import(pathToFileURL(path.join(upstream,'src',f+'.js')).href);
const functions=await load('nodes/materialx/MaterialXCore'),tsl=await load('nodes/tsl/TSLBase');
const {default:NodeBuilder}=await load('nodes/core/NodeBuilder'),{default:StackNode}=await load('nodes/core/StackNode');
const utils=await load('utils');
const names=['mx_rotate2d','mx_rotate3d'];
const token=v=>v===undefined?['undefined']:typeof v==='function'?['function',v.name,v.length,Function.prototype.toString.call(v)]:typeof v==='number'&&!Number.isFinite(v)?['number',String(v)]:Object.is(v,-0)?['number','-0']:typeof v==='symbol'?['symbol',v.description??null]:v;
function graph(value){const objects=[],seen=[];function encode(v){if(typeof v==='function')return ['function',token(v.name),token(v.length),Function.prototype.toString.call(v)];if(v===null||typeof v!=='object')return token(v);let i=seen.indexOf(v);if(i>=0)return ['ref',i];i=seen.length;seen.push(v);const row={kind:Array.isArray(v)?'array':'object',properties:[]};objects.push(row);if(v.isNode===true){row.type=v.type;row.id=v.id;row.uuid=v.uuid;}if(Array.isArray(v))row.length=v.length;for(const key of Reflect.ownKeys(v)){const d=Object.getOwnPropertyDescriptor(v,key),name=typeof key==='symbol'?token(key):key;row.properties.push([name,'value'in d?encode(d.value):['accessor',!!d.get,!!d.set],d.enumerable,d.configurable,'writable'in d?d.writable:null]);}return ['ref',i];}return {root:encode(value),objects};}
function decode(v){if(v===null||typeof v!=='object')return v;if('$undefined'in v)return undefined;if('$symbol'in v)return Symbol(v.$symbol);if('$number'in v)return {NaN:NaN,Infinity:Infinity,'-Infinity':-Infinity,'-0':-0}[v.$number];if('node'in v)return tsl[v.node](...v.args.map(decode));if(Array.isArray(v))return v.map(decode);return Object.fromEntries(Object.entries(v).map(([k,x])=>[k,decode(x)]));}
function metadata(){const out={};for(const name of names){const fn=functions[name];out[name]={name:fn.name,length:fn.length,source:Function.prototype.toString.call(fn),ownKeys:Reflect.ownKeys(fn),constructible:(()=>{try{Reflect.construct(function(){},[],fn);return true;}catch{return false;}})(),parentName:Object.getPrototypeOf(fn).constructor.name};}return out;}
function run(request){
 if(request.mode==='metadata')return {metadata:metadata(),randomCalls};
 if(request.mode==='bindings')return {exports:Object.keys(functions).map(name=>[name,functions[name]===functions[name]]),randomCalls};
 const events=[],messages=[],previous=tsl.getCurrentStack(),stack=new StackNode(),reason={reason:'original'};tsl.setCurrentStack(stack);utils.setConsoleFunction((...a)=>messages.push(a));
 let savedTarget,savedTargetKey,savedMath,result;
 try{const args=request.args.map(decode);
  if(request.instrument){const {argument,key,mode}=request.instrument,target=args[argument],method=target[key];savedTarget=[target,Object.getOwnPropertyDescriptor(target,key)];savedTargetKey=key;Object.defineProperty(target,key,{configurable:true,get(){events.push(['get',key,this===target]);gc();if(mode==='getThrow')throw reason;if(mode==='reenter')events.push(['reenter',functions.mx_rotate2d(tsl.vec2(.12,.23),.5).isNode]);return method;}});}
  if(request.mathPI){savedMath=globalThis.Math;const wrapped=Object.create(savedMath),state={entered:false};Object.defineProperty(wrapped,'PI',{configurable:true,get(){events.push(['math.pi',this===wrapped]);gc();if(request.mathPI==='getThrow')throw reason;if(request.mathPI==='reenter'&&!state.entered){state.entered=true;events.push(['math.reenter',functions.mx_rotate2d(tsl.vec2(.12,.23),.5).isNode]);}if(request.mathPI==='coerce'||request.mathPI==='coerceThrow'){const value={};Object.defineProperty(value,'valueOf',{get(){events.push(['pi.valueOf.get',this===value]);gc();return function(){events.push(['pi.valueOf.call',this===value]);gc();if(request.mathPI==='coerceThrow')throw reason;return savedMath.PI;};}});return value;}return savedMath.PI;}});globalThis.Math=wrapped;}
  const fn=functions[request.fn];const value=Reflect.apply(fn,undefined,args);
  result={value:graph(value),stack:graph(stack.nodes)};
  if(request.build){const builder=new NodeBuilder(null,{debug:{diagnostics:{keywords:false}}},null);builder.shaderStage='fragment';stack.outputNode=value;try{builder.flowBuildStage(stack,'setup');result.nodeType=value.getNodeType(builder);result.flow=graph(builder.flowStagesNode(stack));}catch(error){result.buildError={name:error.name,thrownOriginal:error===reason};}result.builder={shaderStage:builder.shaderStage,buildStage:builder.buildStage,stackIdentity:builder.stack===stack};}
 }catch(error){result={error:{name:error===reason?null:error.name,thrownOriginal:error===reason},stack:graph(stack.nodes)};}
 finally{if(savedMath)globalThis.Math=savedMath;if(savedTarget){if(savedTarget[1])Object.defineProperty(savedTarget[0],savedTargetKey,savedTarget[1]);else Reflect.deleteProperty(savedTarget[0],savedTargetKey);}tsl.setCurrentStack(previous);utils.setConsoleFunction(null);}
 result.events=events;result.messages=graph(messages);result.randomCalls=randomCalls;return result;
}
async function lifetime(){assert.equal(typeof gc,'function');const observations=[];const checkpoint=async()=>{for(let i=0;i<8;++i){await new Promise(resolve=>setImmediate(resolve));gc();}};for(let round=0;round<3;round++)for(const name of names){let held;const weak=(()=>{const previous=tsl.getCurrentStack(),stack=new StackNode();tsl.setCurrentStack(stack);try{const color=tsl[name==='mx_rotate2d'?'vec2':'vec3'](.371+round*.013,.241,.173);held=functions[name](color);return new WeakRef(color);}finally{tsl.setCurrentStack(previous);}})();await checkpoint();assert(weak.deref(),'Actual graph lost '+name);observations.push({round,name,phase:'held',alive:true});held=null;await checkpoint();assert(!weak.deref(),'Dropped graph retained '+name);observations.push({round,name,phase:'dropped',gone:true});}return {rounds:3,observations};}
if(mode==='cases'){assert.equal(typeof gc,'function');const input=JSON.parse(fs.readFileSync(0,'utf8'));process.stdout.write(JSON.stringify(input.map(run)));}
else if(mode==='lifetime')process.stdout.write(JSON.stringify(await lifetime()));else throw Error('Unknown mode');
