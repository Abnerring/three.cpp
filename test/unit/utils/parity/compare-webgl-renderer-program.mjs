import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import {pathToFileURL} from 'node:url';
const [upstream,probe,requestedOutput]=process.argv.slice(2);
assert(upstream&&probe,'Usage: compare-webgl-renderer-program.mjs <pinned> <probe|--oracle-only> [fresh-output]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const imports={};for(const file of ['math/Matrix3.js','math/Matrix4.js','math/Vector3.js','math/Vector4.js','math/Box3.js','math/Color.js','math/ColorManagement.js','math/Frustum.js','math/FrustumArray.js','materials/MeshBasicMaterial.js','materials/ShaderMaterial.js','renderers/webgl/WebGLProperties.js','renderers/webgl/WebGLMaterials.js','renderers/webgl/WebGLUniforms.js','renderers/shaders/UniformsUtils.js','renderers/shaders/UniformsLib.js','renderers/shaders/ShaderLib.js','constants.js','utils.js'])Object.assign(imports,await import(pathToFileURL(path.join(upstream,'src',file)).href));
const {Matrix3,Matrix4,Vector3,Box3,Color,MeshBasicMaterial,ShaderMaterial,WebGLProperties,WebGLMaterials,WebGLUniforms,mergeUniforms,UniformsLib,ShaderLib}=imports;
const {parse}=createRequire(path.join(upstream,'package.json'))('acorn'),source=fs.readFileSync(path.join(upstream,'src/renderers/WebGLRenderer.js'),'utf8');
const body=parse(source,{sourceType:'module',ecmaVersion:'latest'}).body.find(n=>n.type==='ClassDeclaration'&&n.id.name==='WebGLRenderer').body.body.find(n=>n.kind==='constructor').value.body.body;
const names=['getProgram','getProgramVariant','getUniformList','updateCommonMaterialProperties','findLightProbeGrid','setProgram','markUniformsLightsNeedsUpdate','materialNeedsLights','onMaterialDispose','deallocateMaterial','releaseMaterialProgramReferences'];
const functions=names.map(name=>body.find(n=>n.type==='FunctionDeclaration'&&n.id.name===name));assert(functions.every(Boolean));
// Only this oracle evaluates JS. Source algorithms are copied without rewriting;
// program compilation and native uniform commands below are traced boundaries.
const factory=new Function('bindings','imports',`"use strict";
const {Vector3,ColorManagement,WebGLUniforms,NoToneMapping}=imports;
const {_this,_gl,properties,programCache,materials,environments,morphtargets,uniformsGroups,capabilities,clipping,textures,state,_emptyScene,getDFGLUT}=bindings;
let currentRenderState=bindings.currentRenderState,_currentRenderTarget=null,_currentMaterialId=-1,_currentCamera=null,_clippingEnabled=false,_localClippingEnabled=false,_nodesHandler=bindings._nodesHandler??null,_pixelRatio=1,_height=48;
const _vector3=new Vector3(),objectPosition=new Vector3();
${functions.map(n=>source.slice(n.start,n.end)).join('\n')}
return {${names.join(',')},get(name){return eval(name);},set(name,value){return eval(name+'=value');}};
`);
const requests=JSON.parse(fs.readFileSync(new URL('./webgl-renderer-program-fixtures.json',import.meta.url)));
const own=(o,k,v)=>Object.defineProperty(o,k,{value:v,writable:true,enumerable:true,configurable:true});
function obj(values={}){return {...values};}
function alias(h,name,value){h.aliases.push([name,value]);return value;}
function tag(h,value){if(value===null||typeof value!=='object'&&typeof value!=='function')return value;for(const [name,other]of h.aliases)if(value===other)return name;if(Array.isArray(value))return value.map(v=>tag(h,v));return '$object';}
function clean(value){if(value===undefined)return {$undefined:true};if(typeof value==='number'&&!Number.isFinite(value))return {$number:Number.isNaN(value)?'NaN':value<0?'-Infinity':'Infinity'};if(Array.isArray(value)||ArrayBuffer.isView(value))return Array.from(value,clean);if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).map(k=>[k,clean(value[k])]));return value;}
const event=(h,value)=>h.events.push(value);
function method(h,owner,key,label,original){owner[key]=function(...args){return dispatch.call(this,h,label,owner,original,args);};}
const instrument=(h,owner,key,label)=>method(h,owner,key,label,owner[key]);
function uniforms(h){const extra={customScalar:{value:.25},envMapIntensity:{value:1},dfgLUT:{value:null}};return mergeUniforms([ShaderLib.basic.uniforms,UniformsLib.lights,extra]);}
const infos=[['diffuse',0x8b51],['opacity',0x1406],['customScalar',0x1406],['projectionMatrix',0x8b5c],['viewMatrix',0x8b5c],['modelViewMatrix',0x8b5c],['normalMatrix',0x8b5b],['modelMatrix',0x8b5c],['isOrthographic',0x8b56],['receiveShadow',0x8b56],['toneMappingExposure',0x1406],['cameraPosition',0x8b51],['logDepthBufFC',0x1406]];
function program(h,key){const p=alias(h,'program'+h.programs.length,obj({id:h.programs.length,program:obj()}));alias(h,'handle'+p.id,p.program);const reflected=new WebGLUniforms(h.gl,p.program);alias(h,'p_uniforms'+p.id,reflected);p.uniforms=reflected;method(h,p,'getUniforms','program.getUniforms');instrument(h,reflected,'setValue','p.setValue');instrument(h,reflected,'setOptional','p.setOptional');h.programs.push(p);h.programKeys.push(key);return p;}
function parameters(h,material,scene,object){const geometry=h.geometry,morph=geometry.morphAttributes,target=h.pipeline.get('_currentRenderTarget'),tone=material.toneMapped&&(!target||target.isXRRenderTarget===true)?h.renderer.toneMapping:0;
    const p={outputColorSpace:!target?h.renderer.outputColorSpace:target.isXRRenderTarget===true?target.texture.colorSpace:imports.ColorManagement.workingColorSpace,toneMapping:tone,batching:!!object.isBatchedMesh,batchingColor:!!object.isBatchedMesh&&object._colorsTexture!==null,instancing:!!object.isInstancedMesh,instancingColor:!!object.isInstancedMesh&&object.instanceColor!==null,instancingMorph:!!object.isInstancedMesh&&object.morphTexture!==null,skinning:!!object.isSkinnedMesh,morphTargets:!!morph.position,morphNormals:!!morph.normal,morphColors:!!morph.color,morphTargetsCount:(morph.position||morph.normal||morph.color)?.length??0,numClippingPlanes:h.clipping.numPlanes,numClipIntersection:h.clipping.numIntersection,vertexAlphas:material.vertexColors===true&&!!geometry.attributes.color&&geometry.attributes.color.itemSize===4,vertexTangents:!!geometry.attributes.tangent&&(!!material.normalMap||material.anisotropy>0)};
    p.key=material.customProgramCacheKey()+'|'+Object.values(p).map(String).join(',');return p;
}
function maybe(h,label){if(h.request.reentry===label&&!h.reentered){h.reentered=true;event(h,['reenter',label]);h.pipeline.setProgram(h.camera2,h.scene,h.geometry,h.material2,h.object2);event(h,['resume',label]);}if(h.request.throwAt===label)throw label;}
function dispatch(h,label,owner,original,args){event(h,[label,this===owner,...args.map(v=>tag(h,v))]);maybe(h,label);
    if(label==='cache.parameters')return parameters(h,args[0],args[3],args[4]);
    if(label==='cache.key')return args[0].key;
    if(label==='cache.uniforms'){const u=args[0].isShaderMaterial?args[0].uniforms:uniforms(h);alias(h,'m_uniforms'+h.uniformSets.length,u);h.uniformSets.push(u);return u;}
    if(label==='cache.acquire'){let p=h.programs[h.programKeys.indexOf(args[1])];if(!p)p=program(h,args[1]);p.usedTimes=(p.usedTimes??0)+1;return p;}
    if(label==='cache.release'){args[0].usedTimes--;return;}
    if(label==='cache.releaseShader')return;
    if(label==='program.getUniforms')return owner.uniforms;
    if(label==='state.useProgram'){const changed=h.bound!==args[0];h.bound=args[0];return changed;}
    if(label==='depth.reversed')return !!h.request.reversed;
    if(label==='env.get')return args[0]??null;
    if(label==='gl.getProgramParameter')return infos.length-(h.request.noCamPosition?1:0);
    if(label==='gl.getActiveUniform'){const [name,type]=infos.filter(v=>!h.request.noCamPosition||v[0]!=='cameraPosition')[args[1]];return {name,type,size:1};}
    if(label==='gl.getUniformLocation')return args[1];
    if(label.startsWith('gl.uniform')){event(h,['uniform-command',args[0],...args.slice(1).map(clean)]);h.uploads[args[0]]=clean(args.at(-1));return;}
    if(label==='renderer.target')return h.pipeline?h.pipeline.get('_currentRenderTarget'):null;
    if(label==='getDFGLUT')return h.dfg;
    if(label==='material.key')return h.key;
    if(label==='material.compile'){
        event(h,['compile-alias',args[1]===h.renderer,args[0].uniforms===h.uniformSets.at(-1)]);
        if(h.request.cleanUniform)args[0].uniforms.customScalar.needsUpdate=false;
        if(h.request.replaceUniforms){args[0].uniforms=uniforms(h);alias(h,'replacement',args[0].uniforms);h.replacement=args[0].uniforms;}return;
    }
    if(label==='skeleton.compute'){owner.boneTexture=h.bone;return;}
    if(label==='nodes.build'&&h.request.nodeUniforms){args[2].uniforms=h.nodeUniforms;return;}
    if(original){const result=original.apply(this,args);if(label==='materials.refresh')event(h,['refreshed',args[0].diffuse?.value.r,args[0].diffuse?.value.g,args[0].opacity?.value]);return result;}
}
function harness(request){const h={request,aliases:[],events:[],programs:[],programKeys:[],uniformSets:[],uploads:{},key:'base',bound:undefined,reentered:false};
    h.renderer=alias(h,'renderer',{outputColorSpace:imports.LinearSRGBColorSpace,toneMapping:0,toneMappingExposure:1});method(h,h.renderer,'getRenderTarget','renderer.target');
    h.gl=alias(h,'gl',{ACTIVE_UNIFORMS:0x8b86,SAMPLER_2D_SHADOW:0x8b62,SAMPLER_CUBE_SHADOW:0x8dc5,SAMPLER_2D_ARRAY_SHADOW:0x8dc4});for(const name of ['getProgramParameter','getActiveUniform','getUniformLocation','uniform1f','uniform1i','uniform2fv','uniform3f','uniform3fv','uniformMatrix3fv','uniformMatrix4fv'])method(h,h.gl,name,'gl.'+name);
    h.properties=new WebGLProperties();alias(h,'properties',h.properties);
    h.material=alias(h,'material',request.shader?new ShaderMaterial():new MeshBasicMaterial());h.material2=alias(h,'material2',new MeshBasicMaterial());h.material.color??=new Color(1,0,0);h.material.color.setRGB(1,0,0);h.material2.color.setRGB(0,0,1);
    if(request.shader)h.material.uniforms=uniforms(h);if(request.lights)h.material.isMeshLambertMaterial=true;if(request.nodes)h.material.isNodeMaterial=true;if(request.sprite)h.material.isSpriteMaterial=true;
    for(const value of [h.material,h.material2]){method(h,value,'customProgramCacheKey','material.key');method(h,value,'onBeforeCompile','material.compile');instrument(h,value,'addEventListener','material.addListener');instrument(h,value,'removeEventListener','material.removeListener');}
    h.props=alias(h,'props',h.properties.get(h.material));h.props2=alias(h,'props2',h.properties.get(h.material2));
    h.camera=alias(h,'camera',{id:101,projectionMatrix:new Matrix4(),matrixWorldInverse:new Matrix4(),matrixWorld:new Matrix4(),isOrthographicCamera:true,reversedDepth:false,far:100});h.camera2=alias(h,'camera2',{...h.camera,id:102,projectionMatrix:new Matrix4(),matrixWorldInverse:new Matrix4(),matrixWorld:new Matrix4()});for(const value of [h.camera,h.camera2])method(h,value,'updateProjectionMatrix','camera.update');
    h.geometry=alias(h,'geometry',{attributes:{},morphAttributes:{}});h.object=alias(h,'object',{matrixWorld:new Matrix4(),modelViewMatrix:new Matrix4(),normalMatrix:new Matrix3(),receiveShadow:false,instanceColor:null,morphTexture:null,_colorsTexture:null,_matricesTexture:obj(),_indirectTexture:obj(),bindMatrix:new Matrix4(),bindMatrixInverse:new Matrix4(),center:obj({x:.5,y:.5})});h.object2=alias(h,'object2',{...h.object,modelViewMatrix:new Matrix4()});
    h.scene=alias(h,'scene',{isScene:true,fog:null,environment:null,environmentRotation:obj(),environmentIntensity:1});h.empty=alias(h,'empty',{isScene:true,fog:null,environment:null});h.bone=alias(h,'bone',obj());h.dfg=alias(h,'dfg',obj());
    h.lights={version:1};for(const name of ['ambient','probe','sun','sunShadow','directional','directionalShadow','spot','spotShadow','rectArea','point','pointShadow','hemi','sunShadowMatrix','sunShadowCascade','directionalShadowMatrix','spotLightMatrix','spotLightMap','pointShadowMatrix','sunShadowMap','directionalShadowMap','spotShadowMap','pointShadowMap'])h.lights[name]=alias(h,'lights.'+name,[]);h.lights.rectAreaLTC1=null;h.lights.rectAreaLTC2=null;
    h.volumes=alias(h,'volumes',[]);h.renderState=alias(h,'renderState',{state:{lights:{state:h.lights},shadowsArray:[],lightProbeGridArray:h.volumes,transmissionRenderTarget:{}}});
    h.clipping=alias(h,'clipping',{numPlanes:0,numIntersection:0,uniform:{value:null}});method(h,h.clipping,'setState','clipping.setState');
    const cache=alias(h,'cache',{});for(const [key,label]of [['getParameters','parameters'],['getProgramCacheKey','key'],['getUniforms','uniforms'],['acquireProgram','acquire'],['releaseProgram','release'],['releaseShaderCache','releaseShader']])method(h,cache,key,'cache.'+label);
    const env=alias(h,'environments',{});method(h,env,'get','env.get');h.materials=new WebGLMaterials(h.renderer,h.properties);alias(h,'materials',h.materials);instrument(h,h.materials,'refreshMaterialUniforms','materials.refresh');instrument(h,h.materials,'refreshFogUniforms','materials.fog');
    const state=alias(h,'state',{buffers:{depth:{}}});method(h,state,'useProgram','state.useProgram');method(h,state.buffers.depth,'getReversed','depth.reversed');const textures=alias(h,'textures',{});method(h,textures,'resetTextureUnits','textures.reset');const morph=alias(h,'morphtargets',{});method(h,morph,'update','morph.update');const groups=alias(h,'uniformsGroups',{});for(const key of ['update','bind'])method(h,groups,key,'groups.'+key);
    const bindings={_this:h.renderer,_gl:h.gl,properties:h.properties,programCache:cache,materials:h.materials,environments:env,morphtargets:morph,uniformsGroups:groups,capabilities:{logarithmicDepthBuffer:!!request.logDepth},clipping:h.clipping,textures,state,currentRenderState:h.renderState,_emptyScene:h.empty,_pixelRatio:1,_height:48};method(h,bindings,'getDFGLUT','getDFGLUT');
    if(request.nodes){const nodes=alias(h,'nodes',{});for(const key of ['build','onUpdateProgram'])method(h,nodes,key,'nodes.'+key);bindings._nodesHandler=nodes;h.nodeUniforms=uniforms(h);}
    h.pipeline=factory(bindings,imports);for(const [key,value]of Object.entries(request.objectFlags??{}))h.object[key]=value;
    if(request.skin){h.object.isSkinnedMesh=true;h.object.skeleton=alias(h,'skeleton',{boneTexture:request.boneReady?h.bone:null});method(h,h.object.skeleton,'computeBoneTexture','skeleton.compute');}
    if(request.morph)h.geometry.morphAttributes[request.morph]=[];
    if(request.groups)h.material.uniformsGroups=alias(h,'uboGroups',[alias(h,'ubo',{})]);
    if(request.shadows)for(const key of ['sunShadowMap','directionalShadowMap','spotShadowMap','pointShadowMap'])h.lights[key].push(obj());
    if(request.volume){for(let i=0;i<request.volume;++i){const box=new Box3(new Vector3(-1,-1,-1),new Vector3(1,1,1));h.volumes.push(alias(h,'volume'+i,{texture:request.nullVolume?null:alias(h,'probeTexture'+i,{}),boundingBox:box,resolution:new Vector3(2,2,2)}));}}
    if(request.cleanUniform){}if(request.fog)h.scene.fog={isFog:true,color:new Color(.1,.2,.3),near:1,far:20};
    if(request.getter){const value=h.material.opacity;Object.defineProperty(h.material,'opacity',{configurable:true,get(){event(h,'get:opacity');maybe(h,'get:opacity');return value;}});}
    return h;
}
function step(h,s){const target=h[s.target??'material'];switch(s.op??'draw'){
    case 'draw':return h.pipeline.setProgram(h[s.camera??'camera'],h[s.scene??'scene'],h.geometry,h[s.material??'material'],h[s.object??'object']);
    case 'set':target[s.key]=s.value;return;
    case 'props':h.props[s.key]=s.value;return;
    case 'color':target.color.setRGB(...s.value);return;
    case 'key':h.key=s.value;return;
    case 'cell':h.pipeline.set(s.key,s.target?h[s.target]:s.value);return;
    case 'uniform':h.props.uniforms[s.key][s.field??'value']=s.value;return;
    case 'sceneFog':h.scene.fog={isFog:true,color:new Color(.3,.2,.1),near:2,far:30};return;
    case 'geometryColor':h.geometry.attributes.color={itemSize:4};h.material.vertexColors=true;return;
    case 'geometryTangent':h.geometry.attributes.tangent={};h.material.anisotropy=1;return;
    case 'morph':h.geometry.morphAttributes[s.key]=new Array(s.count??1).fill({});return;
    case 'lightsVersion':h.lights.version++;return;
    case 'target':h.pipeline.set('_currentRenderTarget',s.xr?{isXRRenderTarget:true,texture:{colorSpace:s.space??imports.SRGBColorSpace}}:{});return;
    case 'env':h.material.envMap=alias(h,'envmap',{});return;
    case 'dispose':target.dispose();return;
    case 'list':{const a=h.pipeline.getUniformList(h.props),b=h.pipeline.getUniformList(h.props);event(h,['list-identity',a===b,a===h.props.uniformsList,a.map(v=>v.id)]);return;}
    case 'addUniform':h.props.uniforms[s.key]={value:s.value};return;
    case 'translate':h.object.matrixWorld.makeTranslation(s.value,0,0);return;
    case 'volumeCount':{h.volumes.length=s.value;if(s.value){h.volumes[0]=alias(h,'addedVolume',{texture:h.dfg,boundingBox:new Box3(new Vector3(-1,-1,-1),new Vector3(1,1,1)),resolution:new Vector3(2,2,2)});}return;}
    case 'needUpdate':target.needsUpdate=s.value;return;
    case 'directGet':return h.pipeline.getProgram(h.material,h.scene,h.object);
    default:throw new Error('bad fixture op '+s.op);
}}
const commonNames=['outputColorSpace','batching','batchingColor','instancing','instancingColor','instancingMorph','skinning','morphTargets','morphNormals','morphColors','morphTargetsCount','numClippingPlanes','numIntersection','vertexAlphas','vertexTangents','toneMapping','needsLights'];
function stateOf(h){const p=h.props;return {program:tag(h,p.currentProgram),programs:p.programs?Array.from(p.programs.values(),v=>tag(h,v)):[],variants:p.programVariants?Array.from(p.programVariants.values(),v=>tag(h,v)):[],version:p.__version,currentMaterial:h.pipeline.get('_currentMaterialId')===h.material.id,currentCamera:tag(h,h.pipeline.get('_currentCamera')),uniformAlias:p.uniforms===h.uniformSets.at(-1),replacementAlias:!!h.replacement&&p.uniforms===h.replacement,shaderAlias:!!h.material.isShaderMaterial&&p.uniforms===h.material.uniforms,clippingAlias:!!p.uniforms&&p.uniforms.clippingPlanes===h.clipping.uniform,ambientAlias:!!p.uniforms&&p.uniforms.ambientLightColor.value===h.lights.ambient,diffuse:p.uniforms?[p.uniforms.diffuse.value.r,p.uniforms.diffuse.value.g,p.uniforms.diffuse.value.b]:undefined,opacity:p.uniforms?.opacity.value,uniformsNeedUpdate:h.material.uniformsNeedUpdate,programUse:h.programs.map(p=>p.usedTimes),lightsVersion:p.lightsStateVersion,receiveShadow:p.receiveShadow,probe:tag(h,p.lightProbeGrid),uploads:h.uploads,common:Object.fromEntries(commonNames.map(k=>[k,p[k]]))};}
function run(request){const h=harness(request),out={};if(request.op==='metadata')return names.map(key=>{const f=h.pipeline[key];let constructible=false;try{Reflect.construct(function(){},[],f);constructible=true;}catch{}return {key,name:f.name,length:f.length,source:f.toString(),constructible};});const steps=request.steps??[{},{}];out.results=[];for(const s of steps){try{out.results.push({value:tag(h,step(h,s))});}catch(error){const e=error!==null&&['object','function'].includes(typeof error)?{name:error.name}:{name:'ThrownValue',raw:error};out.results.push({error:e});if(!request.continueOnError)break;}}out.events=h.events;out.state=stateOf(h);return clean(out);}
const expected=requests.map(run);for(let i=0;i<requests.length;++i){const failures=expected[i].results?.filter(v=>v.error)??[];assert.equal(failures.length,requests[i].errors??0,'unexpected source exception '+requests[i].case+' '+JSON.stringify(failures));}
const output=requestedOutput??path.join('D:/Temp/surui','webgl-renderer-program-'+Date.now());assert(!fs.existsSync(output),'Output must be fresh');fs.mkdirSync(output,{recursive:true});for(const [name,value]of [['requests',requests],['expected',expected]])fs.writeFileSync(path.join(output,name+'.json'),JSON.stringify(value,null,2));
const failures=[];if(probe!=='--oracle-only'){const actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:96*1024*1024,timeout:300000,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);const compare=(a,b,p='')=>{if(typeof a==='number'&&typeof b==='number'&&Number.isFinite(a)&&Number.isFinite(b)){assert(Math.abs(a-b)<=1e-12*Math.max(1,Math.abs(a),Math.abs(b)),p);return;}if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),p);for(const k of Object.keys(a))compare(a[k],b[k],p+'/'+k);return;}assert.deepEqual(a,b,p);};requests.forEach((request,i)=>{try{compare(actual[i],expected[i]);}catch(e){failures.push({case:request.case,message:e.message});}});}
const summary={cases:requests.length,sourceErrors:expected.flatMap(x=>x.results??[]).filter(x=>x.error).length,failures};fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));console.log(JSON.stringify({output,...summary,failures:failures.length}));assert.equal(failures.length,0,JSON.stringify(failures.slice(0,3)));
