// Pinned-source CPU oracle. No GPU or JavaScript execution is used by production.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
const upstream=path.resolve(process.argv[2]??''),probe=process.argv[3];
if(!process.argv[2]||!probe)throw Error('Usage: compare-webgl-shadow-map.mjs <pinned> <probe|--oracle-only> [output-directory]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--untracked-files=no','--','src'],{encoding:'utf8'}).trim(),'','Pinned source must be clean');
const load=async(file,name)=>(await import(pathToFileURL(path.join(upstream,'src',file+'.js'))))[name];
const WebGLShadowMap=await load('renderers/webgl/WebGLShadowMap','WebGLShadowMap');
const DirectionalLight=await load('lights/DirectionalLight','DirectionalLight'),PointLight=await load('lights/PointLight','PointLight');
const Scene=await load('scenes/Scene','Scene'),OrthographicCamera=await load('cameras/OrthographicCamera','OrthographicCamera');
const Mesh=await load('objects/Mesh','Mesh'),BufferGeometry=await load('core/BufferGeometry','BufferGeometry');
const MeshBasicMaterial=await load('materials/MeshBasicMaterial','MeshBasicMaterial'),MeshDepthMaterial=await load('materials/MeshDepthMaterial','MeshDepthMaterial');
const Texture=await load('textures/Texture','Texture'),Vector2=await load('math/Vector2','Vector2'),Vector4=await load('math/Vector4','Vector4');
const WebGLProperties=await load('renderers/webgl/WebGLProperties','WebGLProperties');
const C=await import(pathToFileURL(path.join(upstream,'src/constants.js')));
const cases=[
 {name:'disabled',enabled:false},{name:'disabledNeedsUpdate',enabled:false,needsUpdate:true},
 {name:'manualIdle',autoUpdate:false},{name:'manualRequested',autoUpdate:false,needsUpdate:true},
 {name:'empty',empty:true},{name:'lightManualIdle',lightAutoUpdate:false},{name:'lightManualRequested',lightAutoUpdate:false,lightNeedsUpdate:true},
 {name:'pcf'},{name:'basic',type:C.BasicShadowMap},{name:'softFallback',type:C.PCFSoftShadowMap},{name:'reversed',reversed:true},
 {name:'clampedAtlas',atlas:true,clamp:true},{name:'atlas',atlas:true},{name:'shadowLayers',shadowLayers:2},
 {name:'noCaster',castShadow:false},{name:'hidden',hidden:true},{name:'nested',nested:true},{name:'groups',groups:true},
 {name:'customDepth',custom:true},{name:'shadowSide',shadowSide:C.DoubleSide},{name:'alphaCoverage',variant:'coverage'},
 {name:'alphaMapCache',variant:'alpha',repeat:true},{name:'displacementCache',variant:'displacement',repeat:true},
 {name:'clippingCache',variant:'clipping',repeat:true},{name:'cacheDispose',variant:'alpha',dispose:true},
 {name:'clearThrow',throwAt:'clear'},{name:'drawThrow',throwAt:'draw'},{name:'beforeThrow',throwAt:'before'},
 {name:'targetThrow',throwAt:'setRenderTarget'},{name:'getTargetThrow',throwAt:'getRenderTarget'},
 {name:'beforeReentry',reenter:true},{name:'resize',resize:true},{name:'typeChange',typeChange:C.BasicShadowMap},
 {name:'typeChangeVsm',typeChange:C.VSMShadowMap},{name:'vsm',type:C.VSMShadowMap},
 {name:'vsmReceiver',type:C.VSMShadowMap,castShadow:false,receiveShadow:true},
 {name:'vsmSamplesResize',type:C.VSMShadowMap,vsmRepeat:true},
 {name:'point',point:true},{name:'pointReversed',point:true,reversed:true},{name:'pointBasic',point:true,type:C.BasicShadowMap},
 {name:'pointDistance',point:true,variant:'alpha',repeat:true},{name:'pointVsm',point:true,type:C.VSMShadowMap},
 {name:'missingShadow',missingShadow:true}
];
const clean=v=>v===undefined?'undefined':v;
function run(request){
 const h={events:[],rows:[],targets:[],materials:[],current:{tag:'outer'},face:3,mip:2,once:false,properties:new WebGLProperties()};
 const record=(...a)=>h.events.push(a),targetKey=t=>t===null?null:t.tag??('target'+enroll(h.targets,t));
 function enroll(values,value){let i=values.indexOf(value);if(i===-1){i=values.length;values.push(value);}return i;}
 const materialKey=m=>{const fresh=!h.materials.includes(m),key=enroll(h.materials,m);if(fresh)m.addEventListener('dispose',()=>record('material.dispose',key));return key;};
 const materialState=m=>({key:materialKey(m),type:m.type,side:clean(m.side),alphaTest:clean(m.alphaTest),visible:clean(m.visible),wireframe:clean(m.wireframe),clipShadows:clean(m.clipShadows),clipIntersection:clean(m.clipIntersection),clippingPlanes:m.clippingPlanes===null?null:m.clippingPlanes.length,alphaMap:!!m.alphaMap,map:!!m.map,displacementMap:!!m.displacementMap,displacementScale:clean(m.displacementScale),displacementBias:clean(m.displacementBias),wireframeLinewidth:clean(m.wireframeLinewidth),linewidth:clean(m.linewidth),version:m.version,light:h.properties.get(m).light===h.light,defines:m.isShaderMaterial?m.defines:null,uniforms:m.isShaderMaterial?{resolution:m.uniforms.resolution.value.toArray(),radius:m.uniforms.radius.value,pass:m.uniforms.shadow_pass.value?.isDepthTexture?'depth':'color'}:null});
 const service=name=>(...args)=>{record(name,...args);if(request.throwAt===name)throw name;};
 const depth={getReversed(){record('depth.getReversed');return request.reversed??false;},setTest:service('depth.setTest')};
 const renderer={localClippingEnabled:request.variant==='clipping',properties:h.properties,state:{buffers:{depth,color:{setClear:service('color.setClear')}},setBlending:service('setBlending'),setScissorTest:service('setScissorTest'),viewport(v){record('viewport',v.toArray());}},
  getRenderTarget(){record('getRenderTarget');if(request.throwAt==='getRenderTarget')throw 'getRenderTarget';return h.current;},getActiveCubeFace(){record('getActiveCubeFace');return h.face;},getActiveMipmapLevel(){record('getActiveMipmapLevel');return h.mip;},
  setRenderTarget(t,face=0,mip=0){record('setRenderTarget',targetKey(t),arguments.length,face,mip);if(request.throwAt==='setRenderTarget')throw 'setRenderTarget';h.current=t;h.face=face;h.mip=mip;},clear(){record('clear');if(request.throwAt==='clear')throw 'clear';},
  renderBufferDirect(camera,fog,geometry,material,object,group){record('draw',object.name,materialState(material),group===null?null:group.materialIndex);if(request.throwAt==='draw')throw 'draw';}
 };
 h.renderer=renderer;h.camera=new OrthographicCamera();h.scene=new Scene();h.light=request.point?new PointLight():new DirectionalLight();h.light.name='light';
 const shadow=h.light.shadow;shadow.mapSize.set(request.clamp?65:32,request.clamp?49:16);shadow.autoUpdate=request.lightAutoUpdate??true;shadow.needsUpdate=request.lightNeedsUpdate??false;
 if(request.point)h.light.distance=23;
 if(request.shadowLayers)shadow.camera.layers.mask=request.shadowLayers;
 const originalUpdate=shadow.updateMatrices;shadow.updateMatrices=function(...args){record('shadow.updateMatrices',args.length);return originalUpdate.apply(this,args);};
 if(request.atlas){shadow._frameExtents=new Vector2(2,request.clamp?3:1);shadow._viewportCount=2;shadow._viewports=[new Vector4(0,0,1,1),new Vector4(1,0,1,1)];}
 h.source=new MeshBasicMaterial();h.source.name='source';h.source.wireframe=true;h.source.wireframeLinewidth=3;h.source.side=C.FrontSide;
 if(request.shadowSide!==undefined)h.source.shadowSide=request.shadowSide;
 if(request.variant==='alpha'){h.source.alphaMap=new Texture();h.source.map=new Texture();h.source.alphaTest=.25;}
 if(request.variant==='coverage')h.source.alphaToCoverage=true;
 if(request.variant==='displacement'){h.source.displacementMap=new Texture();h.source.displacementScale=2;h.source.displacementBias=.5;}
 if(request.variant==='clipping'){h.source.clipShadows=true;h.source.clippingPlanes=[{}];h.source.clipIntersection=true;}
 const geometry=new BufferGeometry();if(request.groups){geometry.addGroup(0,3,0);geometry.addGroup(3,3,1);geometry.addGroup(6,3,2);}
 const second=new MeshBasicMaterial();second.visible=false;h.mesh=new Mesh(geometry,request.groups?[h.source,second,h.source]:h.source);h.mesh.name='caster';h.mesh.castShadow=request.castShadow??true;h.mesh.receiveShadow=request.receiveShadow??false;h.mesh.frustumCulled=false;
 if(request.custom)h.mesh.customDepthMaterial=new MeshDepthMaterial();
 const hook=before=>function(renderer,object,camera,shadowCamera,geometry,material,group){record(before?'before':'after',this===h.mesh,renderer===h.renderer,object===this,camera===h.camera,shadowCamera===shadow.camera,materialKey(material),group===null?null:group.materialIndex);if(before&&request.throwAt==='before')throw 'before';if(before&&request.reenter&&!h.once){h.once=true;record('reenter');h.manager.render([h.light],h.scene,h.camera);}};
 h.mesh.onBeforeShadow=hook(true);h.mesh.onAfterShadow=hook(false);
 if(request.nested){const parent=new Mesh(new BufferGeometry(),new MeshBasicMaterial());parent.name='parent';parent.castShadow=false;parent.frustumCulled=false;parent.add(h.mesh);h.scene.add(parent);}else h.scene.add(h.mesh);
 if(request.hidden)h.mesh.visible=false;
 h.manager=new WebGLShadowMap(renderer,{update(object){record('objects.update',object.name);return object.geometry;}},{maxTextureSize:request.clamp?32:4096});
 const defaults={enabled:h.manager.enabled,autoUpdate:h.manager.autoUpdate,needsUpdate:h.manager.needsUpdate,type:h.manager.type,keys:Object.keys(h.manager),renderLength:h.manager.render.length};
 h.manager.enabled=request.enabled??true;h.manager.autoUpdate=request.autoUpdate??true;h.manager.needsUpdate=request.needsUpdate??false;if(request.type!==undefined)h.manager.type=request.type;
 if(request.missingShadow)h.light={name:'unshadowed'};
 function mapState(map){if(map===null)return null;const d=map.depthTexture;return {key:targetKey(map),size:[map.width,map.height],cube:map.isWebGLCubeRenderTarget===true,texture:[map.texture.format,map.texture.type,map.texture.minFilter,map.texture.magFilter,map.texture.name,map.texture.generateMipmaps],depth:d===null?null:{cube:d.isCubeDepthTexture===true,type:d.type,format:d.format,compare:d.compareFunction,min:d.minFilter,mag:d.magFilter,name:d.name,owner:d.renderTarget===map}};}
 function snapshot(){return {manager:[h.manager.enabled,h.manager.autoUpdate,h.manager.needsUpdate,h.manager.type],target:targetKey(h.current),face:h.face,mip:h.mip,map:mapState(shadow.map),pass:mapState(shadow.mapPass),mapSize:shadow.mapSize.toArray(),needsUpdate:shadow.needsUpdate,reversed:shadow.camera._reversedDepth??null,sourceVersion:h.source.version,disposeListeners:h.source._listeners?.dispose?.length??0};}
 function execute(){let error=null;try{h.manager.render(request.empty?[]:[h.light],h.scene,h.camera);}catch(e){error=typeof e==='object'?{name:e.name}:{raw:e};}h.rows.push({error,state:snapshot()});}
 const savedWarn=console.warn;console.warn=(...args)=>record('warn',args.map(v=>typeof v==='object'?'object':v));
 try{
  execute();
  if(request.repeat)execute();
  if(request.dispose){h.source.dispose();execute();}
  if(request.resize){shadow.mapSize.set(16,8);execute();}
  if(request.vsmRepeat){shadow.blurSamples=5;shadow.radius=2;shadow.mapSize.set(16,8);execute();}
  if(request.typeChange!==undefined){const old=shadow.map;if(old){if(old.depthTexture)old.depthTexture.addEventListener('dispose',()=>record('depth.dispose',targetKey(old)));old.addEventListener('dispose',()=>record('target.dispose',targetKey(old),old.depthTexture===null));}h.manager.type=request.typeChange;h.manager.needsUpdate=true;execute();}
 }finally{console.warn=savedWarn;}
 return {defaults,rows:h.rows,events:h.events};
}
const expected=cases.map(run);
for(let i=0;i<cases.length;++i){const errors=expected[i].rows.filter(row=>row.error).map(row=>row.error);assert.deepEqual(errors,cases[i].throwAt?[{raw:cases[i].throwAt}]:[],'Unexpected pinned-source error in '+cases[i].name);}
const output=process.argv[4]??path.join('D:/Temp/surui','webgl-shadow-map-'+Date.now());assert(!fs.existsSync(output),'Fresh output required');fs.mkdirSync(output,{recursive:true});
fs.writeFileSync(path.join(output,'requests.json'),JSON.stringify(cases,null,2));fs.writeFileSync(path.join(output,'expected.json'),JSON.stringify(expected,null,2));
const failures=[];
if(probe!=='--oracle-only'){
 const actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(cases),encoding:'utf8',timeout:120000,maxBuffer:128*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}));fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);
 for(let i=0;i<cases.length;++i)try{assert.deepEqual(actual[i],JSON.parse(JSON.stringify(expected[i])));}catch(e){failures.push({case:cases[i].name,message:e.message});}
}
const summary={cases:cases.length,boundary:'Pinned CPU state/command traces; no GPU execution claimed',failures};fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));console.log(JSON.stringify({output,...summary,failures:failures.length}));assert.equal(failures.length,0,JSON.stringify(failures.slice(0,3)));
