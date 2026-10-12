import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {execFileSync} from 'node:child_process';
import {pathToFileURL} from 'node:url';
// Exercise the pinned upload/cache orchestration against a recording GL adapter.
// This checks upstream command order; VulkanRenderer.tests.cpp checks actual Vulkan pixels.
if(process.argv.length!==3)throw new Error('Usage: node verify-webgl-mipmaps.mjs <three.js>');
const upstream=path.resolve(process.argv[2]);
const lock=JSON.parse(fs.readFileSync(new URL('../../../../upstream.json',import.meta.url)));
assert.equal(execFileSync('git',['-C',upstream,'rev-parse','HEAD'],{encoding:'utf8'}).trim(),lock.commit);
const T=await import(pathToFileURL(path.join(upstream,'src/Three.Core.js')));
const {WebGLTextures}=await import(pathToFileURL(path.join(upstream,'src/renderers/webgl/WebGLTextures.js')));
const calls=[];let serial=0;const weak=new WeakMap();
const gl=new Proxy({RGBA:T.RGBAFormat,UNSIGNED_BYTE:T.UnsignedByteType,
createTexture(){return {id:++serial};},deleteTexture(){calls.push(['delete']);},texParameteri(){},
generateMipmap(){calls.push(['generate']);}}, {get(o,k){return k in o?o[k]:o[k]=String(k);}});
const state={activeTexture(){},bindTexture(){},pixelStorei(){},getParameter(){return 0;},
texStorage2D(target,levels,format,width,height){calls.push(['allocate',levels,width,height]);},
texSubImage2D(target,level,x,y,width,height){calls.push(['upload',level,width,height]);}};
const props={get(o){if(!weak.has(o))weak.set(o,{});return weak.get(o);},remove(o){weak.delete(o);}};
const textures=new WebGLTextures(gl,{has(){return false;},get(){return null;}},state,props,
{maxTextureSize:16384,maxTextures:32},{convert(v){return v;}},{memory:{textures:0},render:{frame:0}});
function mip(w,h){return {width:w,height:h,data:new Uint8Array(w*h*4)};}
function texture(){const t=new T.DataTexture(new Uint8Array(8*4*4),8,4);t.minFilter=T.NearestMipmapNearestFilter;t.needsUpdate=true;return t;}
function take(){return calls.splice(0);}
const manual=texture();manual.generateMipmaps=true;manual.mipmaps=[mip(8,4),mip(4,2)];manual.addUpdateRange(0,4);
manual.onUpdate=t=>calls.push(['callback',t.generateMipmaps,t.updateRanges.length]);textures.setTexture2D(manual,0);
assert.deepEqual(take(),[['allocate',4,8,4],['upload',0,8,4],['upload',1,4,2],['callback',false,1]]);
manual.needsUpdate=true;textures.setTexture2D(manual,0);
assert.deepEqual(take(),[['delete'],['allocate',2,8,4],['upload',0,8,4],['upload',1,4,2],['callback',false,1]]);
manual.mipmaps=[mip(4,2)];manual.needsUpdate=true;textures.setTexture2D(manual,0);
assert.deepEqual(take(),[['upload',0,4,2],['callback',false,1]]);
const deferred=texture();deferred.generateMipmaps=true;deferred.mipmapsAutoUpdate=false;textures.setTexture2D(deferred,0);
assert.deepEqual(take(),[['allocate',4,8,4],['upload',0,8,4]]);
deferred.needsUpdate=true;textures.setTexture2D(deferred,0);assert.deepEqual(take(),[['upload',0,8,4]]);
const generated=texture();generated.generateMipmaps=true;textures.setTexture2D(generated,0);
assert.deepEqual(take(),[['allocate',4,8,4],['upload',0,8,4],['generate']]);
generated.mipmaps=[mip(8,4),mip(4,2)];generated.needsUpdate=true;textures.setTexture2D(generated,0);
assert.deepEqual(take(),[['upload',0,8,4],['upload',1,4,2]]);
const paused=texture();paused.generateMipmaps=true;paused.mipmaps=[mip(8,4),mip(4,2)];paused.source.dataReady=false;
paused.onUpdate=t=>calls.push(['callback',t.generateMipmaps]);textures.setTexture2D(paused,0);
assert.deepEqual(take(),[['allocate',4,8,4],['callback',false]]);
const shared=texture();shared.mipmaps=[mip(8,4),mip(4,2)];textures.setTexture2D(shared,0);
assert.deepEqual(take(),[['allocate',2,8,4],['upload',0,8,4],['upload',1,4,2]]);
const clone=shared.clone();textures.setTexture2D(clone,0);
assert.deepEqual(take(),[['upload',0,8,4],['upload',1,4,2]]);
textures.setTexture2D(shared,0);assert.deepEqual(take(),[]);
clone.wrapS=T.RepeatWrapping;clone.needsUpdate=true;textures.setTexture2D(clone,0);
assert.deepEqual(take(),[['allocate',2,8,4],['upload',0,8,4],['upload',1,4,2]]);
clone.dispose();assert.deepEqual(take(),[['delete']]);
shared.onUpdate=t=>{calls.push(['callback']);t.needsUpdate=true;};shared.needsUpdate=true;textures.setTexture2D(shared,0);
assert.deepEqual(take(),[['upload',0,8,4],['upload',1,4,2],['callback']]);
textures.setTexture2D(shared,0);assert.deepEqual(take(),[]);
shared.onUpdate=null;shared.mipmaps=[mip(0,4),mip(4,0)];shared.needsUpdate=true;textures.setTexture2D(shared,0);
assert.deepEqual(take(),[['upload',0,0,4],['upload',1,4,0]]);
console.log(`Pinned WebGLTextures: 16 mipmap allocation/upload/cache/callback traces passed; ${lock.commit}`);

