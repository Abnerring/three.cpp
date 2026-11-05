// Execute pinned source functions only in this development-time oracle.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {pathToFileURL} from 'node:url';
import {execFileSync,spawnSync} from 'node:child_process';
const upstream=path.resolve(process.argv[2]??'D:/Temp/surui/threejs-pinned-11738a-20261104');
const expectedOnly=process.argv[3]==='--expected-only',binary=expectedOnly?undefined:process.argv[3],directory=process.argv[4];
const commit=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url))).commit;
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),commit);
assert.equal(execFileSync('git',['-C',upstream,'status','--porcelain','--untracked-files=no','--','src'],{encoding:'utf8'}).trim(),'','Pinned source must be clean');
const {parse}=createRequire(path.join(upstream,'package.json'))('acorn');
const text=fs.readFileSync(path.join(upstream,'src/renderers/WebGLRenderer.js'),'utf8');
const rendererAST=parse(text,{sourceType:'module',ecmaVersion:'latest'}).body.find(n=>n.type==='ClassDeclaration'&&n.id.name==='WebGLRenderer');
const body=rendererAST.body.body.find(n=>n.kind==='constructor').value.body.body;
const {WebGLRenderTarget}=await import(pathToFileURL(path.join(upstream,'src/renderers/WebGLRenderTarget.js')));
const {RenderTarget}=await import(pathToFileURL(path.join(upstream,'src/core/RenderTarget.js')));
const {EventDispatcher}=await import(pathToFileURL(path.join(upstream,'src/core/EventDispatcher.js')));
const {Texture}=await import(pathToFileURL(path.join(upstream,'src/textures/Texture.js')));
const {DepthTexture}=await import(pathToFileURL(path.join(upstream,'src/textures/DepthTexture.js')));
const {Vector4}=await import(pathToFileURL(path.join(upstream,'src/math/Vector4.js')));
const {WebGLProperties}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLProperties.js')));
const names=['getActiveCubeFace','getActiveMipmapLevel','getRenderTarget','setRenderTargetTextures','setRenderTargetFramebuffer','setRenderTarget','getPixelRatio','setPixelRatio','getSize','setSize','getDrawingBufferSize','setDrawingBufferSize','getCurrentViewport','getViewport','setViewport','getScissor','setScissor','getScissorTest','setScissorTest','setOpaqueSort','setTransparentSort'];
const sourceMethods=names.map(name=>{const n=body.find(n=>n.type==='ExpressionStatement'&&n.expression.type==='AssignmentExpression'&&n.expression.left.object?.type==='ThisExpression'&&n.expression.left.property.name===name);return `this.${name}=${text.slice(n.expression.right.start,n.expression.right.end)};`;}).join('\n');
const ratio=body.find(n=>n.type==='FunctionDeclaration'&&n.id.name==='getTargetPixelRatio');
const make=new Function('bindings','Vector4',`const {properties,state,textures,_gl,canvas,xr,_scratchFramebuffer}=bindings;
let _currentActiveCubeFace=0,_currentActiveMipmapLevel=0,_currentRenderTarget=null,_currentMaterialId=-1;
const _currentViewport=new Vector4(),_currentScissor=new Vector4();let _currentScissorTest=null;
let _width=canvas.width,_height=canvas.height,_pixelRatio=1.5,_opaqueSort=null,_transparentSort=null;
const _viewport=new Vector4(0,0,_width,_height),_scissor=new Vector4(0,0,_width,_height);let _scissorTest=false,output=null;
const warn=()=>{throw Error('unexpected warning')};${text.slice(ratio.start,ratio.end)}
${sourceMethods}
this.snapshot=()=>({target:clean(_currentRenderTarget),face:_currentActiveCubeFace,mip:_currentActiveMipmapLevel,viewport:_currentViewport.toArray(),scissor:_currentScissor.toArray(),scissorTest:_currentScissorTest,material:_currentMaterialId});
return this;`.replace('target:clean(_currentRenderTarget)','target:_currentRenderTarget===null?null:_currentRenderTarget.tag'));
function clean(value){if(value===undefined)return 'undefined';if(value===null||typeof value!=='object')return value;if(typeof value.tag==='string')return value.tag;if(value.isVector4)return value.toArray();if(Array.isArray(value))return value.map(clean);return 'object';}
function harness(mode){
    const h={mode,trace:[],reentered:false,properties:new WebGLProperties()},state={},textures={};
    const _gl={FRAMEBUFFER:36160,COLOR_ATTACHMENT0:36064,TEXTURE_CUBE_MAP_POSITIVE_X:34069,TEXTURE_2D:3553};
    const canvas={width:20,height:12,style:{}},xr={isPresenting:false};
    function service(name,...args){h.trace.push([name,...args.map(clean)]);const target=args[0];
        if(name==='textures.setupRenderTarget'){const p=h.properties.get(target);p.__webglFramebuffer=h.framebuffer;p.__webglMultisampledFramebuffer={tag:'multisampled'};
            if(mode==='setupThrow')throw 'setup-stop';if(mode==='setupReentry'&&!h.reentered){h.reentered=true;h.renderer.setRenderTarget(null,2,0);}}
        if(name==='state.bindFramebuffer'){if(mode==='bindThrow')throw 'bind-stop';if(mode==='bindReentry'&&!h.reentered){h.reentered=true;h.renderer.setRenderTarget(h.nested,3,0);}return mode!=='alreadyBound';}
        if(name==='state.drawBuffers'&&mode==='drawBuffersThrow')throw 'draw-buffers-stop';if(name==='textures.useMultisampledRTT')return mode==='multisampleRTT';
    }
    for(const name of ['bindFramebuffer','drawBuffers','viewport','scissor','setScissorTest'])state[name]=(...args)=>service('state.'+name,...args);
    for(const name of ['setupRenderTarget','rebindTextures','setupDepthRenderbuffer','useMultisampledRTT'])textures[name]=(...args)=>service('textures.'+name,...args);
    for(const name of ['framebufferTexture2D','framebufferTextureLayer'])_gl[name]=(...args)=>service('gl.'+name,...args);
    h.renderer=make.call({}, {properties:h.properties,state,textures,_gl,canvas,xr,_scratchFramebuffer:{tag:'scratch'}},Vector4);h.canvas=canvas;h.xr=xr;
    h.target=new WebGLRenderTarget(8,6);h.target.tag='target';h.target.viewport.set(1,2,3,4);h.target.scissor.set(2,1,4,3);h.target.scissorTest=true;
    h.nested=new WebGLRenderTarget(4,3);h.nested.tag='nested';h.properties.get(h.nested).__webglFramebuffer={tag:'nested-framebuffer'};h.framebuffer={tag:'framebuffer'};return h;
}
function run(mode){const h=harness(mode),target=h.target,r=h.renderer,p=h.properties.get(target);let face=0,mip=0;
    if(!['fresh','setupThrow','setupReentry'].includes(mode))p.__webglFramebuffer=h.framebuffer;
    if(mode==='externalDefault')r.setRenderTargetFramebuffer(target);
    if(mode==='externalStored')r.setRenderTargetFramebuffer(target,{tag:'external-framebuffer'});
    if(mode==='externalTextures'){target.depthTexture={renderTarget:null};r.setRenderTargetTextures(target,{tag:'external-color'},{tag:'external-depth'});}
    if(mode==='depthSwap'||mode==='badDepth'){target.depthTexture={renderTarget:null,image:{width:mode==='badDepth'?7:8,height:6}};h.properties.get(target.depthTexture);}
    if(mode==='existing')p.__boundDepthTexture=null;
    if(mode==='cube'||mode==='cubeMip'){target.isWebGLCubeRenderTarget=true;face=2;p.__webglFramebuffer=Array.from({length:6},(_,i)=>mode==='cubeMip'?[{tag:'cube-'+i},{tag:'cube-mip-'+i}]:{tag:'cube-'+i});h.properties.get(target.texture).__webglTexture={tag:'cube-texture'};if(mode==='cubeMip')mip=1;}
    if(['array','volume','compressedArray'].includes(mode)){target.texture[mode==='array'?'isDataArrayTexture':mode==='volume'?'isData3DTexture':'isCompressedArrayTexture']=true;face=3;target.textures[1]=new WebGLRenderTarget().texture;for(let i=0;i<2;i++)h.properties.get(target.textures[i]).__webglTexture={tag:'layer-'+i};}
    if(mode==='multisample'||mode==='multisampleRTT'){target.samples=4;p.__webglMultisampledFramebuffer={tag:'multisampled'};}
    if(mode==='mip'){mip=1;p.__webglFramebuffer=[{tag:'mip0'},{tag:'mip1'}];h.properties.get(target.texture).__webglTexture={tag:'mip-texture'};}
    let error=null;try{r.setRenderTarget(mode==='null'?null:target,face,mip);}catch(e){error=typeof e==='object'?e.message:e;}
    return {mode,trace:h.trace,error,state:r.snapshot()};
}
function sizing(){const h=harness('sizing'),r=h.renderer,v=new Vector4();r.setViewport(1.2,2.4,6.3,4.9);r.setScissor(new Vector4(.4,1.1,3.2,2.5));r.setScissorTest(1);
    const first=r.snapshot();r.setRenderTarget(null);const screen=r.snapshot();r.setRenderTarget(h.target);r.setViewport(1.2,2.4,6.3,4.9);const target=r.snapshot();r.setPixelRatio(2);r.setSize(9.5,7.25,false);r.setDrawingBufferSize(11.25,5.5,1.25);
    return {trace:h.trace,first,screen,target,final:r.snapshot(),viewport:r.getViewport(v).toArray(),scissor:r.getScissor(v).toArray(),scissorTest:r.getScissorTest(),pixelRatio:r.getPixelRatio(),canvas:h.canvas};
}

function targetState(target){const values=target.textures;return {size:[target.width,target.height,target.depth],viewport:target.viewport.toArray(),scissor:target.scissor.toArray(),flags:[target.scissorTest,target.depthBuffer,target.stencilBuffer,target.resolveColorBuffer,target.resolveDepthBuffer,target.resolveStencilBuffer,target.storeMultisampledColorBuffer,target.storeMultisampledDepthBuffer,target.storeMultisampledStencilBuffer,target.multiview,target.useArrayDepthTexture],samples:target.samples,attachments:values.map(texture=>({size:[texture.image.width,texture.image.height,texture.image.depth],owner:texture.renderTarget===target,flags:[texture.isRenderTargetTexture,texture.isArrayTexture,texture.isData3DTexture===true],version:texture.version,sourceVersion:texture.source.version,options:[texture.minFilter,texture.magFilter,texture.format,texture.type,texture.flipY,texture.generateMipmaps,texture.mipmapsAutoUpdate,texture.colorSpace,texture.internalFormat]})),depthNull:target.depthTexture===null,sharedSource:values.length>1&&values[0].source===values[1].source,sharedImage:values.length>1&&values[0].image===values[1].image};}
function dynamicConstruction(C,fields,fail){
    const data={events:[],values:{},fail},options={count:2,depth:3,samples:4};function Derived(){}Derived.prototype=Object.create(C.prototype);
    function DynamicParent(...args){data.events.push('super');data.newTarget=new.target===Derived;data.args=args.map(clean);data.optionsIdentity=args.length>2&&args[2]===options;if(fail==='super')throw 'super-stop';
        const self=Object.create(C.prototype);data.instance=self;for(const name of fields)Object.defineProperty(self,name,{configurable:true,get(){return data.values[name];},set(value){data.events.push(['set',name,clean(value)]);data.values[name]=value;if(fail===name)throw 'setter-stop';}});return self;
    }
    const saved=Object.getPrototypeOf(C);Object.setPrototypeOf(C,DynamicParent);let error=null,returnsParent=false;try{returnsParent=Reflect.construct(C,[17,23,options],Derived)===data.instance;}catch(e){error=e instanceof Error?e.name:e;}finally{Object.setPrototypeOf(C,saved);}
    return {fail,events:data.events,newTarget:data.newTarget,args:data.args,optionsIdentity:data.optionsIdentity,error,returnsParent};
}
function primitiveAllocation(C,primitive){
    const OriginalObject=Object,intrinsic=Object.prototype,old=Object.getOwnPropertyDescriptor(intrinsic,'_setTextureOptions'),method=RenderTarget.prototype._setTextureOptions;
    Object.defineProperty(intrinsic,'_setTextureOptions',{value:method,writable:true,configurable:true});function NewTarget(){}const chosen=Object.create(null);NewTarget.prototype=primitive?17:chosen;if(!primitive)chosen._setTextureOptions=method;
    const poison=Object.create(null);globalThis.Object={prototype:poison,assign:OriginalObject.assign,keys:OriginalObject.keys,defineProperty:OriginalObject.defineProperty};let value,error=null;
    try{value=Reflect.construct(C,[7,9],NewTarget);}catch(e){error=e instanceof Error?e.name:e;}finally{globalThis.Object=OriginalObject;if(old)Object.defineProperty(intrinsic,'_setTextureOptions',old);else delete intrinsic._setTextureOptions;}
    const result={primitive,error,intrinsic:!!value&&Object.getPrototypeOf(value)===intrinsic,chosen:!!value&&Object.getPrototypeOf(value)===chosen,poison:!!value&&Object.getPrototypeOf(value)===poison};if(value)result.initial=targetState(value);return result;
}
function copyGetters(C,fail){
    const source=new C(4,5,{count:2,depth:2,samples:7}),destination=new C(9,10),events=[];let enabled=true;
    for(const name of ['width','height','depth','scissor','scissorTest','viewport','textures','depthBuffer','stencilBuffer','resolveColorBuffer','resolveDepthBuffer','resolveStencilBuffer','storeMultisampledColorBuffer','storeMultisampledDepthBuffer','storeMultisampledStencilBuffer','depthTexture','samples','multiview','useArrayDepthTexture']){
        const value=source[name];Object.defineProperty(source,name,{configurable:true,get(){if(enabled){events.push(name);if(name===fail)throw 'getter-stop';}return value;}});
    }
    let error=null,returnsSelf=false;try{returnsSelf=RenderTarget.prototype.copy.call(destination,source)===destination;}catch(e){error=e instanceof Error?e.name:e;}enabled=false;
    return {fail,events,error,returnsSelf,destination:targetState(destination),source:targetState(source)};
}
function copyAlias(C,mode){
    const source=new C(4,5,{count:2,depth:2}),events=[];let destination=new C(9,10,{count:3});source.samples=7;if(mode==='self')destination=source;if(mode==='sharedArray')destination.textures=source.textures;
    const originalArray=destination.textures,sourceArray=source.textures,first=source.texture,baseClone=first.clone;
    first.clone=function(){events.push(['clone',this===first,source.textures.length,destination.textures.length]);if(mode==='replaceSource')source.textures[0]=new Texture({width:13,height:14,depth:15});if(mode==='replaceDestination')destination.textures=[new Texture()];if(mode==='returnAlias')return this;if(mode==='throw'){destination.width=31;source.samples=23;throw 'clone-stop';}return baseClone.call(this);};
    let error=null,returnsSelf=false;try{returnsSelf=destination.copy(source)===destination;}catch(e){error=e instanceof Error?e.name:e;}
    const aliases=destination.textures.map((item,i)=>({first:item===first,sourceTexture:item===source.textures[i],sourceData:item.source===first.source,firstImage:item.image===first.image}));
    return {mode,events,error,returnsSelf,destination:targetState(destination),source:targetState(source),aliases,originalArrayRetained:originalArray===destination.textures,sourceArrayRetained:sourceArray===source.textures,oldLength:originalArray.length};
}
function cloneIdentity(C,mode){
    const source=new C(6,7,{count:2,depth:3,samples:4,multiview:true,useArrayDepthTexture:true}),depth=new DepthTexture(6,7),owner={};if(mode==='external')depth.renderTarget=owner;source.depthTexture=depth;source.viewport.set(1,2,3,4);source.scissor.set(2,1,4,3);
    const events=[];source.addEventListener('dispose',()=>events.push('source disposed'));const copy=source.clone(),copiedDepth=copy.depthTexture,first=source.texture,other=copy.texture;
    const result={mode,prototype:Object.getPrototypeOf(copy)===C.prototype,constructor:copy.constructor===C,state:targetState(copy),distinctTexture:other!==first,distinctSource:other.source!==first.source,distinctImage:other.image!==first.image,sourceOwner:first.renderTarget===source,distinctDepth:copiedDepth!==depth,sourceDepthOwner:depth.renderTarget===source,copyDepthOwner:copiedDepth.renderTarget===copy,externalDepthOwner:copiedDepth.renderTarget===owner};copy.dispose();result.cloneListenerCopied=events.length!==0;source.dispose();result.events=events;return result;
}
function dynamicClone(C,fail){
    const source=new C(),events=[],replacement={};Object.defineProperty(replacement,'copy',{configurable:true,get(){events.push('get copy');return function(...args){events.push(['call copy',this===replacement,args[0]===source]);if(fail)throw 'copy-stop';return 17;};}});
    function Replacement(...args){events.push(['construct',args.length,new.target===Replacement]);return replacement;}
    Object.defineProperty(source,'constructor',{configurable:true,get(){events.push('get constructor');return Replacement;}});let value,error=null;try{value=source.clone();}catch(e){error=e instanceof Error?e.name:e;}return {fail,events,error,value:clean(value)};
}
function lifecycle(C,mode){
    const target=new C(4,5),first=target.texture,image=first.image,source=first.source,events=[];let done=false,last;
    function second(event){events.push(['second',this===target,event.target===this]);}function third(event){events.push(['third',this===target,event.target===this]);}
    function listener(event){last=event;events.push(['first',this===target,event.target===this,[this.width,this.height,this.depth],this.viewport.toArray(),this.scissor.toArray()]);if(mode==='throw')throw 'dispose-stop';if(mode==='mutateListeners'){this.removeEventListener('dispose',second);this.addEventListener('dispose',third);}if(mode==='reenter'&&!done){done=true;this.setSize(11,12,2);}}
    target.addEventListener('dispose',listener);target.addEventListener('dispose',second);let error=null;try{target.setSize(8,9,3);if(mode==='mutateListeners')target.dispose();}catch(e){error=e instanceof Error?e.name:e;}
    return {mode,events,error,state:targetState(target),eventTargetNull:last.target===null,textureSame:target.texture===first,imageSame:target.texture.image===image,sourceSame:target.texture.source===source};
}
function classCases(){
    const construction={},allocation={},copyGetterRows={},copyAliasRows={},clones={},dynamic={},life={},defaults={};
    const fields=['isRenderTarget','width','height','depth','scissor','scissorTest','viewport','textures','depthBuffer','stencilBuffer','resolveColorBuffer','resolveDepthBuffer','resolveStencilBuffer','storeMultisampledColorBuffer','storeMultisampledDepthBuffer','storeMultisampledStencilBuffer','_depthTexture','depthTexture','samples','multiview','useArrayDepthTexture'];
    for(const C of [RenderTarget,WebGLRenderTarget]){const names=C===RenderTarget?fields:['isWebGLRenderTarget'];construction[C.name]=['','super',...names].map(fail=>dynamicConstruction(C,names,fail));allocation[C.name]=[primitiveAllocation(C,true),primitiveAllocation(C,false)];copyGetterRows[C.name]=['','width','textures','depthTexture','samples'].map(fail=>copyGetters(C,fail));copyAliasRows[C.name]=['normal','self','sharedArray','replaceSource','replaceDestination','returnAlias','throw'].map(mode=>copyAlias(C,mode));clones[C.name]=['owned','external'].map(mode=>cloneIdentity(C,mode));dynamic[C.name]=[dynamicClone(C,false),dynamicClone(C,true)];life[C.name]=['normal','throw','mutateListeners','reenter'].map(mode=>lifecycle(C,mode));defaults[C.name]=[targetState(new C()),targetState(new C(undefined,7,{count:2,depth:3,samples:4,stencilBuffer:true,resolveColorBuffer:false,resolveDepthBuffer:false,resolveStencilBuffer:false,storeMultisampledColorBuffer:false,storeMultisampledDepthBuffer:false,storeMultisampledStencilBuffer:false,multiview:true,useArrayDepthTexture:true}))];}
    return {construction,allocation,copyGetters:copyGetterRows,copyAlias:copyAliasRows,clones,dynamicClone:dynamic,lifecycle:life,defaults};
}

const modes=['fresh','null','existing','externalDefault','externalStored','externalTextures','depthSwap','badDepth','cube','cubeMip','array','volume','compressedArray','multisample','multisampleRTT','mip','setupThrow','setupReentry','bindThrow','bindReentry','drawBuffersThrow','alreadyBound'];
function metadata(constructor,parent){const methods={};for(const key of Object.getOwnPropertyNames(constructor.prototype)){if(key==='constructor')continue;const d=Object.getOwnPropertyDescriptor(constructor.prototype,key),row={enumerable:d.enumerable,configurable:d.configurable};if('value'in d)Object.assign(row,{name:d.value.name,length:d.value.length,source:d.value.toString(),writable:d.writable});if(d.get)Object.assign(row,{getName:d.get.name,getSource:d.get.toString()});if(d.set)Object.assign(row,{setName:d.set.name,setSource:d.set.toString()});methods[key]=row;}return {name:constructor.name,length:constructor.length,source:constructor.toString(),parent:Object.getPrototypeOf(constructor)===parent,methods};}
const expected={switching:modes.map(run),sizing:sizing(),metadata:{RenderTarget:metadata(RenderTarget,EventDispatcher),WebGLRenderTarget:metadata(WebGLRenderTarget,RenderTarget)},classes:classCases()};
if(directory){fs.mkdirSync(directory,{recursive:true});fs.writeFileSync(path.join(directory,'expected.json'),JSON.stringify(expected,null,2));}
if(expectedOnly){console.log(`Pinned target oracle: ${modes.length} switching cases and shared sizing cells`);process.exit(0);}
assert.ok(binary,'Expected probe executable or --expected-only');const execution=spawnSync(path.resolve(binary),[],{encoding:'utf8',env:process.env,maxBuffer:4*1024*1024});assert.equal(execution.status,0,execution.stderr??execution.error?.message);
const actual=JSON.parse(execution.stdout);if(directory)fs.writeFileSync(path.join(directory,'actual.json'),JSON.stringify(actual,null,2));let comparisons=0;
function compare(a,b,where='root'){++comparisons;if(typeof a==='number'&&typeof b==='number'){assert.ok(Math.abs(a-b)<=1e-12*Math.max(1,Math.abs(a),Math.abs(b)),`${where}: ${a} vs ${b}`);return;}if(a&&b&&typeof a==='object'&&typeof b==='object'){assert.deepEqual(Object.keys(a),Object.keys(b),where+' keys');for(const key of Object.keys(b))compare(a[key],b[key],where+'.'+key);return;}assert.deepEqual(a,b,where);}
compare(actual,expected);console.log(`Canonical native target switching: ${comparisons} comparisons match ${commit}`);
