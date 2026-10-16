import assert from 'node:assert/strict';
import {execFileSync,spawnSync} from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const upstream=path.resolve(process.argv[2]);
const probe=process.argv[3]&&path.resolve(process.argv[3]);
const revision='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),revision);
const file='src/renderers/webgl/WebGLProgram.js';
const source=execFileSync('git',['-C',upstream,'show',`HEAD:${file}`],{encoding:'utf8'});
assert.equal(fs.readFileSync(path.join(upstream,file),'utf8').replaceAll('\r\n','\n'),source);
const {WebGLProgram}=await import(pathToFileURL(path.join(upstream,file)));
const {setConsoleFunction}=await import(pathToFileURL(path.join(upstream,'src/utils.js')));
const parameterKeys=[...new Set([...source.matchAll(/parameters\.(\w+)/g)].map(m=>m[1]))];
function encode(v,depth=0){
  if(v===undefined)return '@undefined';
  if(typeof v==='symbol')return '@symbol';
  if(typeof v==='number')return Number.isNaN(v)?'@NaN':!Number.isFinite(v)?(v>0?'@Infinity':'@-Infinity'):Object.is(v,-0)?'@-0':v;
  if(typeof v==='function')return `@function:${v.name}:${v.length}`;
  if(v===null||typeof v!=='object')return v;
  if(Object.hasOwn(v,'fixtureHandle'))return v.fixtureHandle;
  if(depth>5)return '@object';
  const result=Array.isArray(v)?[]:{};
  for(const k of Object.keys(v))Object.defineProperty(result,k,{value:encode(v[k],depth+1),writable:true,enumerable:true,configurable:true});
  return result;
}
function defaults(){
  const p={};
  for(const key of ['numSunLights','numDirLights','numSpotLights','numSpotLightMaps','numSpotLightShadows','numSpotLightShadowsWithMaps','numRectAreaLights','numPointLights','numHemiLights','numSunLightShadows','numDirLightShadows','numPointLightShadows','numClippingPlanes','numClipIntersection'])p[key]=0;
  Object.assign(p,{precision:'highp',shaderType:'FixtureMaterial',shaderName:'fixture',vertexShader:'void main() { gl_Position = vec4( 0.0 ); }',fragmentShader:'void main() { gl_FragColor = vec4( 1.0 ); }',toneMapping:0,outputColorSpace:'srgb',envMap:false,isRawShaderMaterial:true,rendererExtensionParallelShaderCompile:false,defines:{}});
  return p;
}
function scenario(config){
  const restores=[];
  const save=(o,k)=>{const d=Object.getOwnPropertyDescriptor(o,k);restores.push(()=>d?Object.defineProperty(o,k,d):delete o[k]);};
  const trace=[],results=[],sources={};let shaderCount=0,programCount=0,readyCount=0;
  const gl={},renderer={},binding={};
  const event=(...values)=>{trace.push(values.map(v=>encode(v)));if(trace.length===config.failAt)throw new RangeError('event');};
  const parameters=Object.assign(defaults(),config.parameters);
  for(const key of config.undefinedParameters??[])delete parameters[key];
  if(config.sourceProtocol)for(const label of ['vertex','fragment']){
    const o={};Object.defineProperty(o,'replace',{enumerable:true,configurable:true,get(){event('source-get-replace',label);return function replace(regex,replacement){event('source-replace',label,this===o,regex.source,regex.flags,replacement);return this;};}});
    Object.defineProperty(o,Symbol.toPrimitive,{value:function(hint){event('source-convert',label,hint);return 'source:'+label;},writable:true,configurable:true});parameters[label+'Shader']=o;
  }
  for(const [key,values] of Object.entries(config.parameterSequences??{})){let n=0;Object.defineProperty(parameters,key,{enumerable:true,configurable:true,get(){event('sequence',key,n);return values[n++];}});}
  if(config.traceParameters)for(const name of config.parameterKeys){const value=parameters[name];Object.defineProperty(parameters,name,{enumerable:true,configurable:true,get(){event('parameter',name);return value;}});}
  const debug={checkShaderErrors:config.checkShaderErrors};renderer.debug=debug;
  if(config.shaderErrorCallback)debug.onShaderError=function(...args){event('onShaderError',this===debug,args[1],args[2],args[3]);};
  renderer.getContext=function(){event('getContext',this===renderer);return gl;};
  binding.releaseStatesOfProgram=function(program){event('release',this===binding,program.id);};
  Object.assign(gl,{VERTEX_SHADER:35633,FRAGMENT_SHADER:35632,LINK_STATUS:35714,VALIDATE_STATUS:35715,COMPILE_STATUS:35713,ACTIVE_ATTRIBUTES:35721,ACTIVE_UNIFORMS:35718,FLOAT_MAT2:35674,FLOAT_MAT3:35675,FLOAT_MAT4:35676});
  for(const name of ['createProgram','createShader','shaderSource','compileShader','attachShader','bindAttribLocation','linkProgram','getProgramInfoLog','getShaderInfoLog','getProgramParameter','getShaderParameter','getShaderSource','getError','deleteShader','deleteProgram','getActiveAttrib','getAttribLocation','getActiveUniform','getUniformLocation']){
    const fn=function(...args){
      event(name,this===gl,...args);
      if(name==='createProgram')return config.nullProgram?(++programCount,null):{fixtureHandle:`program:${++programCount}`};
      if(name==='createShader')return {fixtureHandle:`shader:${++shaderCount}`};
      if(name==='shaderSource'){sources[args[0].fixtureHandle]=args[1];return;}
      if(name==='getProgramInfoLog')return Object.hasOwn(config,'programLog')?config.programLog:'';
      if(name==='getShaderInfoLog'){const field=args[0].fixtureHandle==='shader:1'?'vertexLog':'fragmentLog';return Object.hasOwn(config,field)?config[field]:'';}
      if(name==='getProgramParameter'){
        const p=args[1];
        if(p===35714)return Object.hasOwn(config,'linkStatus')?config.linkStatus:true;
        if(p===35715)return Object.hasOwn(config,'validateStatus')?config.validateStatus:true;
        if(p===35721)return config.attributes?.length??0;
        if(p===35718)return config.uniforms?.length??0;
        if(p===0x91B1)return Object.hasOwn(config,'readyValues')?config.readyValues[readyCount++]:true;
        return;
      }
      if(name==='getShaderParameter')return Object.hasOwn(config,'compileStatus')?config.compileStatus:true;
      if(name==='getShaderSource')return sources[args[0].fixtureHandle];
      if(name==='getError')return Object.hasOwn(config,'error')?config.error:0;
      if(name==='getActiveAttrib')return config.attributes[args[1]];
      if(name==='getAttribLocation')return `attribute:${args[1]}`;
      if(name==='getActiveUniform')return config.uniforms[args[1]];
      if(name==='getUniformLocation')return `uniform:${args[1]}`;
    };
    Object.defineProperty(fn,'name',{value:name,configurable:true});
    if(config.traceCommands)Object.defineProperty(gl,name,{enumerable:true,configurable:true,get(){event('command',name);return fn;}});
    else gl[name]=fn;
  }
  setConsoleFunction((...args)=>event('console',...args));
  if(config.arrayProtocol)for(const key of ['filter','join']){
    save(Array.prototype,key);const original=Array.prototype[key];
    const method=function(arg){event('array-call',key,this.length,arg);if(key==='filter'&&config.mutateAtFilter){parameters.precision='lowp';parameters.shaderName='changed-filter';}return Reflect.apply(original,this,arguments);};
    Object.defineProperty(method,'name',{value:key,configurable:true});
    Object.defineProperty(Array.prototype,key,{configurable:true,get(){event('array-get',key);return method;}});
  }
  if(config.stringProtocol)for(const key of ['replace','split','trim','toUpperCase']){
    save(String.prototype,key);const original=String.prototype[key];
    const method=function(...args){const first=args[0]!==null&&typeof args[0]==='object'&&'source' in args[0]?[args[0].source,args[0].flags]:args[0];event('string-call',key,this,first,args[1]);if(config.stringReturns&&Object.hasOwn(config.stringReturns,key))return config.stringReturns[key];return Reflect.apply(original,this,args);};
    Object.defineProperty(method,'name',{value:key,configurable:true});Object.defineProperty(method,'length',{value:original.length,configurable:true});
    Object.defineProperty(String.prototype,key,{configurable:true,get(){event('string-get',key);return method;}});
  }
  if(config.constructorSetters)for(const key of ['getUniforms','getAttributes','isReady','destroy','type','name','id','program']){
    save(WebGLProgram.prototype,key);
    Object.defineProperty(WebGLProgram.prototype,key,{configurable:true,set(value){
      event('constructor-set',key,value);
      if(key==='getUniforms'&&config.earlyUniforms){try{event('early-return',typeof value.call(this)==='object');}catch(e){event('early-throw',e.name);if(config.earlyRethrow)throw e;}}
      Object.defineProperty(this,key,{value,writable:true,enumerable:true,configurable:true});
    }});
  }
  let program;
  try{
    if(config.constructMode===1)program=WebGLProgram.call({},renderer,'cache',parameters,binding);
    else if(config.constructMode===2)program=WebGLProgram.call(undefined,renderer,'cache',parameters,binding);
    else if(config.constructMode===3)program=Reflect.construct(WebGLProgram,[renderer,'cache',parameters,binding],function Other(){throw new RangeError('newTarget body');});
    else program=new WebGLProgram(renderer,'cache',parameters,binding);
  }catch(e){results.push(['constructThrow',e.name]);}
  if(program)for(const name of config.actions){
    try{
      let result=config.constructMethods?new program[name]():program[name]();
      if(name==='getUniforms')result=result.seq.map(u=>[u.id,u.setValue.name,u.setValue.length]);
      results.push([name,encode(result)]);
    }catch(e){results.push([name,'throw',e.name]);}
  }
  setConsoleFunction(null);
  for(const restore of restores.reverse())restore();
  const out={trace,results,sources};
  if(program){
    const snapshot={};
    for(const key of ['type','name','id','cacheKey','usedTimes','program','vertexShader','fragmentShader','diagnostics'])snapshot[key]=encode(program[key]);
    snapshot.methods=['getUniforms','getAttributes','isReady','destroy'].map(key=>[key,program[key].name,program[key].length,Object.hasOwn(program[key],'prototype'),true]);
    out.program=snapshot;
  }
  return out;
}
const cases=[];
const add=c=>cases.push({parameters:{},actions:['isReady','getUniforms','getAttributes','getUniforms','isReady','destroy'],...c});
add({});add({parameters:{isRawShaderMaterial:false}});
for(const precision of ['highp','mediump','lowp','custom',null,0])add({parameters:{isRawShaderMaterial:false,precision}});
for(const toneMapping of [0,1,2,3,4,5,6,7,8,99,'toString'])add({parameters:{isRawShaderMaterial:false,toneMapping}});
for(const outputColorSpace of ['srgb','srgb-linear','display-p3','display-p3-linear','',null,'bad'])add({parameters:{isRawShaderMaterial:false,outputColorSpace}});
for(const isRawShaderMaterial of [false,true,0,1,'raw',null,{},[]])for(const glslVersion of ['',null,'100','300 es'])add({parameters:{isRawShaderMaterial,glslVersion}});
const flags=parameterKeys.filter(k=>!['defines','vertexShader','fragmentShader','precision','shaderType','shaderName','glslVersion','outputColorSpace'].includes(k));
for(const key of flags)for(const value of [true,false,null,2,'uv1'])add({parameters:{isRawShaderMaterial:false,[key]:value}});
for(const defines of [{A:1,B:false,C:true,D:null,E:'abc',F:0},{'3':3,'1':1,'2':2,a:[],b:{}},null,17,'abc'])add({parameters:{defines}});
for(const shader of ['NUM_SUN_LIGHTS NUM_DIR_LIGHTS NUM_SPOT_LIGHTS NUM_SPOT_LIGHT_MAPS NUM_SPOT_LIGHT_COORDS NUM_RECT_AREA_LIGHTS NUM_POINT_LIGHTS NUM_HEMI_LIGHTS NUM_SUN_LIGHT_SHADOWS NUM_DIR_LIGHT_SHADOWS NUM_SPOT_LIGHT_SHADOWS_WITH_MAPS NUM_SPOT_LIGHT_SHADOWS NUM_POINT_LIGHT_SHADOWS NUM_CLIPPING_PLANES UNION_CLIPPING_PLANES',
 '#include <common>\n#include <packing>', '#include <missing_fixture_chunk>', '  #include <common>tail\n\t#include <common>',
 '#pragma unroll_loop_start\nfor ( int i = 0; i < 3; i ++ ) {\nx[i] += UNROLLED_LOOP_INDEX;\n}\n#pragma unroll_loop_end',
 '#pragma unroll_loop_start for(int i=2;i<2;i++){ unused[i] } #pragma unroll_loop_end', '',null,17,{},[]])add({parameters:{vertexShader:shader,fragmentShader:shader,numSpotLightShadows:7,numSpotLightMaps:4,numSpotLightShadowsWithMaps:2,numClippingPlanes:5,numClipIntersection:2}});
for(const linkStatus of [true,false,0,null,''])for(const programLog of ['',' note ',null,17])add({checkShaderErrors:true,linkStatus,programLog,vertexLog:' vertex note ',fragmentLog:' fragment note '});
for(const vertexLog of ['', 'warning','ERROR: 0:3: syntax','  ERROR: 0:0: syntax  ','ERROR: 0:0x10: syntax','ERROR: 0:999999999999999999999: syntax'])for(const shaderErrorCallback of [false,true])add({checkShaderErrors:true,linkStatus:false,compileStatus:false,vertexLog,fragmentLog:'ERROR: 0:2: bad',shaderErrorCallback});
for(const readyValues of [[false,false,true],[null,false],[0,false],['',true],[],[1,false]])add({parameters:{rendererExtensionParallelShaderCompile:true},actions:['isReady','isReady','isReady'],readyValues});
for(const constructMode of [0,1,2,3])for(const constructMethods of [false,true])add({constructMode,constructMethods});
add({attributes:[{name:'position',type:35665},{name:'m2',type:35674},{name:'m3',type:35675},{name:'m4',type:35676},{name:'__proto__',type:5126}]});
add({uniforms:[{name:'u',type:5126,size:1},{name:'v',type:35665,size:1},{name:'color[0]',type:35665,size:3},{name:'lights[0].power',type:5126,size:1}],actions:['getAttributes','getUniforms','getAttributes','destroy']});
add({nullProgram:true});
for(const isRawShaderMaterial of [false,true])for(const mutateAtFilter of [false,true])add({parameters:{isRawShaderMaterial},arrayProtocol:true,mutateAtFilter});
for(const sourceProtocol of [true])for(const isRawShaderMaterial of [false,true])add({parameters:{isRawShaderMaterial},sourceProtocol});
for(const earlyUniforms of [false,true])for(const earlyRethrow of [false,true])for(const checkShaderErrors of [false,true])add({constructorSetters:true,earlyUniforms,earlyRethrow,checkShaderErrors});
for(const isRawShaderMaterial of [false,true])add({parameters:{isRawShaderMaterial},stringProtocol:true,checkShaderErrors:true,linkStatus:false,compileStatus:false,vertexLog:'ERROR: 0:2: failure'});
for(const stringReturns of [{replace:17},{trim:17},{trim:null},{trim:' ERROR: 0:1 '},{split:{length:2,0:'first',1:'second'}},{toUpperCase:'changed'}, {toUpperCase:null}])add({stringProtocol:true,stringReturns,checkShaderErrors:true,linkStatus:false,compileStatus:false,vertexLog:'ERROR: 0:2: failure'});
for(const parameterSequences of [
 {isRawShaderMaterial:[true,false]}, {isRawShaderMaterial:[false,true]}, {glslVersion:['100','300 es','100','300 es']},
 {envMap:[false,true,true,true,true,true,true,true]}, {toneMapping:[0,1,2,3]}, {precision:Array.from({length:40},(_,i)=>i%2?'mediump':'highp')},
 {index0AttributeName:['first','second']}, {shaderType:['a','b','c'],shaderName:['a','b','c']},
])add({parameters:{isRawShaderMaterial:false},parameterSequences});
for(const failAt of Array.from({length:190},(_,i)=>i+1))add({traceParameters:true,parameterKeys,traceCommands:true,checkShaderErrors:true,failAt});
for(const failAt of Array.from({length:90},(_,i)=>i+1))add({parameters:{isRawShaderMaterial:false},traceParameters:true,parameterKeys,traceCommands:true,checkShaderErrors:true,failAt});
const expected=cases.map(scenario);
if(!probe){console.log(JSON.stringify({oracleOnly:true,scenarios:cases.length,parameterFields:parameterKeys.length}));process.exit(0);}
const run=spawnSync(probe,[],{input:cases.map(c=>JSON.stringify(c)).join('\n')+'\n',encoding:'utf8',maxBuffer:512*1024*1024});
if(run.status!==0)throw new Error(`native exited ${run.status}: ${run.stderr}\n${run.error??''}`);
const actual=run.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));
let comparisons=0;
function compare(a,b,at){comparisons++;if(typeof b!=='object'||b===null){assert.deepEqual(a,b,at);return;}assert.equal(Array.isArray(a),Array.isArray(b),at);assert.deepEqual(Object.keys(a),Object.keys(b),at+' keys');for(const k of Object.keys(b))compare(a[k],b[k],at+'.'+k);}
try{assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`case ${i}`);}
catch(e){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-webgl-program-diff-'));fs.writeFileSync(path.join(dir,'cases.json'),JSON.stringify(cases));fs.writeFileSync(path.join(dir,'expected.json'),JSON.stringify(expected));fs.writeFileSync(path.join(dir,'actual.json'),JSON.stringify(actual));console.error(`Diagnostic files: ${dir}`);throw e;}
console.log(`WebGLProgram: ${cases.length} scenarios / ${comparisons} comparisons passed (${revision}).`);
