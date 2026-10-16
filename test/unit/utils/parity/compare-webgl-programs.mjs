import assert from 'node:assert/strict';
import {execFileSync,spawnSync} from 'node:child_process';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]),probe=process.argv[3]&&path.resolve(process.argv[3]);
const revision='11738a9d55e502fb484cefffcea3a66dc44e9fe9';
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),revision);
const file='src/renderers/webgl/WebGLPrograms.js';
const source=execFileSync('git',['-C',upstream,'show',`HEAD:${file}`],{encoding:'utf8'});
assert.equal(fs.readFileSync(path.join(upstream,file),'utf8').replaceAll('\r\n','\n'),source);
const load=async file=>import(pathToFileURL(path.join(upstream,file)));
const {WebGLPrograms}=await load(file);
const {ShaderLib}=await load('src/renderers/shaders/ShaderLib.js');
const {UniformsUtils}=await load('src/renderers/shaders/UniformsUtils.js');
const {ColorManagement}=await load('src/math/ColorManagement.js');
const {Layers}=await load('src/core/Layers.js');
const {setConsoleFunction}=await load('src/utils.js');
const materialKeys=[...new Set([...source.matchAll(/material\.(\w+)/g)].map(m=>m[1]))];
const parameterKeys=[...new Set([...source.matchAll(/parameters\.(\w+)/g)].map(m=>m[1]))];
const readKeys={material:materialKeys,geometry:['morphAttributes','attributes'],attributes:['position','normal','tangent','color','uv'],morph:['position','normal','color'],object:['geometry','isInstancedMesh','isBatchedMesh','instanceColor','morphTexture','_colorsTexture','isPoints','isSkinnedMesh'],scene:['fog','environment'],lights:['sun','directional','point','spot','spotLightMap','rectArea','hemi','sunShadowMap','directionalShadowMap','pointShadowMap','spotShadowMap','numSpotLightShadowsWithMaps','numLightProbes'],renderer:['outputColorSpace','toneMapping','shadowMap','state','getRenderTarget'],capabilities:['precision','logarithmicDepthBuffer','getMaxPrecision'],clipping:['numPlanes','numIntersection'],environments:['get'],extensions:['has'],depth:['getReversed']};
function data(o,k,v){Object.defineProperty(o,k,{value:v,writable:true,enumerable:true,configurable:true});}
function encode(v,depth=0){
  if(v===undefined)return '@undefined';if(typeof v==='symbol')return '@symbol';
  if(typeof v==='number')return Number.isNaN(v)?'@NaN':!Number.isFinite(v)?(v>0?'@Infinity':'@-Infinity'):Object.is(v,-0)?'@-0':v;
  if(typeof v==='function')return `@function:${v.name}:${v.length}`;
  if(v===null||typeof v!=='object')return v;
  if(Object.hasOwn(v,'fixtureHandle'))return v.fixtureHandle;if(depth>6)return '@object';
  const r=Array.isArray(v)?[]:{};for(const k of Object.keys(v))data(r,k,encode(v[k],depth+1));return r;
}
function scenario(config){
 const restores=[],trace=[],results=[];const save=(o,k)=>{const d=Object.getOwnPropertyDescriptor(o,k);data(restores,restores.length,()=>d?Object.defineProperty(o,k,d):delete o[k]);};
 const event=(...a)=>{data(trace,trace.length,a.map(v=>encode(v)));if(trace.length===config.failAt)throw new RangeError('event');};
 const attempt=fn=>{try{return encode(fn());}catch(e){return ['throw',e.name];}};
 let nestedDone=false,module,nestedArgs;
 const nestedParameters=point=>{if(config.nestedAt!==point||nestedDone)return;nestedDone=true;const p=module.getParameters(...nestedArgs);event('nested-parameters',p.mapUv,p.vertexUv1s,p.vertexUv2s);};
 const material={type:'MeshBasicMaterial',name:'fixture',precision:null,wireframe:false,transparent:false,blending:1,alphaToCoverage:false,normalMapType:0,toneMapped:true,side:0,fog:true,sizeAttenuation:true,vertexShader:'vertex source',fragmentShader:'fragment source',uniforms:{x:{value:3}},customProgramCacheKey(){event('customKey',this===material);nestedParameters('customKey');return Object.hasOwn(config,'customKey')?config.customKey:'custom-key';}};
 const attributes={position:{itemSize:3},normal:{itemSize:3}},geometry={attributes,morphAttributes:{}},object={geometry,instanceColor:null,morphTexture:null,_colorsTexture:null};
 const scene={fog:null,environment:null},lights={};for(const key of ['sun','directional','point','spot','spotLightMap','rectArea','hemi','sunShadowMap','directionalShadowMap','pointShadowMap','spotShadowMap'])lights[key]=[];
 lights.numSpotLightShadowsWithMaps=0;lights.numLightProbes=0;
 const depth={getReversed(){event('getReversed',this===depth);return Object.hasOwn(config,'reversed')?config.reversed:false;}};
 const renderer={outputColorSpace:'srgb',toneMapping:4,shadowMap:{enabled:false,type:1},state:{buffers:{depth}},getRenderTarget(){event('getRenderTarget',this===renderer);return Object.hasOwn(config,'target')?config.target:null;}};
 const environments={get(value,usePMREM){event('environment',this===environments,value,usePMREM);return Object.hasOwn(config,'envMap')?config.envMap:value;}};
 const extensions={has(name){event('extension',this===extensions,name);return config.extensions&&typeof config.extensions==='object'?config.extensions[name]:false;}};
 const capabilities={precision:'highp',logarithmicDepthBuffer:false,getMaxPrecision(value){event('precision',this===capabilities,value);return Object.hasOwn(config,'maxPrecision')?config.maxPrecision:value;}};
 const binding={},clipping={numPlanes:0,numIntersection:0};
 for(const [label,target]of Object.entries({material,attributes,geometry,object,scene,lights,renderer,capabilities,clipping}))if(config[label]&&typeof config[label]==='object')for(const key of Object.keys(config[label]))data(target,key,config[label][key]);
 for(const key of config.deleteMaterial??[])delete material[key];
 for(const key of Object.keys(ShaderLib)){save(ShaderLib,key);data(ShaderLib,key,{vertexShader:'vertex:'+key,fragmentShader:'fragment:'+key,uniforms:{x:{value:7}}});}
 setConsoleFunction((...a)=>event('console',...a));
 if(Object.hasOwn(config,'workingColorSpace')){save(ColorManagement,'workingColorSpace');data(ColorManagement,'workingColorSpace',config.workingColorSpace);}
 if(Object.hasOwn(config,'transferResult')){save(ColorManagement,'getTransfer');const fn=function getTransfer(value){event('transfer-call',this===ColorManagement,value);return config.transferResult;};Object.defineProperty(ColorManagement,'getTransfer',{enumerable:true,configurable:true,get(){event('transfer-method',this===ColorManagement);return fn;}});}
 const objects={material,geometry,attributes,morph:geometry.morphAttributes,object,scene,lights,renderer,capabilities,clipping,environments,extensions,depth};
 for(const [group,key]of config.symbolFields??[])data(objects[group],key,Symbol('fixture'));
 if(config.symbolChannel)data(material,'map',{channel:Symbol('channel')});
 function traced(o,k,label,sequence){const value=o[k];let n=0;Object.defineProperty(o,k,{enumerable:true,configurable:true,get(){event('get',label,this===o);return sequence===undefined?value:sequence[n++];}});}
 if(config.traceReads)for(const group of Object.keys(config.readKeys))for(const key of config.readKeys[group])traced(objects[group],key,group+'.'+key);
 for(const group of Object.keys(config.sequences??{}))for(const key of Object.keys(config.sequences[group]))traced(objects[group],key,group+'.'+key,config.sequences[group][key]);
 for(const group of Object.keys(config.conversion??{}))for(const key of Object.keys(config.conversion[group])){
   const spec=config.conversion[group][key],value={};Object.defineProperty(value,Symbol.toPrimitive,{value:function(hint){event('convert',group+'.'+key,hint,this===value);if(spec.throws)throw new RangeError('convert');return Object.hasOwn(spec,'value')?spec.value:1;},writable:true,configurable:true});data(objects[group],key,value);
 }
 try{
   try{module=WebGLPrograms(renderer,environments,extensions,capabilities,binding,clipping);}catch(e){results.push(['throw',e.name]);return {results,trace};}
   if(Object.hasOwn(config,'nestedAt'))nestedArgs=[{type:'MeshBasicMaterial',name:'nested',precision:null,toneMapped:false,map:{channel:2},wireframe:false,customProgramCacheKey(){return 'nested';}},lights,[],scene,object,[]];
   if(config.kind==='setProtocol'||config.kind==='layersProtocol'){
     const proto=config.kind==='setProtocol'?Set.prototype:Layers.prototype,methods=config.kind==='setProtocol'?['add','has','clear']:['disableAll','enable'];let nestedDone=false;
     for(const method of methods){
       save(proto,method);const original=proto[method],fn=function(...args){
         event('protocol-call',method,args[0]);if(method==='add')nestedParameters('add');if(Object.hasOwn(config,'hasResult')&&method==='has')return config.hasResult;if(config.ignoreAdd&&method==='add')return this;
         if(config.nestedKey&&method==='enable'&&!nestedDone){nestedDone=true;event('nested-key',module.getProgramCacheKey({shaderID:'nested',isRawShaderMaterial:false,fog:true,instancingColor:true}));}
         return Reflect.apply(original,this,args);
       };Object.defineProperty(fn,'name',{value:method,configurable:true});
       Object.defineProperty(proto,method,{enumerable:false,configurable:true,get(){event('protocol-method',method);return fn;}});
     }
   }
   if(config.kind==='lifecycle'){
     const gl={VERTEX_SHADER:35633,FRAGMENT_SHADER:35632};let programCount=0,shaderCount=0;
     for(const method of ['createProgram','createShader','shaderSource','compileShader','attachShader','bindAttribLocation','linkProgram','deleteProgram'])gl[method]=function(...args){event('gl',method,this===gl,...args);if(method==='createProgram')return {fixtureHandle:'program:'+(++programCount)};if(method==='createShader')return {fixtureHandle:'shader:'+(++shaderCount)};};
     renderer.getContext=function(){event('getContext',this===renderer);return gl;};
     binding.releaseStatesOfProgram=function(program){event('releaseBinding',this===binding,program.id);};
     const parameters={isRawShaderMaterial:true,shaderType:'Fixture',shaderName:'fixture',defines:{},vertexShader:'void main() { gl_Position = vec4( 0.0 ); }',fragmentShader:'void main() { gl_FragColor = vec4( 1.0 ); }',rendererExtensionParallelShaderCompile:false};
     const programsByKey={};
     for(const action of config.actions){
       if(action==='recover')config.failAt=0;
       results.push(attempt(()=>{
         if(action==='dispose')return module.dispose();if(action==='recover')return;
         const key=action.slice(-1);
         if(action.startsWith('acquire')){const p=module.acquireProgram(parameters,key),old=programsByKey[key];programsByKey[key]=p;return [p.id,p.cacheKey,p.usedTimes,old===p,p.program];}
         return module.releaseProgram(programsByKey[key]);
       }));
       results.push(module.programs.map(p=>[p.id,p.usedTimes,encode(p.program)]));
     }
   }else if(config.kind==='cacheProtocol'){
     const program={usedTimes:Object.hasOwn(config,'usedTimes')?config.usedTimes:1,cacheKey:'cache',destroy(){event('destroy',this===program,this.usedTimes);}};
     if(config.countConversion){const count={};Object.defineProperty(count,Symbol.toPrimitive,{value:function(hint){event('count-convert',hint);return config.usedTimes;},writable:true,configurable:true});program.usedTimes=count;}
     if(config.countAccessor){let value=program.usedTimes;Object.defineProperty(program,'usedTimes',{enumerable:true,configurable:true,get(){event('count-get',this===program);return value;},set(v){event('count-set',this===program,v);value=v;}});}
     for(const method of ['get','delete']){
       save(Map.prototype,method);const fn=function(key){event('map-call',method,this instanceof Map,key);if(method==='get')return program;return true;};Object.defineProperty(fn,'name',{value:method,configurable:true});
       Object.defineProperty(Map.prototype,method,{enumerable:false,configurable:true,get(){event('map-method',method,this instanceof Map);return fn;}});
     }
     if(config.operation==='acquire')results.push(attempt(()=>{const p=module.acquireProgram({},'cache');return [p===program,p.usedTimes];}));
     else{
       const array=module.programs;array.push(program);if(config.arrayAlias)module.programs=[];if(config.missingProgram)array.length=0;
       results.push(attempt(()=>module.releaseProgram(program)));results.push(attempt(()=>program.usedTimes));results.push(array.length);
     }
   }else if(config.kind==='arrayProtocol'){
     for(const method of ['push','join']){
       save(Array.prototype,method);const original=Array.prototype[method],fn=function(...args){event('array-call',method,this.length,args[0]);if(config.replaceJoin&&method==='join')return config.joinResult;return Reflect.apply(original,this,args);};Object.defineProperty(fn,'name',{value:method,configurable:true});
       Object.defineProperty(Array.prototype,method,{enumerable:false,configurable:true,get(){event('array-method',method,this.length);return fn;}});
     }
     data(results,results.length,attempt(()=>module.getProgramCacheKey(config.parameters)));
   }else if(config.kind==='layersProtocol')results.push(attempt(()=>module.getProgramCacheKey(config.parameters)));
   else if(config.kind==='metadata')for(const key of Object.keys(module)){
     const value=module[key],row=[key,encode(value)];if(typeof value==='function'){
       let constructible;try{Reflect.construct(String,[],value);constructible=true;}catch{constructible=false;}
       const d=Object.getOwnPropertyDescriptor(value,'prototype');row.push(constructible,Object.getOwnPropertyNames(value),[d.writable,d.enumerable,d.configurable,value.prototype.constructor===value]);
     }results.push(row);
   }else if(config.kind==='key'){
     const parameters=config.parameters;if(config.definesPrototype&&typeof config.definesPrototype==='object')Object.setPrototypeOf(parameters.defines,config.definesPrototype);
     if(Object.hasOwn(config,'symbolParameter'))data(parameters,config.symbolParameter,Symbol('parameter'));
     if(config.mutateDefines){const defines=parameters.defines;Object.defineProperty(defines,'a',{enumerable:true,configurable:true,get(){event('define-get');delete defines.b;data(defines,'new',9);if(config.replaceDefines)data(parameters,'defines',{a:5,b:6,c:7,inherited:8});return 1;}});}
     if(config.traceReads)for(const key of config.parameterKeys)traced(parameters,key,'parameters.'+key);
     results.push(attempt(()=>module.getProgramCacheKey(parameters)));
   }else if(config.kind==='construct'){
     const fn=module[config.method],marker={marker:1};let Target=function Target(){};
     if(config.customTarget)Target.prototype=marker;
     const values=config.method==='getProgramCacheKey'?[config.parameters]:config.method==='getUniforms'?[material]:[];
     results.push(attempt(()=>{const result=Reflect.construct(fn,values,config.customTarget?Target:fn);return {object:typeof result==='object'&&result!==null,defaultPrototype:Object.getPrototypeOf(result)===fn.prototype,customPrototype:Object.getPrototypeOf(result)===marker,uniformAlias:result===material.uniforms};}));
   }else if(config.kind==='uniforms')results.push(attempt(()=>module.getUniforms(material)));
   else for(let i=0;i<(Object.hasOwn(config,'repeat')?config.repeat:1);i++){
     if(i>0){config.failAt=0;for(const key of Object.keys(config.recoveryMaterial??{}))data(material,key,config.recoveryMaterial[key]);}
     results.push(attempt(()=>{const p=module.getParameters(material,lights,Object.hasOwn(config,'shadows')?config.shadows:[],scene,object,Object.hasOwn(config,'lightProbeGrids')?config.lightProbeGrids:[]);const r={parameters:encode(p)};r.key=module.getProgramCacheKey(p);return r;}));
   }
   return {results,trace};
 }finally{setConsoleFunction(null);for(const restore of restores.reverse())restore();}
}
const cases=[];const add=c=>cases.push({readKeys,parameterKeys,...c});
add({});add({traceReads:true});add({kind:'metadata'});
const types=['MeshDepthMaterial','MeshDistanceMaterial','MeshNormalMaterial','MeshBasicMaterial','MeshLambertMaterial','MeshPhongMaterial','MeshToonMaterial','MeshStandardMaterial','MeshPhysicalMaterial','MeshMatcapMaterial','LineBasicMaterial','LineDashedMaterial','PointsMaterial','ShadowMaterial','SpriteMaterial','RawShaderMaterial','Custom','toString','constructor','__proto__'];
for(const type of types){add({material:{type}});add({kind:'uniforms',material:{type}});}
for(const key of materialKeys)if(!['type','name','vertexShader','fragmentShader','customProgramCacheKey'].includes(key))for(const value of [null,false,true,0,1,'', 'enabled',{},[]])add({material:{[key]:value}});
for(const key of ['anisotropy','clearcoat','diffuseRoughness','dispersion','retroreflectivity','iridescence','sheen','transmission','alphaTest','depthPacking'])for(const value of [-1,0,0.5,'2','invalid',null,[],[2]])add({material:{[key]:value}});
const maps=materialKeys.filter(k=>k.endsWith('Map')||k==='map');
for(const key of maps)for(const channel of [0,1,2,3,4,'1',null,false,{},[],[2]])add({material:{anisotropy:1,clearcoat:1,diffuseRoughness:1,iridescence:1,sheen:1,transmission:1,[key]:{channel}},envMap:null});
for(const format of [1030,37490,36285,36286,1])for(const normalMapType of [0,1])add({material:{normalMap:{format,channel:2},normalMapType}});
for(const target of [null,{}, {isXRRenderTarget:true,texture:{colorSpace:'display-p3'}},{isXRRenderTarget:1,texture:{colorSpace:'display-p3'}}])for(const workingColorSpace of ['srgb-linear','display-p3-linear',null,7,{}])add({target,workingColorSpace});
for(const isPMREMTexture of [false,true,1])for(const isCompressedCubeTexture of [false,true])add({envMap:{isPMREMTexture,isCompressedCubeTexture,mapping:302,mipmaps:[{width:64},{width:32}],image:[{mipmaps:[{width:128}]}]}});
for(const group of ['position','normal','color'])for(const value of [[],[{},{}],null,false,0,{},'abc'])add({geometry:{morphAttributes:{[group]:value}}});
for(const name of ['isInstancedMesh','isBatchedMesh','isPoints','isSkinnedMesh'])for(const value of [false,true,1,'yes'])add({object:{[name]:value},material:{map:{channel:3}},attributes:{uv:{},color:{itemSize:4}}});
for(const flag of ['isMeshLambertMaterial','isMeshPhongMaterial','isMeshStandardMaterial','isMeshPhysicalMaterial'])for(const value of [false,true,1,'yes'])add({material:{[flag]:value},attributes:{normal:null}});
for(const shadows of [[],[{}],null,{},'shadows'])add({renderer:{shadowMap:{enabled:true,type:2}},shadows});
for(const precision of [null,'lowp','mediump','highp',1])for(const maxPrecision of ['lowp',null,7])add({material:{precision},maxPrecision});
for(const key of ['anisotropy','clearcoat','alphaTest','depthPacking','type','precision'])for(const value of [1,'2',null,{}])add({conversion:{material:{[key]:{value}}}});
for(const key of ['map','normalMap','alphaMap','precision','type','side','wireframe','isMeshStandardMaterial'])add({sequences:{material:{[key]:[null,{channel:2},true,false,'enabled']}}});
for(const key of ['normal','position','color'])add({sequences:{attributes:{[key]:[null,{},null,{itemSize:4}]}}});
for(const key of ['map','emissiveMap'])for(const isVideoTexture of [false,true,1])for(const colorSpace of ['srgb','srgb-linear','display-p3','invalid',null,{}])add({material:{[key]:{channel:1,isVideoTexture,colorSpace}}});
for(const key of ['map','emissiveMap'])for(const transferResult of ['srgb','linear',null,7,{}])add({transferResult,material:{[key]:{channel:1,isVideoTexture:true,colorSpace:'invalid'}}});
for(const key of ['map','emissiveMap'])add({sequences:{material:{[key]:[{channel:1},{channel:2},{isVideoTexture:true},{colorSpace:'srgb'}]}}});
for(let failAt=1;failAt<=225;failAt++)add({traceReads:true,failAt,repeat:2,recoveryMaterial:{map:null},material:{map:{channel:3},normalMap:{channel:2}}});
for(const shaderID of ['basic','',null,1])for(const isRawShaderMaterial of [false,true,null,0])add({kind:'key',parameters:{shaderID,isRawShaderMaterial,customVertexShaderID:0,customFragmentShaderID:1,customProgramCacheKey:'key'}});
for(const key of parameterKeys)for(const value of [null,false,true,0,2,'text',{},[]])add({kind:'key',parameters:{shaderID:'basic',isRawShaderMaterial:false,customProgramCacheKey:'key',[key]:value}});
for(const defines of [{A:1,B:false,C:null,D:'x',E:[1,2],F:{}},{'3':3,'1':1,'2':2,a:0},null,'abc',17])add({kind:'key',parameters:{shaderID:'basic',isRawShaderMaterial:true,defines,customProgramCacheKey:'key'}});
add({kind:'key',definesPrototype:{inherited:2,a:3},parameters:{shaderID:'basic',isRawShaderMaterial:true,defines:{a:1,b:2},customProgramCacheKey:'key'}});
for(const replaceDefines of [false,true])add({kind:'key',mutateDefines:true,replaceDefines,definesPrototype:{inherited:4,b:8},parameters:{shaderID:'basic',isRawShaderMaterial:true,defines:{a:1,b:2,c:3},customProgramCacheKey:'key'}});
for(let failAt=1;failAt<=110;failAt++)add({kind:'key',traceReads:true,failAt,parameters:{shaderID:'basic',isRawShaderMaterial:false,customProgramCacheKey:'key'}});
for(const operation of ['acquire','release'])for(const usedTimes of [0,1,2,-1,'1','2',null,false,true,[],[1],{},'invalid'])for(const countAccessor of [false,true])add({kind:'cacheProtocol',operation,usedTimes,countAccessor});
for(const operation of ['acquire','release'])for(const usedTimes of [0,1,'1',null,{},'invalid'])add({kind:'cacheProtocol',operation,usedTimes,countConversion:true,countAccessor:true});
for(const arrayAlias of [false,true])for(const missingProgram of [false,true])add({kind:'cacheProtocol',operation:'release',arrayAlias,missingProgram});
for(const operation of ['acquire','release'])for(let failAt=1;failAt<=10;failAt++)add({kind:'cacheProtocol',operation,countAccessor:true,usedTimes:1,failAt});
for(const isRawShaderMaterial of [false,true])add({kind:'arrayProtocol',parameters:{shaderID:'basic',isRawShaderMaterial,customProgramCacheKey:'key',defines:{A:1,B:2}}});
for(const joinResult of [null,17,{},[]])add({kind:'arrayProtocol',replaceJoin:true,joinResult,parameters:{shaderID:'basic',isRawShaderMaterial:true,customProgramCacheKey:'key'}});
for(let failAt=1;failAt<=25;failAt++)add({kind:'arrayProtocol',failAt,parameters:{shaderID:'basic',isRawShaderMaterial:false,customProgramCacheKey:'key'}});
for(const hasResult of [false,true,null,7,'yes',{},[]])add({kind:'setProtocol',hasResult,material:{map:{channel:1},normalMap:{channel:3}},repeat:2});
add({kind:'setProtocol',ignoreAdd:true,material:{map:{channel:1},normalMap:{channel:3}}});
for(let failAt=1;failAt<=20;failAt++)add({kind:'setProtocol',failAt,material:{map:{channel:1},normalMap:{channel:3}},repeat:2,recoveryMaterial:{map:null,normalMap:null}});
for(const nestedKey of [false,true])add({kind:'layersProtocol',nestedKey,parameters:{shaderID:'basic',isRawShaderMaterial:false,instancing:true,clearcoat:true,flatShading:true,vertexNormals:true,fog:true}});
for(let failAt=1;failAt<=20;failAt++)add({kind:'layersProtocol',failAt,parameters:{shaderID:'basic',isRawShaderMaterial:false,instancing:true,clearcoat:true,flatShading:true,vertexNormals:true,fog:true}});
for(const actions of [
 ['acquireA','acquireA','acquireB','dispose','releaseA','releaseA','releaseB'],
 ['acquireA','acquireB','acquireC','releaseB','releaseA','releaseC','acquireA'],
 ['acquireA','releaseA','releaseA','acquireA','dispose','acquireA'],
 ['releaseA','dispose','acquireA','acquireA','releaseA'],
])add({kind:'lifecycle',actions});
for(let failAt=1;failAt<=28;failAt++)add({kind:'lifecycle',failAt,actions:['acquireA','recover','acquireA','acquireB','releaseA','releaseB']});
for(const method of ['dispose','getUniforms','getProgramCacheKey'])for(const customTarget of [false,true])for(const type of ['MeshBasicMaterial','Custom'])add({kind:'construct',method,customTarget,material:{type},parameters:{shaderID:'basic',isRawShaderMaterial:true,customProgramCacheKey:'key'}});
for(const field of ['precision','type','anisotropy','alphaTest','depthPacking','vertexColors','side'])add({symbolFields:[['material',field]]});
add({symbolChannel:true});
for(const symbolParameter of ['shaderID','customProgramCacheKey','precision','mapUv','defines'])add({kind:'key',symbolParameter,parameters:{shaderID:'basic',isRawShaderMaterial:false,customProgramCacheKey:'key'}});
add({nestedAt:'customKey',material:{map:{channel:1}},repeat:2});
add({kind:'setProtocol',nestedAt:'add',material:{map:{channel:1}},repeat:2});
add({kind:'setProtocol',nestedAt:'customKey',material:{map:{channel:1},normalMap:{channel:3}},repeat:2});
// An optional offset runs only newly appended cases during isolated candidate
// verification. The registered repository test runs the entire fixture set.
const offset=Number(process.argv[4]??0);
assert.ok(Number.isInteger(offset)&&offset>=0&&offset<=cases.length);
const selected=cases.slice(offset);
const inputs=selected.map(c=>JSON.stringify(c));
const expected=inputs.map(c=>scenario(JSON.parse(c)));
if(!probe){console.log(JSON.stringify({oracleOnly:true,scenarios:selected.length,offset,materialFields:materialKeys.length,parameterFields:parameterKeys.length}));process.exit(0);}
const run=spawnSync(probe,[],{input:inputs.join('\n')+'\n',encoding:'utf8',maxBuffer:512*1024*1024});
if(run.status!==0)throw new Error(`native exited ${run.status}: ${run.stderr}\n${run.error??''}`);
const actual=run.stdout.trim().split(/\r?\n/).map(line=>JSON.parse(line));let comparisons=0;
function compare(a,b,at){comparisons++;if(typeof b!=='object'||b===null){assert.deepEqual(a,b,at);return;}assert.equal(Array.isArray(a),Array.isArray(b),at);assert.deepEqual(Object.keys(a),Object.keys(b),at+' keys');for(const key of Object.keys(b))compare(a[key],b[key],at+'.'+key);}
try{assert.equal(actual.length,expected.length);for(let i=0;i<expected.length;i++)compare(actual[i],expected[i],`case ${i}`);}
catch(error){const dir=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-webgl-programs-diff-'));fs.writeFileSync(path.join(dir,'cases.json'),JSON.stringify(cases));fs.writeFileSync(path.join(dir,'expected.json'),JSON.stringify(expected));fs.writeFileSync(path.join(dir,'actual.json'),JSON.stringify(actual));console.error(`Diagnostic files: ${dir}`);throw error;}
console.log(`WebGLPrograms: ${selected.length} scenarios / ${comparisons} comparisons passed (${revision}; offset ${offset}).`);
