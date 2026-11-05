import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
const [upstream,probe,requestedOutput]=process.argv.slice(2);
assert(upstream&&probe,'Usage: compare-webgl-textures.mjs <pinned> <probe|--oracle-only> [fresh-directory]');
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const load=file=>import(pathToFileURL(path.join(upstream,'src',file)).href);
const {WebGLTextures}=await load('renderers/webgl/WebGLTextures.js'),{WebGLProperties}=await load('renderers/webgl/WebGLProperties.js');
const constants=await load('constants.js');
const types={};for(const name of ['Texture','DataTexture','CubeTexture','DataArrayTexture','CompressedTexture','DepthTexture'])types[name]=(await load('textures/'+name+'.js'))[name];
const {RenderTarget}=await load('core/RenderTarget.js');
const enums=JSON.parse(fs.readFileSync(new URL('./webgl-textures-enums.json',import.meta.url)));
const requests=JSON.parse(fs.readFileSync(new URL('./webgl-textures-fixtures.json',import.meta.url)));
const mappings={UnsignedByteType:'UNSIGNED_BYTE',UnsignedShortType:'UNSIGNED_SHORT',UnsignedIntType:'UNSIGNED_INT',FloatType:'FLOAT',HalfFloatType:'HALF_FLOAT',ByteType:'BYTE',ShortType:'SHORT',IntType:'INT',RGBAFormat:'RGBA',RGBFormat:'RGB',RedFormat:'RED',RGFormat:'RG',RedIntegerFormat:'RED_INTEGER',RGIntegerFormat:'RG_INTEGER',RGBAIntegerFormat:'RGBA_INTEGER',DepthFormat:'DEPTH_COMPONENT',DepthStencilFormat:'DEPTH_STENCIL',UnsignedInt248Type:'DEPTH24_STENCIL8'};
function metadata(){return {name:WebGLTextures.name,length:WebGLTextures.length,source:WebGLTextures.toString()};}
function makeContext(request){
    const c={request,events:[],textures:[],sources:[],images:[],targets:[],handles:[],unpack:{},properties:new WebGLProperties(),info:{memory:{textures:0},render:{frame:0}},thrown:false};
    function encode(v){
        if(v===undefined)return {$undefined:true};
        if(typeof v==='number'&&!Number.isFinite(v))return {$number:Number.isNaN(v)?'NaN':v<0?'-Infinity':'Infinity'};
        if(v===null||typeof v!=='object')return v;
        for(const [key,values]of [['texture',c.textures],['source',c.sources],['target',c.targets],['handle',c.handles],['image',c.images]])if(values.includes(v))return {[key]:values.indexOf(v)};
        if(ArrayBuffer.isView(v))return {$typed:v.constructor.name,values:Array.from(v)};
        if(Array.isArray(v))return v.map(encode);
        return Object.fromEntries(Object.keys(v).filter(key=>typeof v[key]!=='function').map(key=>[key,encode(v[key])]));
    }
    c.encode=encode;
    const record=(name,args)=>{
        c.events.push([name,...args.map(encode)]);
        if(request.variant==='glThrow'&&!c.thrown&&name==='state.texSubImage2D'){c.thrown=true;throw 'gpu-command';}
        if(name.startsWith('gl.create')){const value={resource:name.slice(9),ordinal:c.handles.length};c.handles.push(value);return value;}
        if(name==='state.pixelStorei'){c.unpack[args[0]]=args[1];return;}
        if(name==='state.getParameter')return c.unpack[args[0]]??0;
        if(name==='state.bindFramebuffer')return true;
        return undefined;
    };
    c.gl={...enums,canvas:{width:8,height:8}};
    for(const name of ['createTexture','deleteTexture','createFramebuffer','deleteFramebuffer','createRenderbuffer','deleteRenderbuffer','bindRenderbuffer','framebufferRenderbuffer','framebufferTexture2D','renderbufferStorage','renderbufferStorageMultisample','generateMipmap','blitFramebuffer','invalidateFramebuffer','texParameterf','texParameteri','texElementImage2D'])c.gl[name]=(...args)=>record('gl.'+name,args);
    c.state={};for(const name of ['activeTexture','bindTexture','unbindTexture','bindFramebuffer','pixelStorei','getParameter','texImage2D','texImage3D','texStorage2D','texStorage3D','texSubImage2D','texSubImage3D','compressedTexImage2D','compressedTexImage3D','compressedTexSubImage2D','compressedTexSubImage3D'])c.state[name]=(...args)=>record('state.'+name,args);
    c.extensions={has(name){c.events.push(['extensions.has',name]);return false;},get(name){c.events.push(['extensions.get',name]);return null;}};
    c.capabilities={maxTextures:request.maxTextures??16,maxTextureSize:1024,maxCubemapSize:1024,maxSamples:4,getMaxAnisotropy(){c.events.push(['maxAnisotropy']);return 16;}};
    c.utils={convert(value){c.events.push(['convert',encode(value)]);for(const [key,gl]of Object.entries(mappings))if(constants[key]===value)return enums[gl];if(value===constants.RGB_S3TC_DXT1_Format)return 33776;return value;}};
    c.manager=new WebGLTextures(c.gl,c.extensions,c.state,c.properties,c.capabilities,c.utils,c.info);
    c.enroll=tex=>{if(!c.textures.includes(tex))c.textures.push(tex);if(!c.sources.includes(tex.source))c.sources.push(tex.source);const image=tex.image;if(image&&typeof image==='object'&&!c.images.includes(image))c.images.push(image);return tex;};
    return c;
}
function makeTexture(c,name){
    const data=new Uint8Array(Array.from({length:64},(_,i)=>i));let t;
    if(name==='data')t=new types.DataTexture(data,4,4);
    else if(name==='array')t=new types.DataArrayTexture(data,2,2,4);
    else if(name==='depth')t=new types.DepthTexture(4,4);
    else if(name==='compressed'||name==='compressedArray'){
        t=new types.CompressedTexture([{width:4,height:4,data},{width:2,height:2,data:new Uint8Array(8)}],4,4,constants.RGB_S3TC_DXT1_Format);
        if(name==='compressedArray'){t.isCompressedArrayTexture=true;t.image.depth=2;t.wrapR=constants.ClampToEdgeWrapping;t.layerUpdates=new Set();t.clearLayerUpdates=function(){this.layerUpdates.clear();};}
    }else if(name==='cube'||name==='cubeData'){
        const faces=Array.from({length:6},()=>name==='cubeData'?new types.DataTexture(data,4,4):{width:4,height:4,data});t=new types.CubeTexture(faces);for(const face of faces)if(name==='cubeData')c.enroll(face);else c.images.push(face);
    }else{
        t=new types.Texture({width:4,height:4,data});
        if(name==='data3d'){t.isData3DTexture=true;t.image.depth=4;t.wrapR=constants.ClampToEdgeWrapping;t.magFilter=t.minFilter=constants.NearestFilter;t.generateMipmaps=false;t.flipY=false;t.unpackAlignment=1;}
        if(name==='video'){t.isVideoTexture=true;t.generateMipmaps=false;t.update=function(){c.events.push(['video.update']);};}
        if(name==='external'){t.isExternalTexture=true;t.sourceTexture={external:true};c.handles.push(t.sourceTexture);}
    }
    c.enroll(t);t.needsUpdate=true;return t;
}
const invoke=(self,key,...args)=>self[key](...args);
function snapshot(c){return {properties:c.textures.map(t=>c.encode(c.properties.get(t))),sources:c.sources.map(s=>c.encode(c.properties.get(s))),memory:c.encode(c.info.memory),ranges:c.textures.map(t=>c.encode(t.updateRanges)),versions:c.textures.map(t=>[t.version,t.source.version,t.source.dataReady]),units:c.manager.getTextureUnits()};}
function safe(c,fn){try{const value=fn();return {value:c.encode(value),state:snapshot(c)};}catch(error){return {error:error&&typeof error==='object'?{name:error.name}:{raw:error},state:snapshot(c)};}}
function upload(c,t,slot=1){const name=t.isCubeTexture?'setTextureCube':t.isDataArrayTexture||t.isCompressedArrayTexture?'setTexture2DArray':t.isData3DTexture?'setTexture3D':'setTexture2D';return c.manager[name](t,slot);}
function run(request){
    if(request.kind==='metadata')return metadata();const c=makeContext(request),rows=[];
    if(request.kind==='units'){for(let i=0;i<4;++i)rows.push(c.manager.allocateTextureUnit());c.manager.setTextureUnits('3');rows.push(c.manager.allocateTextureUnit());c.manager.resetTextureUnits();rows.push(c.manager.getTextureUnits());return {rows,events:c.events};}
    if(request.kind==='upload'){
        const t=makeTexture(c,request.class),variant=request.variant;
        if(variant==='notReady')t.source.dataReady=false;if(variant==='layers'){t.layerUpdates.add(0);t.layerUpdates.add(1);}if(variant==='halfFloat')t.type=constants.HalfFloatType;if(variant==='srgb')t.colorSpace=constants.SRGBColorSpace;
        if(variant==='ranges'){for(const [start,count]of [[12,4],[0,4],[3,5],[32,8],[24,8],[16,8]])t.addUpdateRange(start,count);}
        let once=false;t.onUpdate=function(value){c.events.push(['onUpdate',value===t,this===t]);if(variant==='onUpdateThrow'&&!once){once=true;throw 'onUpdate';}if(variant==='onUpdateReenter'&&!once){once=true;t.source.needsUpdate=true;t.needsUpdate=true;upload(c,t,2);}};
        rows.push(safe(c,()=>upload(c,t)));rows.push(safe(c,()=>upload(c,t)));
        if(variant==='sourceOnly')t.source.needsUpdate=true;
        if(variant==='textureOnly')t.version++;
        if(variant==='notReady'){t.source.dataReady=true;t.needsUpdate=true;}
        if(variant==='samplerChange'){t.wrapS=constants.RepeatWrapping;t.needsUpdate=true;}
        if(variant==='dispose')t.dispose();
        rows.push(safe(c,()=>upload(c,t)));c.info.render.frame++;rows.push(safe(c,()=>upload(c,t)));rows.push(safe(c,()=>t.dispose()));rows.push(safe(c,()=>c.manager.dispose()));
    }else if(request.kind==='shared'){
        const a=makeTexture(c,'data'),b=makeTexture(c,'data');if(request.shared)b.source=a.source;if(request.variant==='cacheDiff')b.wrapS=constants.RepeatWrapping;
        let once=false;a.onUpdate=function(){c.events.push(['a.onUpdate']);if(request.variant==='uploadThrow'&&!once){once=true;throw 'first';}if(request.variant==='onUpdateDispose')a.dispose();};
        rows.push(safe(c,()=>upload(c,a)));rows.push(safe(c,()=>upload(c,b)));
        if(request.variant==='sourceChange'){b.source=makeTexture(c,'data').source;b.needsUpdate=true;}
        if(request.variant==='disposeFirst')a.dispose();if(request.variant==='disposeSecond')b.dispose();
        rows.push(safe(c,()=>upload(c,b)));rows.push(safe(c,()=>upload(c,a)));rows.push(safe(c,()=>c.manager.dispose()));
    }else if(request.kind==='target'){
        const target=new RenderTarget(4,4,{count:request.count,depthBuffer:request.depth!=='none',stencilBuffer:request.stencil??false,samples:request.samples,generateMipmaps:request.mipmaps??false});
        if(request.cube){target.isWebGLCubeRenderTarget=true;target.texture=new types.CubeTexture(Array.from({length:6},()=>({width:4,height:4})));target.texture.isRenderTargetTexture=true;target.texture.generateMipmaps=false;}
        if(request.depth==='texture'){target.depthTexture=new types.DepthTexture(4,4);if(request.cube){target.depthTexture.isCubeTexture=true;target.depthTexture.isCubeDepthTexture=true;target.depthTexture.mapping=constants.CubeReflectionMapping;const image={width:4,height:4,depth:1};target.depthTexture.image=Array(6).fill(image);}}
        c.targets.push(target);for(const t of target.textures)c.enroll(t);if(target.depthTexture)c.enroll(target.depthTexture);
        rows.push(safe(c,()=>c.manager.setupRenderTarget(target)));if(request.rebind)rows.push(safe(c,()=>c.manager.rebindTextures(target,target.texture,target.depthTexture)));rows.push(safe(c,()=>c.manager.updateRenderTargetMipmap(target)));rows.push(safe(c,()=>c.manager.updateMultisampleRenderTarget(target)));rows.push(safe(c,()=>c.manager.setupDepthRenderbuffer(target)));rows.push(safe(c,()=>target.dispose()));rows.push(safe(c,()=>c.manager.dispose()));
    }
    return {rows,events:c.events};
}
const expected=requests.map(run);
for(let i=0;i<requests.length;++i){
    const r=requests[i];const errors=(expected[i].rows??[]).filter(v=>v&&typeof v==='object'&&v.error).map(v=>v.error);
    let wanted=[];
    if(r.kind==='upload'&&r.variant==='onUpdateThrow'&&r.class!=='external')wanted=[{raw:'onUpdate'}];
    if(r.kind==='upload'&&r.variant==='glThrow'&&['data','plain','cube','cubeData'].includes(r.class))wanted=[{raw:'gpu-command'}];
    if(r.kind==='shared'&&r.variant==='uploadThrow')wanted=[{raw:'first'}];
    if(r.kind==='shared'&&r.variant==='sourceChange')wanted=[{name:'TypeError'}];
    assert.deepEqual(errors,wanted,'Unexpected source behavior '+r.case);
}
const output=requestedOutput??path.join('D:/Temp/surui','webgl-textures-'+Date.now());assert(!fs.existsSync(output),'Fresh output required');fs.mkdirSync(output,{recursive:true});
for(const [key,value]of [['requests',requests],['expected',expected],['enums',enums]])fs.writeFileSync(path.join(output,key+'.json'),JSON.stringify(value,null,2));
const failures=[];
if(probe!=='--oracle-only'){
    const actual=JSON.parse(execFileSync(path.resolve(probe),{input:JSON.stringify({requests,enums}),encoding:'utf8',timeout:300000,maxBuffer:256*1024*1024,env:{...process.env,TEMP:'D:/Temp/surui',TMP:'D:/Temp/surui'}}).trim().split(/\r?\n/).at(-1));fs.writeFileSync(path.join(output,'actual.json'),JSON.stringify(actual,null,2));assert.equal(actual.length,expected.length);for(let i=0;i<requests.length;++i)try{assert.deepEqual(actual[i],JSON.parse(JSON.stringify(expected[i])));}catch(error){failures.push({case:requests[i].case,message:error.message});}
}
const summary={cases:requests.length,boundary:'CPU command recorder; no GPU execution claimed',expectedErrors:expected.reduce((n,v)=>n+(v.rows??[]).filter(r=>r.error).length,0),failures};fs.writeFileSync(path.join(output,'summary.json'),JSON.stringify(summary,null,2));console.log(JSON.stringify({output,...summary,failures:failures.length}));assert.equal(failures.length,0,JSON.stringify(failures.slice(0,3)));
