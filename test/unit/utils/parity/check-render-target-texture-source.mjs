// Executes pinned three.js functions with recorded GL calls. This is source
// protocol evidence; real Vulkan pixels are checked by VulkanRendererTests.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {pathToFileURL} from 'node:url';
import {execFileSync} from 'node:child_process';
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], {encoding:'utf8'}).trim(), lock.commit);
const {parse} = createRequire(path.join(upstream, 'package.json'))('acorn');
function source(file) {
    const text = fs.readFileSync(path.join(upstream, file), 'utf8');
    return {text, ast:parse(text, {ecmaVersion:'latest', sourceType:'module'})};
}
function nodes(ast, predicate, results = []) {
    if (!ast || typeof ast !== 'object') return results;
    if (predicate(ast)) results.push(ast);
    for (const item of Object.values(ast)) {
        if (Array.isArray(item)) for (const child of item) nodes(child, predicate, results);
        else if (item && typeof item === 'object') nodes(item, predicate, results);
    }
    return results;
}
function originalFunction(unit, name) {
    const matches = nodes(unit.ast, n => n.type === 'FunctionDeclaration' && n.id?.name === name);
    assert.equal(matches.length, 1, name);
    return unit.text.slice(matches[0].start, matches[0].end);
}
const stateUnit = source('src/renderers/webgl/WebGLState.js');
const emptyCalls = [], binds = [];
const gl = {TEXTURE_2D:3553, TEXTURE_3D:32879, TEXTURE_2D_ARRAY:35866, TEXTURE0:33984,
    RGBA:6408, UNSIGNED_BYTE:5121, TEXTURE_MIN_FILTER:10241, TEXTURE_MAG_FILTER:10240, NEAREST:9728,
    createTexture:()=>({empty:true}), bindTexture:(...args)=>binds.push(args), texParameteri:()=>{},
    texImage2D:(...args)=>emptyCalls.push(args), activeTexture:()=>{}};
const createEmpty = new Function('gl', originalFunction(stateUnit, 'createTexture') + '; return createTexture;')(gl);
const empty = createEmpty(gl.TEXTURE_2D, gl.TEXTURE_2D, 1);
assert.equal(emptyCalls.length, 1);
assert.deepEqual([...emptyCalls[0].at(-1)], [0,0,0,0]);
const bind = new Function('gl', 'maxTextures', 'emptyTextures',
    'let currentTextureSlot = null; const currentBoundTextures = {}; ' +
    originalFunction(stateUnit, 'bindTexture') + '; return bindTexture;')(gl, 8, {[gl.TEXTURE_2D]:empty});
const texturesUnit = source('src/renderers/webgl/WebGLTextures.js');
const propertyMap = new WeakMap(), warnings = [], uploads = [];
const properties = {get(texture) { if (!propertyMap.has(texture)) propertyMap.set(texture, {}); return propertyMap.get(texture); },
    remove(texture) { propertyMap.delete(texture); }};
const setter = new Function('properties', '_gl', 'state', 'updateVideoTexture', 'warn', 'uploadTexture',
    originalFunction(texturesUnit, 'setTexture2D') + ';return setTexture2D;')(
    properties, gl, {bindTexture:bind}, ()=>{throw Error('unexpected video');}, x=>warnings.push(x),
    (...args)=>uploads.push(args));
let imageReads = 0;
const targetTexture = {isRenderTargetTexture:true, version:0, get image(){++imageReads; throw Error('no CPU read');}};
setter(targetTexture, 0);
assert.equal(binds.at(-1)[1], empty);
const attachment = {attachment:true}; properties.get(targetTexture).__webglTexture = attachment;
for (const version of [0,1,17,NaN]) { targetTexture.version = version; setter(targetTexture, 0); }
assert.equal(binds.at(-1)[1], attachment);
assert.equal(imageReads, 0); assert.equal(uploads.length, 0); assert.equal(warnings.length, 0);
// Deletion during a later texture upload implicitly unbinds the old GL object.
// Source code makes no explicit replacement bind until a later setTexture2D.
// GLES 3.0.6 sections 3.8.1/3.9.2.1 define incomplete sampling as opaque black.
const deleted = [], memory = {memory:{textures:1}}, bindsBeforeDelete = binds.length;
gl.deleteTexture = object => deleted.push(object); gl.deleteFramebuffer = ()=>{};
const deallocate = new Function('properties','_gl','destroyTexture','info',
    originalFunction(texturesUnit, 'deallocateRenderTarget') + ';return deallocateRenderTarget;')(
    properties,gl,()=>{throw Error('unexpected depth');},memory);
const mockTarget = {depthTexture:null,textures:[targetTexture]};
deallocate(mockTarget);
assert.deepEqual(deleted,[attachment]); assert.equal(memory.memory.textures,0);
assert.equal(binds.length,bindsBeforeDelete); assert.equal(propertyMap.has(targetTexture),false);
setter(targetTexture,0); assert.equal(binds.at(-1)[1],empty);
const internalFormat = new Function('_gl',originalFunction(texturesUnit,'getInternalFormat')+';return getInternalFormat;')(
    {RGBA8:32856,SRGB8_ALPHA8:35907});
assert.equal(internalFormat('RGBA8'),32856); assert.equal(internalFormat('SRGB8_ALPHA8'),35907);
const ordinary = {isRenderTargetTexture:false, version:1, image:{complete:true}};
setter(ordinary, 1); assert.equal(uploads.length, 1);
assert.equal(uploads[0][1], ordinary); assert.equal(uploads[0][2], 1);
properties.get(ordinary).__version = 1; setter(ordinary, 1); assert.equal(uploads.length, 1);
const renderer = source('src/renderers/WebGLRenderer.js');
const colorVariables = nodes(renderer.ast, n => n.type === 'VariableDeclarator' && n.id.name === 'colorSpace' &&
    n.init?.type === 'ConditionalExpression' && renderer.text.slice(n.init.start, n.init.end).includes('_currentRenderTarget'));
assert.equal(colorVariables.length, 1);
const expression = renderer.text.slice(colorVariables[0].init.start, colorVariables[0].init.end);
const chooseColor = new Function('_currentRenderTarget','_this','ColorManagement', 'return (' + expression + ');');
const screen = {outputColorSpace:'screen-space'}, working = {workingColorSpace:'working-space'};
assert.equal(chooseColor(null,screen,working),'screen-space');
assert.equal(chooseColor({texture:{colorSpace:'attachment-space'}},screen,working),'working-space');
assert.equal(chooseColor({isXRRenderTarget:true,texture:{colorSpace:'attachment-space'}},screen,working),'attachment-space');
const {RenderTarget} = await import(pathToFileURL(path.join(upstream,'src/core/RenderTarget.js')));
const target = new RenderTarget(8,8), texture = target.texture;
assert.equal(texture.flipY,false); assert.equal(texture.generateMipmaps,false); assert.equal(texture.version,1);
let textureDisposes=0, targetDisposes=0;
texture.addEventListener('dispose',()=>++textureDisposes);
target.addEventListener('dispose',()=>++targetDisposes);
texture.dispose(); assert.equal(textureDisposes,1); assert.equal(targetDisposes,0);
target.setSize(16,4); assert.equal(targetDisposes,1); assert.equal(target.texture,texture);
assert.equal(texture.image.width,16); assert.equal(texture.image.height,4);
target.dispose(); assert.equal(targetDisposes,2); assert.equal(textureDisposes,1);
console.log('Pinned render-target source: upload bypass, zero RGBA fallback, native attachment binding, output color-space selection and target lifecycle passed');
