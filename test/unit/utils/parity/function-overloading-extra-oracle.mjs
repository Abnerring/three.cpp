import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {pathToFileURL} from 'node:url';
const [upstream,mode='trace']=process.argv.slice(2);
let randomCalls=0;Math.random=()=>((++randomCalls)%257)/257;
const sourceFile=pathToFileURL(path.join(upstream,'src/nodes/utils/FunctionOverloadingNode.js')).href;
const {default:Overloading,overloadingFn}=await import(sourceFile);
const {default:Node}=await import(pathToFileURL(path.join(upstream,'src/nodes/core/Node.js')).href);
const originalError=Error;
function capture(){
 const saved=originalError.prepareStackTrace;originalError.prepareStackTrace=(_,sites)=>sites;
 try{return new originalError().stack.filter(site=>site.getFileName()===sourceFile).map(site=>({
  functionName:site.getFunctionName(),line:site.getLineNumber(),column:site.getColumnNumber(),position:site.getPosition(),
  enclosingLine:site.getEnclosingLineNumber(),enclosingColumn:site.getEnclosingColumnNumber(),
  isConstructor:site.isConstructor(),isAsync:site.isAsync(),isNative:site.isNative()
 }));}finally{if(saved===undefined)delete originalError.prepareStackTrace;else originalError.prepareStackTrace=saved;}
}
function traceCase(request){
 const frames=[],events=[],reason={marker:'reason'},builder={marker:'builder'};let entered=false;
 const note=label=>{events.push(label);frames.push({label,frames:capture()});if(request.gc)global.gc();if(request.throw)throw reason;
  if(request.reenter&&!entered){entered=true;events.push('reenter.before');n.getCandidateFn(builder);events.push('reenter.after');}
 };
 const getter=(o,k,label,value)=>Object.defineProperty(o,k,{configurable:true,get(){note(label);return value;}});
 const parameter={getNodeType(){note('param.call');return 'float';}};
 const candidate=function(){note('candidate.call');return 'result';};
 const input={type:'float'},inputs=[input],layout={inputs,type:'float'},shader={layout};candidate.shaderNode=shader;
 const n=new Overloading([candidate],parameter),proto=Overloading.prototype;
 let restore;
 try{
  if(request.target==='constructor'){
   const key=request.key,saved=Object.getOwnPropertyDescriptor(proto,key);restore=()=>saved?Object.defineProperty(proto,key,saved):delete proto[key];
   Object.defineProperty(proto,key,{configurable:true,set(value){note('constructor.set:'+key);Object.defineProperty(this,key,{value,writable:true,enumerable:true,configurable:true});}});
   if(request.factory)overloadingFn([])();else new Overloading();
  }else{
   switch(request.target){
    case 'parameters':getter(n,'parametersNodes','parameters.get',[parameter]);break;
    case 'cacheGet':getter(n,'_candidateFn','cache.get',null);break;
    case 'cacheSet':Object.defineProperty(n,'_candidateFn',{configurable:true,get(){return null;},set(value){note('cache.set');}});break;
    case 'functions':getter(n,'functionNodes','functions.get',[candidate]);break;
    case 'shader':getter(candidate,'shaderNode','shader.get',shader);break;
    case 'layout':getter(shader,'layout','layout.get',layout);break;
    case 'inputs':getter(layout,'inputs','inputs.get',inputs);break;
    case 'paramsLength':{const params={0:parameter};getter(params,'length','params.length',1);n.parametersNodes=params;break;}
    case 'inputsLength':{const value={0:input};getter(value,'length','inputs.length',1);layout.inputs=value;break;}
    case 'paramIndex':{const params={length:1};getter(params,'0','params.0',parameter);n.parametersNodes=params;break;}
    case 'inputIndex':{const value={length:1};getter(value,'0','inputs.0',input);layout.inputs=value;break;}
    case 'paramMethod':getter(parameter,'getNodeType','param.method',parameter.getNodeType);break;
    case 'inputType':getter(input,'type','input.type','float');break;
    case 'resultShader':n._candidateFn=candidate;getter(candidate,'shaderNode','result.shader',shader);break;
    case 'candidate':n._candidateFn=candidate;break;
    case 'method':getter(n,'getCandidateFn','method.get',n.getCandidateFn);break;
    case 'iterator':{let index=0;const iterator={};getter(iterator,'next','iterator.next.get',function(){note('iterator.next.call');return {done:index++>0,value:candidate};});n.functionNodes={[Symbol.iterator](){note('iterator.call');return iterator;}};break;}
    case 'spread':{let index=0;n._candidateFn=candidate;n.parametersNodes={[Symbol.iterator](){note('spread.iterator');return {next(){note('spread.next');return {done:index++>0,value:parameter};}};}};break;}
    case 'error':shader.layout=null;globalThis.Error=new Proxy(originalError,{construct(target,args,newTarget){note('Error.construct');return Reflect.construct(target,args,newTarget);}});restore=()=>globalThis.Error=originalError;break;
    default:throw new originalError('Unknown trace target '+request.target);
   }
   n[request.method??'getCandidateFn'](builder);
  }
 }catch(error){events.push(error===reason?'reason':error.name);}
 finally{restore?.();}
 return {name:request.name,events,frames};
}
function constructorCase(request){
 const events=[],reason={marker:'reason'},functions=[],parameter={marker:'parameter'};let value,error;
 let NewTarget;
 if(request.kind==='implicit'){NewTarget=class Child extends Overloading{};}
 else if(request.kind==='explicit'){NewTarget=class Child extends Overloading{constructor(...args){events.push('child.before');super(...args);events.push('child.after');}};}
 else {NewTarget=function Alternate(){};NewTarget.prototype=request.prototype==='null'?null:request.prototype==='primitive'?17:{marker:'alternate'};}
 const originalPrototype=NewTarget.prototype;
 let target=NewTarget;
 if(request.proxy)target=new Proxy(NewTarget,{get(object,key,receiver){if(key==='prototype'){events.push('newTarget.prototype');if(request.gc)global.gc();if(request.throw)throw reason;}return Reflect.get(object,key,receiver);}});
 try{value=request.kind==='implicit'||request.kind==='explicit'?new target(functions,parameter):Reflect.construct(Overloading,[functions,parameter],target);}
 catch(e){error=e;}
 if(error)return {name:request.name,events,error:error===reason?'reason':error.name};
 const fallback=request.prototype==='null'||request.prototype==='primitive';
 return {name:request.name,events,prototypeSame:Object.getPrototypeOf(value)===(fallback?Object.prototype:originalPrototype),
  functionNodesSame:value.functionNodes===functions,parameterSame:value.parametersNodes[0]===parameter,restFresh:value.parametersNodes!==functions,
  candidate:value._candidateFn,global:value.global,isNode:value instanceof Node,ownKeys:Reflect.ownKeys(value),id:value.id,uuid:value.uuid};
}
const input=JSON.parse(fs.readFileSync(0,'utf8'));
if(mode==='trace')process.stdout.write(JSON.stringify(input.map(traceCase)));
else if(mode==='construct')process.stdout.write(JSON.stringify(input.map(constructorCase)));
else assert.fail('Unknown mode');
