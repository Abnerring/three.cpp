import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [upstream,mode='cases']=process.argv.slice(2);let randomCalls=0;Math.random=()=>((++randomCalls)%257)/257;
const load=p=>import(pathToFileURL(path.join(upstream,'src',p+'.js')));
const {default:Overloading,overloadingFn}=await load('nodes/utils/FunctionOverloadingNode');
const {default:Node}=await load('nodes/core/Node');const tsl=await load('nodes/tsl/TSLCore');
const normal=v=>v===undefined?'#undefined':v===null?null:typeof v==='function'?'#function':typeof v==='object'?'#object':v;
function metadata(){
 const fn=f=>({name:f.name,length:f.length,source:Function.prototype.toString.call(f),parentName:Object.getPrototypeOf(f).constructor.name,constructible:(()=>{try{Reflect.construct(function(){},[],f);return true;}catch{return false;}})()});
 const proto=Overloading.prototype,type=Object.getOwnPropertyDescriptor(Overloading,'type'),methods=[];
 for(const key of Reflect.ownKeys(proto)){if(key==='constructor')continue;const d=Object.getOwnPropertyDescriptor(proto,key);methods.push({key,...fn(d.value),writable:d.writable,enumerable:d.enumerable,configurable:d.configurable});}
 return {constructor:{...fn(Overloading),parentName:Object.getPrototypeOf(Overloading).name,prototypeKeys:Reflect.ownKeys(proto)},type:{...fn(type.get),enumerable:type.enumerable,configurable:type.configurable},methods,outer:fn(overloadingFn),inner:fn(overloadingFn([]))};
}
function run(r){
 const out={kind:r.kind},events=[],builder={marker:'builder'};let reason={marker:'reason'};
 const param=(type,index)=>({getNodeType(b){events.push(['param',index,this===params[index],b===builder]);return type;}});
 let params=(r.types??[]).map(param);
 const candidate=(types,index)=>{const f=function(...args){events.push(['call',index,this===undefined,args.length,args.every((v,i)=>v===params[i])]);return 'result'+index;};f.shaderNode={layout:types===null?null:{type:'return'+index,inputs:types.map(type=>({type}))}};f.index=index;return f;};
 const choose=v=>v===null?null:v===undefined?'#undefined':typeof v==='function'?v.index:normal(v);
 const error=e=>e===reason?{identity:'reason'}:e instanceof Error?{name:e.name,...(e.message==='THREE.FunctionOverloadingNode: FunctionNode must be a layout.'?{message:e.message}:{})}:{value:normal(e)};
 const attempt=fn=>{try{return {value:fn()};}catch(e){return {error:error(e)};}};
 if(r.kind==='constructor'){
  params=Array.from({length:r.count},(_,i)=>param('float',i));let supplied=r.mode==='null'?null:r.mode==='undefined'?undefined:[];
  const a=r.mode==='omitted'?new Overloading():new Overloading(supplied,...params),b=new Overloading();
  Object.assign(out,{isNode:a instanceof Node,type:a.type,nodeType:a.nodeType,global:a.global,isCacheable:a.isCacheable(builder),functionNodesSame:a.functionNodes===supplied,functionNodesNull:a.functionNodes===null,defaultFresh:a.functionNodes!==b.functionNodes,parameterLength:a.parametersNodes.length,parametersSame:a.parametersNodes.every((v,i)=>v===params[i]),restFresh:a.parametersNodes!==params,candidate:a._candidateFn,ownKeys:Reflect.ownKeys(a),id:a.id,uuid:a.uuid});
 }else if(r.kind==='score'){
  const fns=r.definitions.map(candidate),n=new Overloading(fns,...params);Object.assign(out,attempt(()=>normal(r.method==='getCandidateFn'?choose(n.getCandidateFn(builder)):n[r.method](builder))));out.cache=choose(n._candidateFn);
 }else if(r.kind==='cache'){
  params=[param('float',0)];const first=candidate(['float'],0),second=candidate(['int'],1),n=new Overloading(r.mode==='none'?[]:[first],...params);
  if(r.mode==='undefined')n._candidateFn=undefined;else if(r.mode==='false')n._candidateFn=false;
  out.first=attempt(()=>choose(n.getCandidateFn(builder)));
  n.functionNodes=[second];params[0].getNodeType=()=>{events.push('changed-param');return 'int';};
  if(r.mode==='parameters-get-first')Object.defineProperty(n,'parametersNodes',{get(){events.push('parameters.get');throw reason;}});
  out.second=attempt(()=>choose(n.getCandidateFn(builder)));out.cache=choose(n._candidateFn);
 }else if(r.kind==='protocol'){
  const state={reads:0,entered:false},f=candidate(['float'],0),n=new Overloading([],{}),input={},inputs={},parameters={},layout={},shader={},iterator={};
  const get=(o,k,label,value,throws=false)=>Object.defineProperty(o,k,{configurable:true,get(){events.push(label);if(throws)throw reason;return typeof value==='function'?value():value;}});
  get(f,'shaderNode','shader.get',()=>{if(r.mode==='reentrant'&&!state.entered){state.entered=true;events.push(['reentrant',choose(n.getCandidateFn(builder))]);}if(r.mode==='gc'){global.gc();events.push('gc');}return r.mode==='shader-undefined'?undefined:shader;},r.mode==='shader-get');
  get(shader,'layout','layout.get',r.mode==='layout-null'||r.mode.startsWith('return-')?null:r.mode==='layout-undefined'?undefined:layout,r.mode==='layout-get');
  get(layout,'inputs','inputs.get',inputs,r.mode==='inputs-get');get(inputs,'length','inputs.length',1,r.mode==='inputs-length');
  get(parameters,'length','params.length',()=>r.mode==='dynamic-length'&&++state.reads>1?0:1,r.mode==='params-length');
  const p={};params=[p];get(parameters,'0','param.get',p,r.mode==='param-get');get(inputs,'0','input.get',input,r.mode==='input-get');
  get(p,'getNodeType','method.get',()=>function(b){events.push(['method.call',this===p,b===builder]);if(r.mode==='method-call')throw reason;if(r.mode==='mutate-type')state.type='float';return 'float';},r.mode==='method-get');
  get(input,'type','input.type',()=>state.type??(r.mode==='mutate-type'?'int':'float'),r.mode==='input-type');
  let steps=0;
  get(iterator,'next','next.get',()=>function(){events.push(['next.call',this===iterator]);if(r.mode==='next-call')throw reason;if(r.mode==='result-primitive')return 7;const result={};get(result,'done','done.get',steps++>0,r.mode==='done-get');get(result,'value','value.get',()=>f,r.mode==='value-get');return result;},r.mode==='next-get');
  get(iterator,'return','return.get',()=>r.mode==='return-not-callable'?7:function(){events.push(['return.call',this===iterator]);if(r.mode==='return-call')throw {marker:'close-reason'};return r.mode==='return-primitive'?7:{};},r.mode==='return-get');
  const iterable={};get(iterable,Symbol.iterator,'iterator.get',()=>function(){events.push(['iterator.call',this===iterable]);return iterator;});n.functionNodes=iterable;n.parametersNodes=parameters;
  out.result=attempt(()=>choose(n.getCandidateFn(builder)));out.cache=choose(n._candidateFn);
 }else if(r.kind==='setup-iteration'){
  let reads=0,index=0;const iterator={},iterable={},n=new Overloading();
  n._candidateFn=function(...args){events.push(['candidate.call',this===undefined,...args]);if(r.mode==='call-throw')throw reason;return args.join(',');};
  Object.defineProperty(n,'parametersNodes',{get(){events.push(['parameters.get',++reads]);if(r.mode==='parameters-get-second'&&reads===2)throw reason;return iterable;}});
  Object.defineProperty(iterable,Symbol.iterator,{get(){events.push('iterator.get');return function(){events.push(['iterator.call',this===iterable]);return iterator;};}});
  Object.defineProperty(iterator,'next',{get(){events.push('next.get');return function(){events.push(['next.call',this===iterator]);if(r.mode==='next-call')throw reason;if(r.mode==='result-primitive')return 3;const done=index>=2,value=10*(++index);return {get done(){events.push('done.get');return done;},get value(){events.push('value.get');if(r.mode==='value-get')throw reason;return value;}};};}});
  Object.defineProperty(iterator,'return',{get(){events.push('return.get');return function(){events.push('return.call');return {};};}});
  out.result=attempt(()=>n.setup(builder));
 }else if(r.kind==='factory'){
  if(r.mode==='real-fn'){
   const a=tsl.Fn(()=>1).setLayout({name:'actualFloat',type:'float',inputs:[{name:'x',type:'float'}]}),b=tsl.Fn(()=>2).setLayout({name:'actualInt',type:'int',inputs:[{name:'x',type:'int'}]});
   params=[param('float',0)];const node=overloadingFn([a,b])(...params);out.isRealFunctionSelected=node.getCandidateFn(builder)===a;out.generatedType=node.generateNodeType(builder);const result=node.setup(builder);out.isNode=result.isNode;out.resultType=result.type;
  }else{
   const fns=[],p=new Node('float'),factory=overloadingFn(fns),foreign={marker:'foreign'};
   const node=factory.call(foreign,...(r.mode==='empty'?[]:r.mode==='primitive'?[7]:[p]));
   Object.assign(out,{type:node.type,isNode:node instanceof Node,functionNodesSame:node.functionNodes===fns,parameterLength:node.parametersNodes.length,firstSame:node.parametersNodes[0]===p,firstNodeType:node.parametersNodes[0]?.nodeType??null,global:node.global,candidate:node._candidateFn});
  }
 }else throw Error('Unknown fixture');
 out.events=events;out.randomCalls=randomCalls;return out;
}
if(mode==='metadata')process.stdout.write(JSON.stringify(metadata()));else if(mode==='cases')process.stdout.write(JSON.stringify(JSON.parse(fs.readFileSync(0,'utf8')).map(run)));else throw Error('Unknown mode');
