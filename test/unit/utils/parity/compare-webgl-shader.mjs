import assert from 'node:assert/strict';
import fs from 'node:fs';import path from 'node:path';import os from 'node:os';
import {execFileSync,spawnSync}from'node:child_process';import {pathToFileURL}from'node:url';
const upstream=path.resolve(process.argv[2]),probe=path.resolve(process.argv[3]);const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const {WebGLShader}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLShader.js')));
function run(variant,construction,point){
 const log=[],gl={},type={},source={};const record=x=>{log.push(x);if(log.length===point)throw RangeError('audit');};
 const shader=variant===1?null:variant===2?17:variant===3?undefined:variant===4?'handle':variant===5?[]:variant===6?function(){}:{};
 const replacement=()=>record('replacement:next');
 for(const name of ['createShader','shaderSource','compileShader']){
  const method=function(...args){record('call:'+name);record(this===gl);record(args.length);
   if(name==='createShader'){
    record(args[0]===type);if(variant===11)Object.defineProperty(gl,'shaderSource',{value:replacement,writable:true,enumerable:true,configurable:true});
    if(variant===13){const nested={};for(const k of ['createShader','shaderSource','compileShader'])nested[k]=()=>{record('nested:'+k);return 7;};WebGLShader(nested,2,'nested');}return shader;
   }
   record(args[0]===shader);if(name==='shaderSource'){record(args[1]===source);if(variant===12)Object.defineProperty(gl,'compileShader',{value:replacement,writable:true,enumerable:true,configurable:true});}
  };
  Object.defineProperty(gl,name,{enumerable:true,configurable:true,get(){record('get:'+name);if((variant===7&&name==='createShader')||(variant===8&&name==='shaderSource')||(variant===9&&name==='compileShader'))return 17;
   if(variant===10&&name==='createShader')Object.defineProperty(gl,'shaderSource',{value:replacement,writable:true,enumerable:true,configurable:true});return method;
  }});
 }
 const proto={};function Other(){throw RangeError('unexpected newTarget body');}if(construction===2||construction===4)Other.prototype=proto;if(construction===3)Other.prototype=null;
 let result,error='';try{result=construction===0?WebGLShader(gl,type,source):construction===1?new WebGLShader(gl,type,source):Reflect.construct(WebGLShader,[gl,type,source],Other);}catch(e){error=e.name;}
 const isObject=result!==null&&(typeof result==='object'||typeof result==='function');const descriptor=k=>{const d=Object.getOwnPropertyDescriptor(WebGLShader,k);return[d.writable,d.enumerable,d.configurable];};
 return {kind:'shader',variant,construction,point,error,log,sameShader:result===shader,object:isObject,proto:isObject?Object.getPrototypeOf(result)===WebGLShader.prototype?'fn':Object.getPrototypeOf(result)===proto?'other':Object.getPrototypeOf(result)===Object.prototype?'object':'other-proto':'primitive',name:WebGLShader.name,length:WebGLShader.length,prototypeDescriptor:descriptor('prototype'),nameDescriptor:descriptor('name'),lengthDescriptor:descriptor('length'),constructor:WebGLShader.prototype.constructor===WebGLShader};
}
const expected=[];for(let v=0;v<14;v++)for(let c=0;c<5;c++)for(let p=-1;p<22;p++)expected.push(run(v,c,p));
const child=spawnSync(probe,[],{encoding:'utf8',maxBuffer:64*1024*1024});if(child.status!==0)throw Error(`probe ${child.status}: ${child.stderr}`);const actual=child.stdout.trim().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,expected.length);
for(let i=0;i<expected.length;i++)try{assert.deepEqual(actual[i],expected[i]);}catch(e){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-webgl-shader-'));fs.writeFileSync(path.join(dir,'actual.json'),JSON.stringify(actual[i],null,2));fs.writeFileSync(path.join(dir,'expected.json'),JSON.stringify(expected[i],null,2));throw Error(`case ${i}: ${e.message}\n${dir}`);}
console.log(`WebGLShader parity: ${expected.length} scenarios passed against ${lock.commit}`);
