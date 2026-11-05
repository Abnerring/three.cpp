import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {createRequire} from 'node:module';
import {pathToFileURL} from 'node:url';
const [upstream,probe,requestedOutput]=process.argv.slice(2);
assert(upstream&&probe,'Usage: compare-webgl-renderer-scene.mjs <pinned> <probe|--oracle-only> [fresh-output]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=p=>import(pathToFileURL(path.join(upstream,'src',p)).href),imports={};
for(const p of ['math/Matrix3.js','math/Matrix4.js','math/Vector3.js','math/Vector4.js','math/Color.js','math/ColorManagement.js','math/Sphere.js','math/Frustum.js','math/FrustumArray.js','core/Layers.js','renderers/WebGLRenderTarget.js','renderers/webgl/WebGLRenderLists.js','renderers/webgl/WebGLRenderStates.js','constants.js','utils.js'])Object.assign(imports,await load(p));
const {Matrix3,Matrix4,Vector3,Vector4,Color,Sphere,Layers,WebGLRenderTarget,WebGLRenderLists,WebGLRenderStates}=imports;
const {parse}=createRequire(path.join(upstream,'package.json'))('acorn'),source=fs.readFileSync(path.join(upstream,'src/renderers/WebGLRenderer.js'),'utf8'),ast=parse(source,{sourceType:'module',ecmaVersion:'latest'});
const constructor=ast.body.find(n=>n.type==='ClassDeclaration'&&n.id.name==='WebGLRenderer').body.body.find(n=>n.kind==='constructor').value.body.body;
const render=constructor.find(n=>n.type==='ExpressionStatement'&&n.expression.type==='AssignmentExpression'&&n.expression.left.type==='MemberExpression'&&n.expression.left.object.type==='ThisExpression'&&n.expression.left.property.name==='render').expression.right;
const privateNames=['projectObject','renderScene','renderTransmissionPass','renderObjects','renderObject'],functions=privateNames.map(name=>constructor.find(n=>n.type==='FunctionDeclaration'&&n.id.name===name));
// Only the oracle evaluates JavaScript. These are the unmodified pinned source
// functions in one closure; backend doubles below trace protocol boundaries.
const factory=new Function('bindings','imports',`"use strict";
const {Matrix4,Vector4,Color,ColorManagement,Frustum,FrustumArray,WebGLRenderTarget,WebGLCoordinateSystem,DoubleSide,BackSide,FrontSide,HalfFloatType,UnsignedByteType,LinearMipmapLinearFilter,NoToneMapping,error}=imports;
const _this=bindings._this;let {objects,renderLists,renderStates,clipping,background,shadowMap,textures,state,xr,bindingStates,extensions,capabilities}=bindings;
let currentRenderList=null,currentRenderState=null,output=bindings.output??null,_isContextLost=bindings._isContextLost??false,_nodesHandler=bindings._nodesHandler??null;
const renderListStack=[],renderStateStack=[],stencil=false;
let _currentActiveCubeFace=0,_currentActiveMipmapLevel=0,_currentRenderTarget=null,_currentMaterialId=-1,_currentCamera=null,_currentClearAlpha=0,_opaqueSort=null,_transparentSort=null,_clippingEnabled=false,_localClippingEnabled=false,_renderBackground=false;
const _currentViewport=new Vector4(),_currentClearColor=new Color(0),_frustum=new Frustum(),_frustumArray=new FrustumArray(),_projScreenMatrix=new Matrix4(),_vector4=new Vector4();
${functions.map(n=>source.slice(n.start,n.end)).join('\n')}
const renderHolder={};renderHolder.render=${source.slice(render.start,render.end)};const render=renderHolder.render;
return {render,get(name){return eval(name);},set(name,value){return eval(name+'=value');},${privateNames.join(',')}};
`);
const requests=JSON.parse(fs.readFileSync(new URL('./webgl-renderer-scene-fixtures.json',import.meta.url)));
function clean(v){if(v===undefined)return {$undefined:true};if(typeof v==='number'&&!Number.isFinite(v))return {$number:Number.isNaN(v)?'NaN':v<0?'-Infinity':'Infinity'};if(Array.isArray(v))return v.map(clean);if(v&&typeof v==='object')return Object.fromEntries(Object.keys(v).map(k=>[k,clean(v[k])]));return v;}
function tag(v){if(v===null||typeof v!=='object')return v;if(Object.hasOwn(v,'tag'))return v.tag;if(Array.isArray(v))return v.map(tag);return '$object';}
const event=(h,v)=>h.events.push(v);
function method(h,owner,key,label,original){owner[key]=function(...args){return dispatch.call(this,h,label,owner,original,args);};}
const instrument=(h,value,key,label)=>method(h,value,key,label,value[key]);
function camera(h,name,id){const out={tag:name,id,isCamera:true,parent:null,matrixWorldAutoUpdate:true,coordinateSystem:2000,reversedDepth:false,projectionMatrix:new Matrix4(),matrixWorldInverse:new Matrix4(),layers:new Layers()};method(h,out,'updateMatrixWorld','camera.update:'+name);return out;}
function material(h,name,id){const out={tag:name,id,visible:true,transparent:false,transmission:0,side:0,forceSinglePass:false,allowOverride:true,version:0};method(h,out,'onBeforeRender','material.before:'+name);Object.defineProperty(out,'needsUpdate',{configurable:true,set(v){event(h,['material.needsUpdate',this.tag,v]);if(v===true)this.version++;}});return out;}
function node(h,name,id,kind='mesh'){const out={tag:name,id,visible:true,layers:new Layers(),children:[],renderOrder:0,frustumCulled:false,matrixWorld:new Matrix4(),modelViewMatrix:new Matrix4(),normalMatrix:new Matrix3()};if(kind==='mesh')out.isMesh=true;if(kind==='sprite')out.isSprite=true;if(kind==='group')out.isGroup=true;if(kind==='lod'){out.isLOD=true;out.autoUpdate=true;method(h,out,'update','lod.update:'+name);}method(h,out,'onBeforeRender','object.before:'+name);method(h,out,'onAfterRender','object.after:'+name);method(h,out,'intersectsFrustum','intersects:'+name);return out;}
function scene(h,name,id){const out=node(h,name,id,'scene');out.isScene=true;out.overrideMaterial=null;out.matrixWorldAutoUpdate=true;method(h,out,'updateMatrixWorld','scene.update:'+name);method(h,out,'onBeforeRender','scene.before:'+name);method(h,out,'onAfterRender','scene.after:'+name);return out;}
function geometry(h,name){const out={tag:name,groups:[],boundingSphere:null};method(h,out,'computeBoundingSphere','geometry.sphere:'+name);return out;}
function maybeReenter(h,label){if(h.request.reentry!==label||h.reentered)return;h.reentered=true;event(h,['reenter',label]);h.renderer.render(h.inner,h.camera2);event(h,['resumed',label]);}
function dispatch(h,label,owner,original,args){
    const request=h.request;event(h,[label,this===owner,...args.map(tag)]);
    if(label.startsWith('scene.before:')||label.startsWith('scene.after:')||label.startsWith('object.')||label.startsWith('material.before:')||label==='draw')event(h,['arg-identities',label,args.map(a=>a&&typeof a==='object'&&Object.hasOwn(a,'tag')?(h.aliases[a.tag]===undefined?null:a===h.aliases[a.tag]):null)]);
    maybeReenter(h,label);if(request.throwAt===label)throw label;
    if(label==='objects.update'){const object=args[0];h.textureUnits++;if(request.shrinkChildren&&object===h.mesh)h.scene.children.length=1;return object.geometry;}
    if(label.startsWith('geometry.sphere:')){owner.boundingSphere=new Sphere(new Vector3(),.5);return;}
    if(label.startsWith('intersects:'))return request.rejectFrustum?false:args[0].intersectsSphere(new Sphere(new Vector3(),.5));
    if(label==='clipping.init')return !!request.clipping;
    if(label==='textures.getTextureUnits')return h.textureUnits;
    if(label==='textures.setTextureUnits'){h.textureUnits=args[0];return;}
    if(label==='xr.getCamera')return h.camera2;
    if(label==='xr.getDepthSensingMesh')return request.depthMesh?h.depthMesh:null;
    if(label==='xr.hasDepthSensing')return !!request.depthSensing;
    if(label==='output.begin')return Object.hasOwn(request,'outputValue')?request.outputValue:true;
    if(label==='output.hasRenderPass')return !!request.skipScene;
    if(label==='output.isCompositing')return !!request.compositing;
    if(label==='extensions.has')return args[0]==='WEBGL_multisampled_render_to_texture'&&!!request.msrt;
    if(label==='renderer.getRenderTarget')return h.target;
    if(label==='renderer.getActiveCubeFace'||label==='renderer.getActiveMipmapLevel')return 0;
    if(label==='renderer.setRenderTarget'){const target=args[0];if(target&&!Object.hasOwn(target,'tag'))target.tag='transmission-target';h.target=target;h.pipeline.set('_currentRenderTarget',target);return;}
    if(label==='renderer.getClearColor'){args[0].copy(h.clearColor);return args[0];}
    if(label==='renderer.getClearAlpha')return h.clearAlpha;
    if(label==='renderer.setClearColor'){h.clearColor.set(args[0]);if(args.length>1)h.clearAlpha=args[1];return;}
    if(label==='draw'){event(h,['draw-state',args[3].side,args[3].version,args[0].viewport===undefined]);if(request.drawThrow&&(!request.throwMesh||args[4]===h.mesh))throw 'GPU:unsupported';return;}
    if(original){const value=original.apply(this,args);if(label==='renderStates.get'&&!value.sceneHarnessInstrumented){Object.defineProperty(value,'sceneHarnessInstrumented',{value:true});for(const key of ['init','pushLight','pushShadow','setupLights','setupLightsView'])instrument(h,value,key,'renderState.'+key);}return value;}
}
function harness(request){
    const h={request,events:[],textureUnits:0,reentered:false,target:null,clearAlpha:.25,clearColor:new Color(0x123456)},renderer={tag:'renderer',sortObjects:Object.hasOwn(request,'sort')?request.sort:true,clippingPlanes:[],localClippingEnabled:!!request.clipping,transmissionResolutionScale:.5,toneMapping:3},bindings={_this:renderer};
    const info={render:{frame:0},autoReset:true};method(h,info,'reset','info.reset');renderer.info=info;h.renderer=renderer;
    const cam=camera(h,'camera',101),cam2=camera(h,'camera2',102);h.camera=cam;h.camera2=cam2;cam2.viewport=new Vector4(2,3,32,24);
    const root=scene(h,'scene',1),inner=scene(h,'inner',2),mat=material(h,'material',7),mat2=material(h,'material2',8),over=material(h,'override',9);h.scene=root;h.inner=inner;h.material=mat;
    for(const key of ['transparent','side','allowOverride'])if(Object.hasOwn(request,key))mat[key]=request[key];if(request.transmission){mat.transmission=1;mat.side=2;}if(request.override)root.overrideMaterial=over;
    const mesh=node(h,'mesh',11),mesh2=node(h,'mesh2',12),geom=geometry(h,'geometry'),geom2=geometry(h,'geometry2');h.mesh=mesh;mesh.geometry=geom;mesh.material=mat;mesh2.geometry=geom2;mesh2.material=mat2;mesh2.matrixWorld.makeTranslation(0,0,.5);
    if(Object.hasOwn(request,'visible'))mesh.visible=request.visible;if(Object.hasOwn(request,'materialVisible'))mat.visible=request.materialVisible;if(request.cull)mesh.frustumCulled=true;if(request.layerMismatch)mesh.layers.set(2);
    if(request.groups){geom.groups=[{tag:'group',start:0,count:3,materialIndex:1}];mesh.material=[mat2,mat];}
    if(request.sprite){delete mesh.isMesh;mesh.isSprite=true;}if(request.line){delete mesh.isMesh;mesh.isLine=true;}if(request.points){delete mesh.isMesh;mesh.isPoints=true;}
    let children=[mesh,mesh2];if(request.group||request.lod){const parent=node(h,'parent',13,request.lod?'lod':'group');parent.renderOrder=5;parent.children=children;children=[parent];}root.children=children;
    if(request.arrayCamera){cam.isArrayCamera=true;const a=camera(h,'array0',201),b=camera(h,'array1',202);a.viewport=new Vector4(0,0,16,16);b.viewport=new Vector4(16,0,16,16);cam.cameras=[a,b];}
    if(request.light){const light=node(h,'light',21,'light');Object.assign(light,{isLight:true,isAmbientLight:true,color:new Color(0xffffff),intensity:2,castShadow:true});children.push(light);}
    const extensions={};method(h,extensions,'has','extensions.has');bindings.extensions=extensions;bindings.capabilities={samples:4};
    const lists=new WebGLRenderLists(),states=new WebGLRenderStates(extensions);instrument(h,lists,'get','renderLists.get');instrument(h,states,'get','renderStates.get');bindings.renderLists=lists;bindings.renderStates=states;
    for(const [name,keys]of [['objects',['update']],['clipping',['init','beginShadows','endShadows','setGlobalState']],['background',['addToRenderList','render']],['shadowMap',['render']],['textures',['getTextureUnits','setTextureUnits','updateMultisampleRenderTarget','updateRenderTargetMipmap']],['bindingStates',['resetDefaultState']]]){const value={};for(const key of keys)method(h,value,key,name+'.'+key);bindings[name]=value;}
    const state={buffers:{}};for(const name of ['depth','color']){const buffer={};for(const key of ['setTest','setMask'])method(h,buffer,key,'state.'+name+'.'+key);state.buffers[name]=buffer;}method(h,state,'viewport','state.viewport');method(h,state,'setPolygonOffset','state.setPolygonOffset');bindings.state=state;
    const xr={enabled:!!request.xr,isPresenting:!!request.xr,cameraAutoUpdate:true};for(const key of ['updateCamera','getCamera','getDepthSensingMesh','hasDepthSensing'])method(h,xr,key,'xr.'+key);bindings.xr=xr;renderer.xr=xr;
    const depthMesh=node(h,'depthMesh',22);depthMesh.geometry=geometry(h,'depth-geometry');depthMesh.material=mat2;h.depthMesh=depthMesh;
    if(request.output){const output={};for(const key of ['begin','hasRenderPass','isCompositing','end'])method(h,output,key,'output.'+key);bindings.output=output;}
    if(request.nodes){const nodes={};for(const key of ['renderStart','updateLights','setObject','renderEnd'])method(h,nodes,key,'nodes.'+key);bindings._nodesHandler=nodes;mat.isNodeMaterial=true;}
    bindings._isContextLost=!!request.contextLost;
    for(const key of ['getRenderTarget','getActiveCubeFace','getActiveMipmapLevel','setRenderTarget','getClearColor','getClearAlpha','setClearColor','clear'])method(h,renderer,key,'renderer.'+key);method(h,renderer,'renderBufferDirect','draw');
    h.aliases=Object.fromEntries([renderer,cam,cam2,root,inner,mat,mat2,over,mesh,mesh2,geom,geom2,depthMesh].map(value=>[value.tag,value]));if(request.arrayCamera)for(const value of cam.cameras)h.aliases[value.tag]=value;if(request.groups)h.aliases.group=geom.groups[0];
    const pipeline=factory(bindings,imports);h.pipeline=pipeline;renderer.render=pipeline.render;pipeline.get('_currentViewport').set(0,0,64,48);
    if(request.target){const target=new WebGLRenderTarget(64,48);target.tag='target';h.target=target;pipeline.set('_currentRenderTarget',target);}if(request.badCamera)cam.isCamera=false;
    if(request.visibleGetter)Object.defineProperty(mesh,'visible',{configurable:true,get(){event(h,'get:visible');return request.visible;}});return h;
}
function run(request){const h=harness(request),out={};if(request.op==='metadata')return ['render',...privateNames,'WebGLRenderTarget'].map(key=>{const fn=key==='WebGLRenderTarget'?h.pipeline.get(key):h.pipeline[key];let constructible=false;try{Reflect.construct(function(){},[],fn);constructible=true;}catch{}return {key,name:fn.name,length:fn.length,source:fn.toString(),constructible};});try{out.value=clean(h.renderer.render(h.scene,h.camera));}catch(error){out.error=error!==null&&['object','function'].includes(typeof error)?{name:error.name}:{name:'ThrownValue',raw:error};}
    out.events=clean(h.events);out.state={renderListDepth:h.pipeline.get('renderListStack').length,renderStateDepth:h.pipeline.get('renderStateStack').length,currentListNull:h.pipeline.get('currentRenderList')===null,currentStateNull:h.pipeline.get('currentRenderState')===null,materialSide:h.material.side,materialVersion:h.material.version,frame:h.renderer.info.render.frame,toneMapping:h.renderer.toneMapping,textureUnits:h.textureUnits,target:tag(h.target)};return out;
}
const expected=requests.map(run);requests.forEach((request,i)=>assert.deepEqual(expected[i].error,request.expectedError,'Unexpected pinned result '+request.case));
const output=requestedOutput??path.join('D:/Temp/surui','webgl-renderer-scene-'+Date.now());assert(!fs.existsSync(output),'Output must be fresh');fs.mkdirSync(output,{recursive:true});for(const [name,value]of [['requests',requests],['expected',expected]])fs.writeFileSync(path.join(output,name+'.json'),JSON.stringify(value,null,2));
const failures=[];if(probe!=='--oracle-only'){const actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify(requests),encoding:'utf8',maxBuffer:64*1024*1024,timeout:300000,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);requests.forEach((request,i)=>{try{assert.deepEqual(actual[i],expected[i]);}catch(e){failures.push({case:request.case,message:e.message});}});}
const summary={cases:requests.length,successes:expected.filter(x=>!x.error).length,expectedErrors:expected.filter(x=>x.error).length,failures};fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));console.log(JSON.stringify({output,...summary,failures:failures.length}));assert.equal(failures.length,0,JSON.stringify(failures.slice(0,3)));
