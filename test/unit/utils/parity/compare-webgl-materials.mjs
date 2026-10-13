import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import assert from 'node:assert/strict';
import {spawnSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url),'utf8'));
assert.equal(spawnSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).stdout.trim(),lock.commit);
const {WebGLMaterials}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLMaterials.js')));
// Input domains from the pinned source; expected behavior is executed below.
const fields={
  "uniforms": [
    "fogColor",
    "fogNear",
    "fogFar",
    "fogDensity",
    "envMapIntensity",
    "color",
    "opacity",
    "diffuse",
    "emissive",
    "map",
    "mapTransform",
    "alphaMap",
    "alphaMapTransform",
    "bumpMap",
    "bumpMapTransform",
    "bumpScale",
    "normalMap",
    "normalMapTransform",
    "normalScale",
    "displacementMap",
    "displacementMapTransform",
    "displacementScale",
    "displacementBias",
    "emissiveMap",
    "emissiveMapTransform",
    "specularMap",
    "specularMapTransform",
    "alphaTest",
    "envMap",
    "envMapRotation",
    "reflectivity",
    "ior",
    "refractionRatio",
    "lightMap",
    "lightMapIntensity",
    "lightMapTransform",
    "aoMap",
    "aoMapIntensity",
    "aoMapTransform",
    "dashSize",
    "totalSize",
    "scale",
    "size",
    "uvTransform",
    "rotation",
    "specular",
    "shininess",
    "gradientMap",
    "metalness",
    "metalnessMap",
    "metalnessMapTransform",
    "roughness",
    "roughnessMap",
    "roughnessMapTransform",
    "diffuseRoughness",
    "diffuseRoughnessMap",
    "diffuseRoughnessMapTransform",
    "sheenColor",
    "sheenRoughness",
    "sheenColorMap",
    "sheenColorMapTransform",
    "sheenRoughnessMap",
    "sheenRoughnessMapTransform",
    "clearcoat",
    "clearcoatRoughness",
    "clearcoatMap",
    "clearcoatMapTransform",
    "clearcoatRoughnessMap",
    "clearcoatRoughnessMapTransform",
    "clearcoatNormalMap",
    "clearcoatNormalMapTransform",
    "clearcoatNormalScale",
    "dispersion",
    "retroreflectivity",
    "iridescence",
    "iridescenceIOR",
    "iridescenceThicknessMinimum",
    "iridescenceThicknessMaximum",
    "iridescenceMap",
    "iridescenceMapTransform",
    "iridescenceThicknessMap",
    "iridescenceThicknessMapTransform",
    "transmission",
    "transmissionSamplerMap",
    "transmissionSamplerSize",
    "transmissionMap",
    "transmissionMapTransform",
    "thickness",
    "thicknessMap",
    "thicknessMapTransform",
    "attenuationDistance",
    "attenuationColor",
    "anisotropyVector",
    "anisotropyMap",
    "anisotropyMapTransform",
    "specularIntensity",
    "specularColor",
    "specularColorMap",
    "specularColorMapTransform",
    "specularIntensityMap",
    "specularIntensityMapTransform",
    "matcap",
    "referencePosition",
    "nearDistance",
    "farDistance"
  ],
  "material": [
    "isNodeMaterial",
    "uniformsNeedUpdate",
    "isMeshBasicMaterial",
    "isMeshLambertMaterial",
    "envMap",
    "envMapIntensity",
    "isMeshToonMaterial",
    "isMeshPhongMaterial",
    "isMeshStandardMaterial",
    "isMeshPhysicalMaterial",
    "isMeshMatcapMaterial",
    "isMeshDepthMaterial",
    "isMeshDistanceMaterial",
    "isMeshNormalMaterial",
    "isLineBasicMaterial",
    "isLineDashedMaterial",
    "isPointsMaterial",
    "isSpriteMaterial",
    "isShadowMaterial",
    "color",
    "opacity",
    "isShaderMaterial",
    "emissive",
    "emissiveIntensity",
    "map",
    "alphaMap",
    "bumpMap",
    "bumpScale",
    "side",
    "normalMap",
    "normalScale",
    "displacementMap",
    "displacementScale",
    "displacementBias",
    "emissiveMap",
    "specularMap",
    "alphaTest",
    "reflectivity",
    "ior",
    "refractionRatio",
    "lightMap",
    "lightMapIntensity",
    "aoMap",
    "aoMapIntensity",
    "dashSize",
    "gapSize",
    "scale",
    "size",
    "rotation",
    "specular",
    "shininess",
    "gradientMap",
    "metalness",
    "metalnessMap",
    "roughness",
    "roughnessMap",
    "diffuseRoughness",
    "diffuseRoughnessMap",
    "sheen",
    "sheenColor",
    "sheenRoughness",
    "sheenColorMap",
    "sheenRoughnessMap",
    "clearcoat",
    "clearcoatRoughness",
    "clearcoatMap",
    "clearcoatRoughnessMap",
    "clearcoatNormalMap",
    "clearcoatNormalScale",
    "dispersion",
    "retroreflectivity",
    "iridescence",
    "iridescenceIOR",
    "iridescenceThicknessRange",
    "iridescenceMap",
    "iridescenceThicknessMap",
    "transmission",
    "transmissionMap",
    "thickness",
    "thicknessMap",
    "attenuationDistance",
    "attenuationColor",
    "anisotropy",
    "anisotropyRotation",
    "anisotropyMap",
    "specularIntensity",
    "specularColor",
    "specularColorMap",
    "specularIntensityMap",
    "matcap"
  ]
};
function describe(v) {
    if(v===undefined)return 'undefined';if(v===null)return null;
    if(typeof v==='number')return Number.isNaN(v)?'NaN':v===Infinity?'Infinity':v===-Infinity?'-Infinity':Object.is(v,-0)?'-0':v;
    if(typeof v!=='object'&&typeof v!=='function')return v;
    if(Object.hasOwn(v,'label'))return {ref:v.label};
    if(Array.isArray(v))return v.map(describe);
    if('elements' in v)return {matrix:describe(v.elements)};
    return 'object';
}
function tick(state,text) {
    state.log.push(text);const index=++state.count;
    if(index===state.throwAt)throw new TypeError('fixture');
    if(!state.nested&&index===state.reenterAt){state.nested=true;try{state.nestedManager.refreshMaterialUniforms(state.nestedUniforms,state.nestedMaterial,2,64,state.nestedTarget);}finally{state.nested=false;}}
}
function numeric(state,label,result,throws=false){return {label,valueOf(){tick(state,'coerce '+label);if(throws)throw new TypeError('number');return result;}};}
function scalar(state,code,label='override'){return [undefined,null,false,true,0,-0,-2,3,NaN,Infinity,'','3',[],numeric(state,label,2),numeric(state,label,2,true)][code];}
function observed(owner,key,store,state,label){Object.defineProperty(owner,key,{enumerable:true,configurable:true,get(){tick(state,'get '+label);return store[key];},set(value){tick(state,'set '+label+' '+JSON.stringify(describe(value)));store[key]=value;}});}
function sink(state,label){
    if(!state.sinkPrototype){state.sinkPrototype={};for(const name of ['copy','multiplyScalar','negate','set','setFromMatrix4','transpose','premultiply','setFromMatrixPosition']){
        const fn=function(...args){tick(state,'call '+this.label+'.'+name+' '+JSON.stringify(args.map(describe)));if(state.returnMethod===name)return scalar(state,state.returnValue);return this;};
        Object.defineProperty(state.sinkPrototype,name,{enumerable:true,configurable:true,get(){tick(state,'get '+this.label+'.'+name);return state.badMethod===name?17:fn;}});
    }}
    return Object.assign(Object.create(state.sinkPrototype),{label});
}
function texture(state,label,profile){
    const result={label},store={matrixAutoUpdate:profile===3?1:true,matrix:{label:label+'.matrix'},isCubeTexture:true,isRenderTargetTexture:false,updateMatrix(){tick(state,'call '+label+'.updateMatrix');}};
    for(const key of ['matrixAutoUpdate','matrix','isCubeTexture','isRenderTargetTexture','updateMatrix'])observed(result,key,store,state,label+'.'+key);
    return result;
}
function flag(name,kind){const names=['','isNodeMaterial','isMeshBasicMaterial','isMeshLambertMaterial','isMeshToonMaterial','isMeshPhongMaterial','isMeshStandardMaterial','isMeshPhysicalMaterial','isMeshMatcapMaterial','isMeshDepthMaterial','isMeshDistanceMaterial','isMeshNormalMaterial','isLineBasicMaterial','isLineDashedMaterial','isPointsMaterial','isSpriteMaterial','isShadowMaterial','isShaderMaterial'];if(kind===18)return true;if(kind===19)return name==='isMeshBasicMaterial'||name==='isShaderMaterial';return name===names[kind]||(kind===7&&name==='isMeshStandardMaterial')||(kind===13&&name==='isLineBasicMaterial');}
class Fixture{
    constructor(state,spec,prefix=''){
        this.state=state;this.uniforms={};this.uniformStore={};this.material={};this.materialStore={};this.properties={};this.renderer={};this.target={};this.fog={};
        const {kind=0,profile=1}=spec,labeled=key=>({label:prefix+key});
        for(const name of fields.uniforms){const holder={},store={value:sink(state,prefix+name+'Value')};this.uniformStore[name]=store;observed(holder,'value',store,state,prefix+'uniform.'+name+'.value');const wrapper={[name]:name===spec.omitUniform?undefined:holder};observed(this.uniforms,name,wrapper,state,prefix+'uniforms.'+name);}
        for(const name of fields.material){let value;
            if(name.startsWith('is'))value=flag(name,kind);
            else if(name==='uniformsNeedUpdate')value=true;
            else if(['color','emissive','specular','normalScale','sheenColor','clearcoatNormalScale','attenuationColor','specularColor'].includes(name))value=labeled('material.'+name);
            else if(name.endsWith('Map')||name==='map'||name==='matcap')value=profile===0?null:texture(state,prefix+name,profile);
            else if(name==='iridescenceThicknessRange')value=[50,400];
            else if(name==='side')value=profile===3?'1':1;
            else value=profile===0?0:profile===2?numeric(state,prefix+name,2):profile===3?'3':2;
            this.materialStore[name]=value;observed(this.material,name,this.materialStore,state,prefix+'material.'+name);
        }
        if(spec.override)this.materialStore[spec.override]=scalar(state,spec.value??0,prefix+spec.override+'Override');
        const cache={},cacheStore={envMap:profile===0?null:texture(state,prefix+'cachedEnv',profile),envMapRotation:{x:0.25,y:-0.5,z:0.125,order:'XYZ'}};
        const light={},lightStore={matrixWorld:labeled('light.matrixWorld'),shadow:{camera:{near:1,far:99}}};
        for(const key of ['matrixWorld','shadow'])observed(light,key,lightStore,state,prefix+'light.'+key);cacheStore.light=light;
        for(const key of ['envMap','envMapRotation','light'])observed(cache,key,cacheStore,state,prefix+'cache.'+key);
        const propertiesStore={get(){tick(state,'call '+prefix+'properties.get');return cache;}};observed(this.properties,'get',propertiesStore,state,prefix+'properties.get');
        const targetStore={texture:labeled('transmissionTexture'),width:128,height:64};for(const key of ['texture','width','height'])observed(this.target,key,targetStore,state,prefix+'target.'+key);
        const renderTarget=!spec.renderMode?null:{isXRRenderTarget:spec.renderMode===2,texture:{colorSpace:'display-p3'}};
        const rendererStore={outputColorSpace:'srgb',getRenderTarget(){tick(state,'call '+prefix+'renderer.getRenderTarget');return renderTarget;}};
        for(const key of ['getRenderTarget','outputColorSpace'])observed(this.renderer,key,rendererStore,state,prefix+'renderer.'+key);
        const fogColor=labeled('fogColor'),fogColorStore={getRGB(...args){tick(state,'call '+prefix+'fogColor.getRGB '+JSON.stringify(args.map(describe)));return args[0];}};
        observed(fogColor,'getRGB',fogColorStore,state,prefix+'fogColor.getRGB');
        const fogStore={color:fogColor,isFog:spec.fog===1||spec.fog===3,isFogExp2:spec.fog===2||spec.fog===3,near:1,far:80,density:0.01};
        for(const key of ['color','isFog','isFogExp2','near','far','density'])observed(this.fog,key,fogStore,state,prefix+'fog.'+key);
        this.manager=WebGLMaterials(this.renderer,this.properties);
    }
    snapshot(){return Object.fromEntries(fields.uniforms.map(name=>[name,describe(this.uniformStore[name].value)]));}
}
function scenario(spec){
    const state={log:[],count:0,throwAt:spec.throwAt??-1,reenterAt:spec.reenterAt??-1,nested:false,badMethod:spec.badMethod,returnMethod:spec.returnMethod,returnValue:spec.returnValue};const f=new Fixture(state,spec),nested=new Fixture(state,{kind:2,profile:1},'nested.');
    Object.assign(state,{nestedManager:nested.manager,nestedUniforms:nested.uniforms,nestedMaterial:nested.material,nestedTarget:nested.target});
    let status='undefined';try{const args=spec.fog?[f.uniforms,f.fog]:[f.uniforms,f.material,1.5,100,f.target];if(Object.hasOwn(spec,'argIndex'))args[spec.argIndex]=scalar(state,spec.argValue);const result=f.manager[spec.fog?'refreshFogUniforms':'refreshMaterialUniforms'](...args);if(spec.repeat)f.manager.refreshMaterialUniforms(f.uniforms,f.material,0.75,200,f.target);status=describe(result);}catch(e){status={error:e.name};if(e.name!=='TypeError'&&e.name!=='RangeError')status.message=e.message;}
    return {status,log:state.log,uniforms:f.snapshot(),needsUpdate:describe(f.materialStore.uniformsNeedUpdate)};
}
const nativeKinds=['Material','MeshBasicMaterial','MeshLambertMaterial','MeshToonMaterial','MeshPhongMaterial','MeshStandardMaterial','MeshPhysicalMaterial','MeshMatcapMaterial','MeshDepthMaterial','MeshDistanceMaterial','MeshNormalMaterial','LineBasicMaterial','LineDashedMaterial','PointsMaterial','SpriteMaterial','ShadowMaterial','ShaderMaterial','RawShaderMaterial'];
const classes=Object.fromEntries(await Promise.all(nativeKinds.map(async name=>[name,(await import(pathToFileURL(path.join(upstream,'src/materials',name+'.js'))))[name]])));
const getSource=name=>import(pathToFileURL(path.join(upstream,'src',name+'.js')));
const {Texture}=await getSource('textures/Texture'),{CubeTexture}=await getSource('textures/CubeTexture'),{Color}=await getSource('math/Color'),{Vector2}=await getSource('math/Vector2'),{Vector3}=await getSource('math/Vector3'),{Matrix3}=await getSource('math/Matrix3'),{Euler}=await getSource('math/Euler'),{PointLight}=await getSource('lights/PointLight'),{WebGLProperties}=await getSource('renderers/webgl/WebGLProperties');
const isColorName=name=>['diffuse','emissive','specular','sheenColor','attenuationColor','specularColor','color','fogColor'].includes(name);
const isVector2Name=name=>['normalScale','clearcoatNormalScale','transmissionSamplerSize','anisotropyVector'].includes(name);
function nativeDescription(v){if(v===null||typeof v!=='object'||Object.hasOwn(v,'label'))return describe(v);if(v.isColor)return {color:[v.r,v.g,v.b]};if(v.isVector2)return {vector:[v.x,v.y]};if(v.isVector3)return {vector:[v.x,v.y,v.z]};return describe(v);}
function nativeScenario(spec){
    const material=new classes[nativeKinds[spec.kind]](),uniforms={};
    for(const name of fields.uniforms){let value=-99;if(isColorName(name))value=new Color(0.1,0.2,0.3);else if(isVector2Name(name))value=new Vector2(1.5,2.5);else if(name==='referencePosition')value=new Vector3(0.25,0.5,0.75);else if(name.endsWith('Transform')||name==='envMapRotation')value=new Matrix3();uniforms[name]={value};}
    const map=new Texture();map.offset.set(0.125,-0.25);map.repeat.set(2,3);map.center.set(0.5,0.25);map.rotation=0.5;map.label='native-map';
    const environment=new CubeTexture();environment.label='native-env';
    if(spec.full)for(const name of fields.material){if(name.startsWith('is')||name==='uniformsNeedUpdate')continue;let value=2;if(isColorName(name))value=new Color(0.25,0.5,0.75);else if(isVector2Name(name))value=new Vector2(1.5,2.5);else if(name.endsWith('Map')||name==='map'||name==='matcap')value=map;else if(name==='iridescenceThicknessRange')value=[50,400];else if(name==='side')value=1;if(material[name]&&typeof material[name]==='object'&&(isColorName(name)||isVector2Name(name)))material[name].copy(value);else material[name]=value;}
    const properties=WebGLProperties(),cache=properties.get(material);if(spec.full){cache.envMap=environment;cache.envMapRotation=new Euler(0.25,-0.5,0.125);}
    const light=new PointLight();light.matrixWorld.makeTranslation(4,5,6);light.shadow.camera.near=2;light.shadow.camera.far=80;cache.light=light;
    const renderer={getRenderTarget(){return !spec.renderMode?null:{isXRRenderTarget:spec.renderMode===2,texture:{colorSpace:'srgb'}};},outputColorSpace:'srgb'},manager=WebGLMaterials(renderer,properties),target={texture:map,width:128,height:64};
    let status='undefined';try{status=describe(spec.fogNative?manager.refreshFogUniforms(uniforms,{color:new Color(0.25,0.5,0.75),isFog:spec.fogNative===1,isFogExp2:spec.fogNative===2,near:1,far:80,density:0.01}):manager.refreshMaterialUniforms(uniforms,material,1.5,100,target));}catch(e){status={error:e.name};if(e.name!=='TypeError')status.message=e.message;}
    return {status,uniforms:Object.fromEntries(fields.uniforms.map(name=>[name,nativeDescription(uniforms[name].value)])),needsUpdate:describe(material.uniformsNeedUpdate),textureMatrix:describe(map.matrix)};
}

function constructScenario(spec){
    const method=spec.method,mode=spec.mode,state={log:[],count:0,throwAt:mode===10?2:-1,reenterAt:-1,nested:false};
    const f=new Fixture(state,{kind:7,profile:1,fog:1}),fn=method===0?WebGLMaterials:method===1?f.manager.refreshFogUniforms:f.manager.refreshMaterialUniforms;
    const flags=(v,key)=>{const d=Object.getOwnPropertyDescriptor(v,key);return [d!==undefined,d?.writable??false,d?.enumerable??false,d?.configurable??false];};
    const p=fn.prototype,metadata={name:fn.name,length:fn.length,own:Object.getOwnPropertyNames(fn),nameFlags:flags(fn,'name'),lengthFlags:flags(fn,'length'),prototypeFlags:flags(fn,'prototype'),prototypeOwn:Object.getOwnPropertyNames(p),constructorFlags:flags(p,'constructor'),constructorIdentity:p.constructor===fn,functionPrototype:Object.getPrototypeOf(fn)===Function.prototype};
    const targetCalls=[],custom={marker:17};function Alternate(){targetCalls.push('targetBody');}let target=fn;
    if(mode===1||mode===9){Alternate.prototype=custom;target=Alternate;if(mode===9)Object.preventExtensions(custom);}
    if(mode===2||mode===3){Alternate.prototype=mode===2?17:null;target=Alternate;}
    if(mode===4)fn.prototype=custom;if(mode===5)fn.prototype=17;if(mode===6)target=()=>{};if(mode===7)target={};
    const args=method===0?[f.renderer,f.properties]:method===1?[f.uniforms,f.fog]:[f.uniforms,f.material,1.5,100,f.target];
    let result;try{const v=mode===8?fn.apply(null,args):Reflect.construct(fn,args,target),object=v!==null&&(typeof v==='object'||typeof v==='function');result={object,fnPrototype:object&&Object.getPrototypeOf(v)===fn.prototype,customPrototype:object&&Object.getPrototypeOf(v)===custom,objectPrototype:object&&Object.getPrototypeOf(v)===Object.prototype,keys:object?Object.getOwnPropertyNames(v):describe(v)};}catch(e){result={error:e.name};}finally{fn.prototype=p;}
    return {metadata,result,targetCalls,log:state.log,uniforms:f.snapshot()};
}

const cases=[];
for(let kind=0;kind<20;++kind)for(let profile=0;profile<4;++profile)cases.push({kind,profile});
for(let fog=1;fog<5;++fog)for(let renderMode=0;renderMode<3;++renderMode)cases.push({fog,renderMode});
for(const kind of [2,5,7,10,13,14,15,16])cases.push({kind,repeat:true});
for(const kind of [2,5,7,10,13,14,15,16]){
    const length=scenario({kind}).log.length;
    for(let throwAt=1;throwAt<=length+1;++throwAt)cases.push({kind,throwAt});
    for(const reenterAt of [1,Math.floor(length/3),Math.floor(length/2),length])cases.push({kind,reenterAt});
}
for(const override of ['opacity','alphaTest','side','bumpScale','shininess','diffuseRoughness','sheen','clearcoat','dispersion','retroreflectivity','iridescence','transmission','anisotropy','anisotropyRotation','iridescenceThicknessRange','specularIntensity','map','normalMap'])for(let value=0;value<15;++value)cases.push({kind:override==='shininess'?5:7,override,value});
for(const override of ['dashSize','gapSize','scale','size'])for(let value=0;value<15;++value)cases.push({kind:override==='size'?14:13,override,value});
for(const omitUniform of fields.uniforms)cases.push({kind:7,omitUniform});
for(let kind=0;kind<nativeKinds.length;++kind)for(const full of [false,true])cases.push({native:true,kind,full});
for(let method=0;method<3;++method)for(let mode=0;mode<11;++mode)cases.push({construct:true,method,mode});
for(const kind of [0,1,2,7,10,13,14,15,16,17])for(let argIndex=0;argIndex<5;++argIndex)for(let argValue=0;argValue<15;++argValue)cases.push({kind,argIndex,argValue});
for(const fog of [1,2])for(let argIndex=0;argIndex<2;++argIndex)for(let argValue=0;argValue<15;++argValue)cases.push({fog,argIndex,argValue});
for(const kind of [7,10])for(const method of ['copy','multiplyScalar','negate','set','setFromMatrix4','transpose','premultiply','setFromMatrixPosition']){cases.push({kind,badMethod:method});for(const returnValue of [0,1,7])cases.push({kind,returnMethod:method,returnValue});}
for(let fogNative=1;fogNative<=3;++fogNative)for(let renderMode=0;renderMode<3;++renderMode)cases.push({native:true,fogNative,renderMode,kind:0});
for(let fog=1;fog<=3;++fog)for(let renderMode=0;renderMode<3;++renderMode){const length=scenario({fog,renderMode}).log.length;for(let throwAt=1;throwAt<=length+1;++throwAt)cases.push({fog,renderMode,throwAt});for(const reenterAt of [1,Math.floor(length/2),length])cases.push({fog,renderMode,reenterAt});}
if(process.argv[4]==='--native-only')cases.splice(0,cases.length,...cases.filter(s=>s.native));
const expected=cases.map(spec=>spec.construct?constructScenario(spec):spec.native?nativeScenario(spec):scenario(spec));
if(process.argv[3]==='--oracle-only'){console.log(`${cases.length} pinned WebGLMaterials scenarios executed`);process.exit(0);}
const native=spawnSync(process.argv[3],[],{input:JSON.stringify(cases),encoding:'utf8',maxBuffer:128*1024*1024});assert.equal(native.status,0,native.stderr);
const actual=native.stdout.trim().split(/\r?\n/).map(JSON.parse);assert.equal(actual.length,expected.length);
let comparisons=0;
function compare(a,b,key){comparisons++;if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.equal(Array.isArray(a),Array.isArray(b),key);assert.deepEqual(Object.keys(a).sort(),Object.keys(b).sort(),key+' keys');for(const k of Object.keys(b))compare(a[k],b[k],key+'.'+k);}else if(typeof a==='string'&&typeof b==='string'&&/^(call|set) [^ ]+ /.test(a)&&/^(call|set) [^ ]+ /.test(b)){const am=/^((?:call|set) [^ ]+) (.+)$/.exec(a),bm=/^((?:call|set) [^ ]+) (.+)$/.exec(b);assert.equal(am[1],bm[1],key+' operation');compare(JSON.parse(am[2]),JSON.parse(bm[2]),key+' payload');}else assert.deepEqual(a,b,key);}
for(let i=0;i<cases.length;++i){try{compare(actual[i],expected[i],`case[${i}]`);}catch(e){console.error(JSON.stringify(cases[i]));const diagnostics=fs.mkdtempSync(path.join(os.tmpdir(),'threecpp-webgl-materials-parity-'));fs.writeFileSync(path.join(diagnostics,'case.json'),JSON.stringify(cases[i],null,2));fs.writeFileSync(path.join(diagnostics,'actual.json'),JSON.stringify(actual[i],null,2));fs.writeFileSync(path.join(diagnostics,'expected.json'),JSON.stringify(expected[i],null,2));console.error(`Parity diagnostics: ${diagnostics}`);throw e;}}
console.log(`WebGLMaterials parity: ${cases.length} scenarios / ${comparisons} comparisons passed against ${lock.commit}`);
