import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync, spawnSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T = await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const expectedWarnings=[];console.warn=(...args)=>expectedWarnings.push(args.join(' '));
const num = x => Number.isNaN(x) ? 'NaN' : x === Infinity ? 'Infinity' : x === -Infinity ? '-Infinity' : Object.is(x,-0) ? '-0' : x;
const rect = x => x.toArray().map(num);
function state(t) {
    return {size:[t.width,t.height,t.depth].map(num),scissor:rect(t.scissor),viewport:rect(t.viewport),
        flags:[t.scissorTest,t.depthBuffer,t.stencilBuffer,t.resolveColorBuffer,t.resolveDepthBuffer,t.resolveStencilBuffer,
            t.storeMultisampledColorBuffer,t.storeMultisampledDepthBuffer,t.storeMultisampledStencilBuffer,
            t.multiview,t.useArrayDepthTexture,t.isRenderTarget3D === true],samples:num(t.samples),
        attachments:t.textures.map(x=>({size:[x.image.width,x.image.height,x.image.depth].map(num),version:num(x.version),
            sourceVersion:num(x.source.version),flags:[x.isRenderTargetTexture,x.isArrayTexture,x.isData3DTexture === true],
            owner:x.renderTarget === t,options:[x.mapping,x.wrapS,x.wrapT,x.magFilter,x.minFilter,x.format,x.type,x.anisotropy,
                x.colorSpace,x.flipY,x.generateMipmaps,x.mipmapsAutoUpdate,x.internalFormat,x.unpackAlignment]})),
        sameSource:t.textures.length>1&&t.textures[0].source===t.textures[1].source,
        sameImage:t.textures.length>1&&t.textures[0].image===t.textures[1].image};
}
function compact(target,source=null,destination=null){
    const textures=[];for(let i=0;i<target.textures.length;i++){
        if(!(i in target.textures)){textures.push('hole');continue;}const texture=target.textures[i];if(!texture){textures.push(null);continue;}
        const image=texture.image,descriptor=Object.getOwnPropertyDescriptor(target.textures,String(i));
        textures.push({name:texture.name,image:image?[image.width,image.height,image.depth].map(num):null,version:num(texture.version),sourceVersion:num(texture.source.version),
            owner:texture.renderTarget===null?'none':texture.renderTarget===source?'source':texture.renderTarget===destination?'destination':'other',
            flags:[texture.isRenderTargetTexture,texture.isArrayTexture,texture.isData3DTexture===true],descriptor:[descriptor.writable,descriptor.enumerable,descriptor.configurable]});
    }
    return {size:[target.width,target.height,target.depth].map(num),scissor:rect(target.scissor),viewport:rect(target.viewport),samples:num(target.samples),lengthWritable:Object.getOwnPropertyDescriptor(target.textures,'length').writable,textures};
}
function copyReentry(mode){
    const source=new T.RenderTarget(4,5,{count:2,depth:2}),destination=new T.RenderTarget(9,10,{count:3});source.samples=7;source.scissor.set(1,2,3,4);source.viewport.set(5,6,7,8);
    const first=new T.Texture({width:4,height:5,depth:2});first.name='callback';source.textures[0]=first;
    const sourceArray=source.textures,destinationArray=destination.textures,trace=[];
    first.clone=function(){
        trace.push({stage:'clone',source:compact(source,source,destination),destination:compact(destination,source,destination)});
        if(mode===1){source.textures[0]=new T.Texture({width:13,height:14,depth:15});source.textures[0].name='replacement';}
        if(mode===2)source.textures.length=0;
        if(mode===3){const added=new T.Texture();added.name='appended';source.textures.push(added);}
        if(mode===4)destination.textures=[new T.Texture()];
        if(mode===5)return first;
        if(mode===6){destination.width=31;source.samples=23;throw new TypeError('stop');}
        if(mode===7)Object.defineProperty(destination.textures,'length',{writable:false});
        if(mode===8){destination.textures.push(new T.Texture());Object.defineProperty(destination.textures,'0',{writable:false});}
        if(mode===9)delete source.textures[1];
        if(mode===10)destination.copy(destination);
        if(mode===11)destination.textures=[];
        if(mode===12){destination.textures=[];Object.defineProperty(destination.textures,'length',{writable:false});}
        if(mode===13){Object.defineProperty(destinationArray,'length',{writable:false});destination.textures=[new T.Texture(),new T.Texture()];}
        if(mode===14){destination.textures=[new T.Texture()];Object.defineProperty(destination.textures,'0',{writable:false});}
        return T.Texture.prototype.clone.call(first);
    };
    let error='';try{destination.copy(source)}catch(e){error=e.name;}
    return {audit:'copy-reentry',mode,trace,error,source:compact(source,source,destination),destination:compact(destination,source,destination),sourceArraySame:sourceArray===source.textures,destinationArraySame:destinationArray===destination.textures,oldDestinationLength:destinationArray.length};
}
function setValuesReentry(mode){
    const target=new T.RenderTarget(3,4,{count:2}),trace=[],first=new T.Texture({width:3,height:4,depth:1}),extra=new T.Texture({width:7,height:8,depth:1});first.name='callback';extra.name='extra';target.textures[0]=first;
    extra.setValues=function(values){trace.push('extra');T.Texture.prototype.setValues.call(extra,values);};
    first.setValues=function(values){trace.push(structuredClone(values));T.Texture.prototype.setValues.call(first,values);
        if(mode===1)target.textures.length=0;if(mode===2)target.textures.push(extra);if(mode===3)target.textures=[first,extra];if(mode===4)delete target.textures[1];if(mode===5)throw new TypeError('stop');};
    let error='';try{target._setTextureOptions({mapping:123,flipY:true,generateMipmaps:true,internalFormat:'R8'})}catch(e){error=e.name;}
    return {audit:'setValues-reentry',mode,trace,error,target:compact(target)};
}
function descriptorCase(mode){
    const source=new T.RenderTarget(4,5,{count:2}),destination=new T.RenderTarget(9,10,{count:3});
    if(mode===0)Object.defineProperty(destination.textures,'length',{writable:false});
    if(mode===1)Object.defineProperty(destination.textures,'1',{configurable:false});
    if(mode===2)Object.defineProperty(destination.textures,'2',{configurable:false});
    if(mode===3){Object.defineProperty(destination.textures,'0',{configurable:false});delete destination.textures[2];}
    if(mode===4)delete source.textures[1];if(mode===5)source.textures[1]=null;if(mode===6)destination.textures=source.textures;
    let error='';try{destination.copy(source)}catch(e){error=e.name;}
    return {audit:'descriptors',mode,error,source:compact(source,source,destination),destination:compact(destination,source,destination)};
}
function getters(mode){
    const options={},trace=[];let reads=0;
    Object.defineProperty(options,'minFilter',{enumerable:true,configurable:true,get(){const n=++reads;trace.push(`min:${n}`);if(mode===1&&n===1){delete options.flipY;options.magFilter=T.NearestFilter;}if(mode===2&&n===1)return undefined;if(mode===3&&n===2)return undefined;return n%2?T.NearestFilter:T.LinearFilter;}});
    Object.defineProperty(options,'flipY',{enumerable:true,configurable:true,get(){trace.push('flip');return true;}});options.count=2;options.wrapR=T.RepeatWrapping;
    let target;if(mode<4){target=new T.RenderTarget();target._setTextureOptions(options);}else if(mode===4)target=new T.RenderTarget(3,4,options);else target=new T.RenderTarget3D(3,4,5,options);
    return {audit:'getters',mode,trace,reads,wrapR:target.textures.map(t=>t.isData3DTexture?t.wrapR:'undefined'),state:state(target)};
}
function resizeFailure(mode){
    const target=mode>=5?new T.RenderTarget3D(3,4,2,{count:2}):new T.RenderTarget(3,4,{count:2}),trace=[];target.viewport.set(1,2,3,4);target.scissor.set(5,6,7,8);
    target.addEventListener('dispose',()=>{trace.push(compact(target));if(mode===7)target.textures.length=0;});
    if(mode===0)delete target.textures[1];if(mode===1)target.textures[1]=null;if(mode===2)target.textures[1].image=null;if(mode===3)target.textures[0].image=null;
    if(mode===4){Object.defineProperty(target.textures,'0',{writable:false});Object.defineProperty(target.textures,'length',{writable:false});}
    if(mode===5)target.textures[0].isArrayTexture=true;if(mode===6)target.textures[0].isData3DTexture=false;
    let error='';try{target.setSize(7,8,3)}catch(e){error=e.name;}
    return {audit:'resize-partial',mode,error,trace,state:compact(target)};
}
function pixelsCopy(){
    const source=new T.RenderTarget(2,2,{count:2}),pixels=new Float32Array([1,2,3,4]);source.textures[0].image.data=pixels;const clone=source.clone(),first=clone.textures[0].image,second=clone.textures[1].image;first.data[1]=19;
    return {audit:'image-data-identity',imageDifferent:first!==source.textures[0].image,cloneImagesDifferent:first!==second,cloneSourcesDifferent:clone.textures[0].source!==clone.textures[1].source,
        pixelsShared:pixels===first.data,sourceValue:pixels[1],secondValue:second.data[1],sourceVersion:source.textures[0].source.version,cloneVersions:[clone.textures[0].source.version,clone.textures[1].source.version]};
}
const expected=[];
for(const volume of [false,true]) for(const count of [0,1,2,2.5,-1,NaN]) {
    const options={count,depth:3,samples:2.5,depthBuffer:false,resolveColorBuffer:false,storeMultisampledDepthBuffer:false,
        multiview:true,useArrayDepthTexture:true,magFilter:T.NearestFilter,wrapS:T.RepeatWrapping,colorSpace:T.SRGBColorSpace};
    const t=volume?new T.RenderTarget3D(4,5,7,options):new T.RenderTarget(4,5,options);
    const steps=[state(t)],events=[];
    t.addEventListener('dispose',e=>events.push({target:e.target===t,state:state(t)}));
    t.viewport.set(1,2,3,4);t.scissor.set(4,3,2,1);t.setSize(4,5,volume?7:3);steps.push(state(t));
    t.setSize(-2.5,0,1);steps.push(state(t));t.setSize(NaN,-0,2);t.setSize(NaN,-0,2);steps.push(state(t));
    const copy=t.clone();steps.push(state(copy));
    const cloneIdentity=copy.textures.map((x,i)=>[x===t.textures[i],x.source===t.textures[i].source,x.image===t.textures[i].image,x.mipmaps===t.textures[i].mipmaps]);
    expected.push({steps,events,cloneIdentity});
}
{
    const a=new T.RenderTarget(),b=new T.RenderTarget(),c=new T.RenderTarget(),depth=new T.DepthTexture(3,4),trace=[];
    a.depthTexture=depth;b.depthTexture=depth;trace.push([depth.renderTarget===a,a.depthTexture===b.depthTexture]);
    const ac=a.clone(),bc=b.clone();trace.push([ac.depthTexture===depth,ac.depthTexture.renderTarget===ac,ac.depthTexture.source===depth.source,bc.depthTexture===depth,bc.depthTexture.renderTarget===a]);
    b.depthTexture=null;trace.push(depth.renderTarget===a);a.depthTexture=null;trace.push(depth.renderTarget===null);
    b.depthTexture=depth;c.depthTexture=depth;b.copy(c);trace.push([depth.renderTarget===b,b.depthTexture===c.depthTexture]);expected.push(trace);
}
{
    const t=new T.RenderTarget(3,4,{count:2}),array=t.textures,old=t.texture,replacement=new T.Texture();t.texture=replacement;
    expected.push([array[0]===replacement,replacement.renderTarget===null,!replacement.isRenderTargetTexture,old.renderTarget===t]);
    t.copy(t);expected.push(state(t));
}
{
    const t=new T.RenderTarget(2,3),events=[];let nested=false;
    t.addEventListener('dispose',()=>{events.push(state(t));if(!nested){nested=true;t.setSize(9,8,4);}});
    t.setSize(5,6,2);expected.push({events,final:state(t)});
}
{
    const t=new T.RenderTarget(2,3);t.addEventListener('dispose',()=>{throw new TypeError('stop');});
    try{t.setSize(5,6,2);}catch{}expected.push(state(t));
}
{
    const t=new T.RenderTarget(),trace=[],options={};
    for(const key of ['minFilter','flipY','samples'])Object.defineProperty(options,key,{enumerable:true,get(){trace.push(key);return key==='flipY'?true:T.NearestFilter;}});
    t._setTextureOptions(options);expected.push({trace:[...trace],state:state(t)});trace.length=0;
    const constructed=new T.RenderTarget(2,3,options);expected.push({trace:[...trace],state:state(constructed)});
}
for(let mode=0;mode<15;mode++)expected.push(copyReentry(mode));
for(let mode=0;mode<6;mode++)expected.push(setValuesReentry(mode));
for(let mode=0;mode<7;mode++)expected.push(descriptorCase(mode));
for(let mode=0;mode<6;mode++)expected.push(getters(mode));
for(let mode=0;mode<8;mode++)expected.push(resizeFailure(mode));
expected.push(pixelsCopy());
const run=spawnSync(process.argv[3],[],{encoding:'utf8',maxBuffer:32*1024*1024});assert.equal(run.status,0,run.stderr||String(run.error));
assert.deepEqual(run.stderr.split(/\r?\n/).filter(Boolean),expectedWarnings,'warning trace');
const actual=JSON.parse(run.stdout);
let comparisons=0;
function compare(a,b,where='root'){
    if(a&&typeof a==='object'){assert.equal(Array.isArray(a),Array.isArray(b),where);assert.deepEqual(Object.keys(a),Object.keys(b),where);for(const k of Object.keys(a))compare(a[k],b[k],`${where}.${k}`);}
    else {assert.deepEqual(a,b,where);comparisons++;}
}
assert.equal(actual.length,expected.length);
const failures=[];for(let i=0;i<expected.length;i++){try{compare(actual[i],expected[i],`root.${i}`);}catch(error){failures.push(`${expected[i]?.audit??'baseline'}:${expected[i]?.mode??i} ${error.message}`);}}
assert.equal(failures.length,0,failures.join('\n'));
console.log(`RenderTarget parity: ${expected.length} scenarios / ${comparisons} exact comparisons; ${lock.commit}`);
