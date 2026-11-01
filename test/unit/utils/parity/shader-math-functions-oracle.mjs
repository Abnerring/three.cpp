import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [upstream,mode='cases']=process.argv.slice(2);
if(!upstream)throw Error('Explicit fixed upstream required');
let randomCalls=0,randomSeed=0x12345678;
Math.random=()=>{randomCalls++;randomSeed=(Math.imul(randomSeed,1664525)+1013904223)>>>0;return randomSeed/4294967296;};
const load=file=>import(pathToFileURL(path.join(upstream,'src',file+'.js')).href);
const hashModule=await load('nodes/math/Hash'),mathModule=await load('nodes/math/MathUtils');
const tsl=await load('nodes/tsl/TSLBase');
const {default:NodeBuilder}=await load('nodes/core/NodeBuilder');
const {default:StackNode}=await load('nodes/core/StackNode');
const utils=await load('utils');
const functions={hash:hashModule.hash,...mathModule};
const own=(o,k,v)=>Object.defineProperty(o,k,{value:v,writable:true,enumerable:true,configurable:true});
const token=value=>value===undefined?['undefined']:typeof value==='function'?['function',value.name,value.length,Function.prototype.toString.call(value)]:typeof value==='number'&&!Number.isFinite(value)?['number',String(value)]:Object.is(value,-0)?['number','-0']:typeof value==='symbol'?['symbol',value.description??null]:value;
function graph(value){
 const objects=[],seen=[];
 function encode(v){
  if(typeof v==='function')return ['function',token(v.name),token(v.length),Function.prototype.toString.call(v)];
  if(v===null||typeof v!=='object')return token(v);
  let i=seen.indexOf(v);if(i>=0)return ['ref',i];i=seen.length;seen.push(v);
  const row={kind:Array.isArray(v)?'array':'object',properties:[]};objects.push(row);
  if(v.isNode===true){row.type=v.type;row.id=v.id;row.uuid=v.uuid;}
  if(Array.isArray(v))row.length=v.length;
  for(const key of Reflect.ownKeys(v)){
   const d=Object.getOwnPropertyDescriptor(v,key),name=typeof key==='symbol'?token(key):key;
   row.properties.push([name,'value'in d?encode(d.value):['accessor',!!d.get,!!d.set],d.enumerable,d.configurable,'writable'in d?d.writable:null]);
  }
  return ['ref',i];
 }
 return {root:encode(value),objects};
}
function decode(v){
 if(v===null||typeof v!=='object')return v;
 if('$undefined'in v)return undefined;if('$symbol'in v)return Symbol(v.$symbol);
 if('$number'in v)return {NaN:NaN,Infinity:Infinity,'-Infinity':-Infinity,'-0':-0}[v.$number];
 if('node'in v)return tsl[v.node](...v.args.map(decode));
 if(Array.isArray(v))return v.map(decode);return Object.fromEntries(Object.entries(v).map(([k,x])=>[k,decode(x)]));
}
function metadata(){
 const result={};
 for(const name of ['hash','parabola','gain','pcurve','sinc']){
  const fn=functions[name];const f=name==='hash'?fn.shaderNode.jsFunc:fn;
  result[name]={name:token(fn.name),length:token(fn.length),source:Function.prototype.toString.call(fn),ownKeys:Reflect.ownKeys(fn).map(token),callback:{name:f.name,length:f.length,source:Function.prototype.toString.call(f),constructible:(()=>{try{Reflect.construct(function(){},[],f);return true;}catch{return false;}})()},isFn:token(fn.isFn),layout:name==='hash'?graph(fn.shaderNode.layout):null};
 }
 return result;
}
function rawInputs(args,kind,events){
 if(!kind)return args;let index=0;
 return {[Symbol.iterator](){events.push('iterator');return {next(){events.push('next');if(kind==='nextThrow')throw RangeError('next');return {get done(){events.push('done');if(kind==='doneThrow')throw RangeError('done');return kind==='empty'||index++>0;},get value(){events.push('value');if(kind==='valueThrow')throw RangeError('value');return args[0];}};},get return(){events.push('get:return');return function(){events.push('return');if(kind==='returnThrow')throw RangeError('return');return kind==='returnPrimitive'?17:{};};}};}};
}
function run(request){
 if(request.mode==='metadata')return {metadata:metadata(),randomCalls};
 if(request.mode==='bindings')return {hash:hashModule.hash===functions.hash,math:Object.keys(mathModule).map(name=>[name,mathModule[name]===functions[name]]),randomCalls};
 const events=[],messages=[],previous=tsl.getCurrentStack(),stack=new StackNode();tsl.setCurrentStack(stack);utils.setConsoleFunction((...a)=>messages.push(a));
 let result;
 try{
  const args=request.args.map(decode);
  if(request.instrument){
   const {argument,key,mode}=request.instrument,target=args[argument],method=target[key];
   Object.defineProperty(target,key,{configurable:true,get(){events.push(['get',key,this===target]);if(mode==='getThrow')throw RangeError('method getter');if(mode==='reenter'){events.push(['reenter',functions.parabola(tsl.float(0.371),tsl.float(2.25)).isNode]);}if(mode==='collect'&&global.gc)global.gc();return method;}});
  }
  const value=request.mode==='body'?functions.hash.shaderNode.jsFunc(rawInputs(args,request.iterator,events)):functions[request.fn](...args);
  result={value:graph(value),stack:graph(stack.nodes)};
  if(request.build){
   const builder=new NodeBuilder(null,{debug:{diagnostics:{keywords:false}}},null);builder.shaderStage='fragment';
   stack.outputNode=value;
   try{builder.flowBuildStage(stack,'setup');result.nodeType=value.getNodeType(builder);result.flow=graph(builder.flowStagesNode(stack));}catch(error){result.buildError={name:error.name,message:error.message};}
   result.builder={shaderStage:builder.shaderStage,buildStage:builder.buildStage,stackIdentity:builder.stack===stack};
  }
 }catch(error){result={error:{name:error.name,message:error.message},stack:graph(stack.nodes)};}
 finally{tsl.setCurrentStack(previous);utils.setConsoleFunction(null);}
 result.events=events;result.messages=graph(messages);result.randomCalls=randomCalls;return result;
}
async function lifetime(){
 if(!global.gc)throw Error('--expose-gc required');const observations=[];
 const checkpoint=async()=>{for(let i=0;i<8;i++){await new Promise(resolve=>setImmediate(resolve));global.gc();}};
 for(let round=0;round<3;round++)for(const name of ['hash','parabola','gain','pcurve','sinc']){
  let held;const weak=(()=>{const previous=tsl.getCurrentStack(),stack=new StackNode();tsl.setCurrentStack(stack);try{const x=tsl.float(0.371+round*0.013),k=tsl.float(2.271),b=tsl.float(3.179);held=name==='hash'?functions.hash.shaderNode.jsFunc([x]):functions[name](x,k,b);return new WeakRef(x);}finally{tsl.setCurrentStack(previous);}})();
  await checkpoint();if(!weak.deref())throw Error('Actual '+name+' graph lost its input');observations.push({round,name,phase:'held',alive:true});held=null;
  await checkpoint();if(weak.deref())throw Error('Dropped '+name+' graph retained its input');observations.push({round,name,phase:'dropped',gone:true});
 }
 return {rounds:3,observations};
}
if(mode==='cases'){const requests=JSON.parse(fs.readFileSync(0,'utf8'));process.stdout.write(JSON.stringify(requests.map(run)));}
else if(mode==='lifetime')process.stdout.write(JSON.stringify(await lifetime()));
else throw Error('Unknown mode');
