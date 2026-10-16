import assert from 'node:assert/strict';
import {execFileSync,spawnSync} from 'node:child_process';
import fs from 'node:fs';import os from 'node:os';import path from 'node:path';import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=process.argv[3]&&path.resolve(process.argv[3]);
const revision='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),revision);
const file='src/renderers/webgl/WebGLProgram.js';assert.equal(fs.readFileSync(path.join(upstream,file),'utf8').replaceAll('\r\n','\n'),execFileSync('git',['-C',upstream,'show',`HEAD:${file}`],{encoding:'utf8'}));
const {WebGLProgram}=await import(pathToFileURL(path.join(upstream,file)));
const {ColorManagement}=await import(pathToFileURL(path.join(upstream,'src/math/ColorManagement.js')));
const {ShaderChunk}=await import(pathToFileURL(path.join(upstream,'src/renderers/shaders/ShaderChunk.js')));
const {setConsoleFunction}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
function encode(v,depth=0){if(v===undefined)return '@undefined';if(typeof v==='symbol')return '@symbol';if(typeof v==='number')return Number.isNaN(v)?'@NaN':!Number.isFinite(v)?(v>0?'@Infinity':'@-Infinity'):Object.is(v,-0)?'@-0':v;if(typeof v==='function')return `@function:${v.name}:${v.length}`;if(v===null||typeof v!=='object')return v;if(Object.hasOwn(v,'fixtureHandle'))return v.fixtureHandle;if(depth>5)return '@object';const out=Array.isArray(v)?[]:{};for(const key of Object.keys(v))Object.defineProperty(out,key,{value:encode(v[key],depth+1),writable:true,enumerable:true,configurable:true});return out;}
function summary(v){if(v===null||typeof v!=='object')return encode(v);if(Object.hasOwn(v,'seq'))return ['uniforms',v.seq.length];return ['object',Object.keys(v)];}
function metadata(fn){return Object.getOwnPropertyNames(fn).map(key=>{const d=Object.getOwnPropertyDescriptor(fn,key);return [key,!!d.writable,!!d.enumerable,!!d.configurable,key==='prototype'?fn[key].constructor===fn:encode(fn[key])];});}
function defaults(){const p={};for(const key of ['numSunLights','numDirLights','numSpotLights','numSpotLightMaps','numSpotLightShadows','numSpotLightShadowsWithMaps','numRectAreaLights','numPointLights','numHemiLights','numSunLightShadows','numDirLightShadows','numPointLightShadows','numClippingPlanes','numClipIntersection'])p[key]=0;return Object.assign(p,{precision:'highp',shaderType:'FixtureMaterial',shaderName:'fixture',vertexShader:'void main() { gl_Position = vec4( 0.0 ); }',fragmentShader:'void main() { gl_FragColor = vec4( 1.0 ); }',toneMapping:0,outputColorSpace:'srgb',envMap:false,isRawShaderMaterial:true,rendererExtensionParallelShaderCompile:false,defines:{}});}
function scenario(config){
 const restores=[];const save=(o,k)=>{const d=Object.getOwnPropertyDescriptor(o,k);restores.push(()=>d?Object.defineProperty(o,k,d):delete o[k]);};
 const trace=[],results=[],sources={},gl={},renderer={},binding={};let entered=false,program,shaderN=0,programN=0,readyN=0,debugN=0;
 const event=(...a)=>{trace.push(a.map(x=>encode(x)));if(trace.length===config.failAt)throw new RangeError('event');};
 const parameters=Object.assign(defaults(),config.parameters);
 const reentry=phase=>{if(config.reentryPhase!==phase||entered)return;entered=true;event('enter',phase);try{if(config.reentryMethod==='construct'){const p=new WebGLProgram(renderer,'nested',defaults(),binding);event('nested',p.id);}else{const fn=program[config.reentryMethod];event('reentry-result',summary(fn.call(config.reentryBare?undefined:program)));}}catch(e){event('reentry-throw',e.name);if(config.reentryRethrow)throw e;}};
 const scalar=(label,value)=>({[Symbol.toPrimitive](hint){event('convert',label,hint);reentry('convert:'+label);return value;}});
 renderer.getContext=function(){event('context',this===renderer);return gl;};
 const debug={checkShaderErrors:!!config.debug};renderer.debug=debug;
 if(config.debugGetters)for(const key of ['checkShaderErrors','onShaderError'])Object.defineProperty(debug,key,{enumerable:true,configurable:true,get(){event('debug-get',key);reentry('debug:'+key);if(key==='checkShaderErrors')return true;if(config.debugMode===1)return 17;if(config.debugMode===2&&debugN++>=1)return 17;return function(...args){event('shader-error',this===debug,args[1],args[2],args[3]);reentry('error-callback');};}});
 binding.releaseStatesOfProgram=function(p){event('release',this===binding,p.id);reentry('release');};
 Object.assign(gl,{VERTEX_SHADER:35633,FRAGMENT_SHADER:35632,LINK_STATUS:35714,VALIDATE_STATUS:35715,COMPILE_STATUS:35713,ACTIVE_ATTRIBUTES:35721,ACTIVE_UNIFORMS:35718,FLOAT_MAT2:35674,FLOAT_MAT3:35675,FLOAT_MAT4:35676});
 for(const name of ['createProgram','createShader','shaderSource','compileShader','attachShader','bindAttribLocation','linkProgram','getProgramInfoLog','getShaderInfoLog','getProgramParameter','getShaderParameter','getShaderSource','getError','deleteShader','deleteProgram','getActiveAttrib','getAttribLocation','getActiveUniform','getUniformLocation']){
  const fn=function(...args){event(name,this===gl,...args);reentry('body:'+name);
   if(name==='createProgram')return {fixtureHandle:'p'+(++programN)};
   if(name==='createShader')return {fixtureHandle:'s'+(++shaderN)};
   if(name==='shaderSource'){Object.defineProperty(sources,args[0].fixtureHandle,{value:args[1],writable:true,enumerable:true,configurable:true});return;}
   if(name==='getProgramInfoLog')return Object.hasOwn(config,'programLog')?config.programLog:'';
   if(name==='getShaderInfoLog')return Object.hasOwn(config,'shaderLog')?config.shaderLog:'';
   if(name==='getProgramParameter'){const p=args[1];if(p===35714)return Object.hasOwn(config,'link')?config.link:true;if(p===35715)return true;if(p===35718)return 0;if(p===35721)return config.attributeProtocol?scalar('count',Object.hasOwn(config,'attributeCount')?config.attributeCount:1):0;if(p===0x91B1)return config.ready?config.ready[readyN++]:true;return;}
   if(name==='getShaderParameter')return Object.hasOwn(config,'compile')?config.compile:true;
   if(name==='getShaderSource')return sources[args[0].fixtureHandle];
   if(name==='getError')return 0;
   if(name==='getActiveAttrib'){const info={};for(const key of ['name','type'])Object.defineProperty(info,key,{enumerable:true,configurable:true,get(){event('attrib-get',key);reentry('attrib:'+key);return key==='name'?scalar('attribute',Object.hasOwn(config,'attributeName')?config.attributeName:'position'):(Object.hasOwn(config,'attributeType')?config.attributeType:35665);}});return info;}
   if(name==='getAttribLocation')return 'location:'+String(args[1]);
  };Object.defineProperty(fn,'name',{value:name,configurable:true});Object.defineProperty(gl,name,{enumerable:true,configurable:true,get(){event('command-get',name);reentry('getter:'+name);return fn;}});
 }
 setConsoleFunction((...a)=>event('console',...a));
 if(config.colorProtocol){parameters.isRawShaderMaterial=false;
  for(const key of ['_getMatrix','getTransfer','getLuminanceCoefficients','workingColorSpace']){save(ColorManagement,key);Object.defineProperty(ColorManagement,key,{enumerable:true,configurable:true,get(){event('cm-get',key);reentry('cm-get:'+key);if(key==='workingColorSpace')return 'working';return function(...args){event('cm-call',key,this===ColorManagement,args[1],args[2]);reentry('cm-call:'+key);if(key==='getTransfer')return Object.hasOwn(config,'transfer')?config.transfer:'srgb';const target=args[0];if(key==='getLuminanceCoefficients'){target.x=.1;target.y=.2;target.z=.3;return target;}if(config.colorMode===1)target.elements=17;else if(config.colorMode===2)target.elements={map(callback){event('map',callback.length);return scalar('mapped','matrix-custom');}};else target.elements=[.1,.2,.3,.4,.5,.6,.7,.8,.9];return target;};}});}
  if(config.fixedProtocol){save(Number.prototype,'toFixed');Object.defineProperty(Number.prototype,'toFixed',{configurable:true,get(){event('fixed-get');return function(...args){event('fixed-call',this,args[0]);reentry('fixed-call');return Object.hasOwn(config,'fixedReturn')?config.fixedReturn:this;};}});}
 }
 if(config.chunkProtocol){save(ShaderChunk,'fixture_advanced');Object.defineProperty(ShaderChunk,'fixture_advanced',{enumerable:true,configurable:true,get(){event('chunk-get');reentry('chunk-get');return Object.hasOwn(config,'chunk')?config.chunk:'chunk NUM_DIR_LIGHTS';}});parameters.vertexShader='#include <fixture_advanced>';}
 if(config.numericProtocol)for(const key of ['numSpotLightShadows','numSpotLightMaps','numSpotLightShadowsWithMaps','numClippingPlanes','numClipIntersection','shadowMapType','combine','envMapMode','toneMapping'])parameters[key]=scalar(key,config.numericValue);
 try{program=new WebGLProgram(renderer,'advanced',parameters,binding);}catch(e){results.push(['construct-throw',e.name]);}
 if(program)for(const name of config.actions){try{const fn=program[name];const receiver=config.bare?undefined:config.other?{id:'other',name:'other-name',type:'other-type'}:program;results.push([name,summary(config.constructMethod?new fn():fn.call(receiver))]);}catch(e){results.push([name,'throw',e.name]);}}
 setConsoleFunction(null);for(const restore of restores.reverse())restore();
 const out={trace,results,sources};if(program){out.id=program.id;out.program=encode(program.program);out.diagnostics=encode(program.diagnostics);if(config.metadata){out.metadata={constructor:metadata(WebGLProgram)};for(const key of ['getUniforms','getAttributes','isReady','destroy'])out.metadata[key]=metadata(program[key]);}}return out;
}
const cases=[];const add=c=>cases.push({actions:['getUniforms','getAttributes','isReady','destroy'],...c});
add({});
for(const colorMode of [0,1,2])for(const transfer of ['srgb','linear',null,17])for(const fixedProtocol of [false,true])add({colorProtocol:true,colorMode,transfer,fixedProtocol});
for(const fixedReturn of [undefined,null,17,'FIXED',{},[]])add({colorProtocol:true,fixedProtocol:true,...(fixedReturn===undefined?{}:{fixedReturn})});
for(const attributeName of ['position','__proto__',null,17,{},[]])for(const attributeType of [35665,35674,35675,35676,null])add({attributeProtocol:true,attributeName,attributeType});
for(const attributeCount of [0,1,2,-1,1.5,null,'2'])add({attributeProtocol:true,attributeCount});
for(const debugMode of [0,1,2])for(const link of [true,false])for(const bare of [false,true])add({debugGetters:true,debugMode,link,bare,shaderLog:'ERROR: 0:2: bad'});
for(const phase of ['getter:getProgramParameter','body:getProgramParameter','body:deleteShader','getter:deleteShader','debug:checkShaderErrors','debug:onShaderError','error-callback','attrib:name','attrib:type','convert:attribute','convert:count','release'])for(const method of ['getUniforms','getAttributes','isReady','destroy'])for(const reentryBare of [false,true])add({debugGetters:true,link:false,attributeProtocol:true,reentryPhase:phase,reentryMethod:method,reentryBare});
for(const reentryPhase of ['cm-get:_getMatrix','cm-get:workingColorSpace','cm-call:_getMatrix','cm-get:getTransfer','cm-call:getTransfer','cm-call:getLuminanceCoefficients','fixed-call','chunk-get','convert:numSpotLightShadows','convert:shadowMapType','convert:toneMapping'])add({colorProtocol:true,chunkProtocol:true,fixedProtocol:true,numericProtocol:true,numericValue:2,reentryPhase,reentryMethod:'construct'});
for(const chunk of ['first\n#include <common>',null,17,{},[],false])add({chunkProtocol:true,chunk});
for(const numericValue of [null,17,'2',{},[],true])for(const isRawShaderMaterial of [true,false])add({numericProtocol:true,numericValue,parameters:{isRawShaderMaterial}});
for(const bare of [false,true])for(const other of [false,true])for(const constructMethod of [false,true])add({debug:true,link:false,programLog:'log',bare,other,constructMethod,metadata:true});
for(const failAt of Array.from({length:75},(_,i)=>i+1))add({colorProtocol:true,fixedProtocol:true,attributeProtocol:true,debugGetters:true,link:false,failAt});
const expected=cases.map(scenario);
if(!probe){console.log(JSON.stringify({oracleOnly:true,scenarios:cases.length}));process.exit(0);}
const run=spawnSync(probe,[],{input:cases.map(c=>JSON.stringify(c)).join('\n')+'\n',encoding:'utf8',maxBuffer:512*1024*1024});if(run.status!==0)throw new Error(`native exited ${run.status}: ${run.stderr}\n${run.error??''}`);
const actual=run.stdout.trim().split(/\r?\n/).map(l=>JSON.parse(l));let comparisons=0;
function compare(a,b,at){comparisons++;if(typeof b!=='object'||b===null){assert.deepEqual(a,b,at);return;}assert.equal(Array.isArray(a),Array.isArray(b),at);assert.deepEqual(Object.keys(a),Object.keys(b),at+' keys');for(const k of Object.keys(b))compare(a[k],b[k],at+'.'+k);}
try{assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`case ${i}`);}catch(e){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-webgl-program-advanced-diff-'));fs.writeFileSync(path.join(dir,'cases.json'),JSON.stringify(cases));fs.writeFileSync(path.join(dir,'expected.json'),JSON.stringify(expected));fs.writeFileSync(path.join(dir,'actual.json'),JSON.stringify(actual));console.error(`Diagnostic files: ${dir}`);throw e;}
console.log(`WebGLProgram advanced: ${cases.length} scenarios / ${comparisons} comparisons passed (${revision}).`);
