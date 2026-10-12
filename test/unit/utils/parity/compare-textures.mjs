import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
const upstream = path.resolve(process.argv[2]);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', upstream, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const T = await import(pathToFileURL(path.join(upstream, 'src/Three.Core.js')));
const number = x => Number.isNaN(x) ? 'NaN' : x === Infinity ? 'Infinity' : x === -Infinity ? '-Infinity' : Object.is(x, -0) ? '-0' : x;
console.warn = () => {};
const state = texture => {
    texture.uuid = 'texture'; texture.source.uuid = 'image';
    return { json: texture.toJSON(), source: texture.source.toJSON(), size: [texture.width, texture.height, texture.depth].map(number),
        version: texture.version, sourceVersion: texture.source.version, pmremVersion: texture.pmremVersion,
        isArrayTexture: texture.isArrayTexture, dataReady: texture.source.dataReady };
};
const expected = {};
expected.base = state(new T.Texture());
const data = new T.DataTexture(new Uint8Array([1, 2, 3, 255, 5, 6, 7, 128]), 2, 1);
data.name = 'pixels'; data.needsUpdate = 1; data.needsUpdate = true;
data.needsPMREMUpdate = true; data.needsPMREMUpdate = 'true'; data.source.dataReady = false;
data.offset.set(0.2, -0.4); data.repeat.set(2, 3); data.center.set(0.5, 0.7); data.rotation = 0.6; data.updateMatrix();
data.userData.value = [1, null, 'a']; data.addUpdateRange(-0.5, 7.2);
const clone = data.clone();
expected.data = state(data); expected.clone = state(clone);
expected.cloneIdentity = [clone.source === data.source, clone.image === data.image, clone.mipmaps !== data.mipmaps,
    clone.updateRanges.length === 0, clone.userData === data.userData];
const array = new T.DataArrayTexture(new Uint16Array([1, 65535, 2, 9]), 2, 1, 2);
expected.array = state(array);
expected.volume = state(new T.Data3DTexture(new Float32Array([0.25, 0.5, 0.75, 1]), 1, 1, 4));
const depth = new T.DepthTexture(8, 4, T.UnsignedIntType, T.UVMapping, T.ClampToEdgeWrapping, T.ClampToEdgeWrapping,
    T.NearestFilter, T.NearestFilter, 1, T.DepthFormat, 3);
depth.compareFunction = T.LessEqualCompare;
const depthClone = depth.clone(); expected.depth = state(depth); expected.depthClone = state(depthClone);
const framebuffer = new T.FramebufferTexture(10, 20), framebufferClone = framebuffer.clone();
expected.framebuffer = state(framebuffer); expected.framebufferClone = state(framebufferClone);
const cubeDepth = new T.CubeDepthTexture(16), cubeDepthClone = cubeDepth.clone();
expected.cubeDepth = state(cubeDepth); expected.cubeDepthClone = state(cubeDepthClone);
expected.cubeDepthIdentity = [cubeDepth.image[0] === cubeDepth.image[5], Array.isArray(cubeDepthClone.image), cubeDepthClone.image[0] === cubeDepth.image[0]];
const face = { data: new Uint8Array([1, 2, 3, 4]), width: 1, height: 1 };
expected.cube = state(new T.CubeTexture(Array(6).fill(face)));
expected.compressed = state(new T.CompressedTexture([face], 1, 1, T.RGBA_S3TC_DXT5_Format));
expected.compressedArray = state(new T.CompressedArrayTexture([face], 1, 1, 6, T.RGBA_S3TC_DXT5_Format));
const compressedCube = new T.CompressedCubeTexture(Array(6).fill(face), T.RGBA_S3TC_DXT5_Format);
expected.compressedCube = state(compressedCube);
try { compressedCube.clone(); expected.cubeCloneError = 'none'; } catch (error) { expected.cubeCloneError = error.name; }
for (const layer of [2, -0, NaN, 2, 0, NaN]) array.addLayerUpdate(layer);
expected.layerUpdates = [...array.layerUpdates].map(number);
{
    const volume = new T.Data3DTexture(), compressed = new T.CompressedArrayTexture();
    for (const texture of [array, volume, compressed]) texture.setValues({wrapR:T.MirroredRepeatWrapping});
    for (const texture of [depth, cubeDepth]) texture.setValues({compareFunction:T.GreaterEqualCompare});
    expected.setValuesDerived = [array.wrapR, volume.wrapR, compressed.wrapR, depth.compareFunction, cubeDepth.compareFunction];
    depth.setValues({compareFunction:null}); expected.setValuesNullCompare = depth.compareFunction === null;
    const texture = new T.Texture(); texture.setValues({version:7.5, pmremVersion:-0.5});
    expected.setValuesVersions = [texture.version, texture.pmremVersion, texture.source.version];
    texture.needsUpdate = true; texture.needsPMREMUpdate = true;
    expected.incrementVersions = [texture.version, texture.pmremVersion, texture.source.version];
    expected.primitiveUserData = [3, true, '', 'abc'].map(value => {
        texture.userData = value; return texture.toJSON().userData ?? null;
    });
}
{
    const map = new T.DataTexture(new Uint8Array([200, 80, 40, 255]), 1, 1);
    map.uuid = 'map'; map.source.uuid = 'shared-image';
    const alpha = map.clone(); alpha.uuid = 'alpha';
    const material = new T.MeshBasicMaterial(); material.uuid = 'material';
    material.map = map; material.alphaMap = alpha; material.lightMap = map; material.lightMapIntensity = 2;
    material.aoMap = alpha; material.aoMapIntensity = 0.25; material.specularMap = map; material.envMap = alpha; material.combine = T.AddOperation;
    const geometry = new T.BufferGeometry(), scene = new T.Scene();
    scene.add(new T.Mesh(geometry, material), new T.Mesh(geometry, material));
    const document = scene.toJSON(), standalone = material.toJSON();
    expected.materialLinks = Object.fromEntries(['map', 'alphaMap', 'lightMap', 'lightMapIntensity', 'aoMap', 'aoMapIntensity', 'specularMap', 'envMap', 'combine']
        .map(field => [field, document.materials[0][field]]));
    expected.sceneTextures = document.textures; expected.sceneImages = document.images;
    expected.standaloneTextures = standalone.textures; expected.standaloneImages = standalone.images;
    expected.sceneMaterialCount = document.materials.length;
}
expected.uv = [];
for (const mode of [T.UVMapping, T.CubeReflectionMapping])
for (const s of [T.RepeatWrapping, T.ClampToEdgeWrapping, T.MirroredRepeatWrapping])
for (const t of [T.RepeatWrapping, T.ClampToEdgeWrapping, T.MirroredRepeatWrapping])
for (const flip of [false, true]) {
    data.mapping = mode; data.wrapS = s; data.wrapT = t; data.flipY = flip;
    for (let i = -17; i <= 17; ++i) {
        const uv = new T.Vector2(i / 5, (i * i % 11 - 5) / 3);
        data.transformUv(uv); expected.uv.push(number(uv.x), number(uv.y));
    }
}
{
    const texture = new T.Texture();
    texture.userData = {userData: {rotation: 4}, rotation: 2}; texture.setValues(texture.userData);
    expected.aliasedSetValues = [texture.rotation, texture.userData.rotation];
}
expected.reentrantSourcePixels = [0, 1, 2].map(replacement => {
    const image = {data: [0, 2, 3], width: 1, height: 1};
    Object.defineProperty(image.data, '0', {get() {
        image.data = replacement === 0 ? [7, 8] : replacement === 1 ? new Uint8Array([7, 8]) : null;
        image.width = 4; image.height = 5; return 1;
    }});
    try { return new T.TextureSource(image).toJSON().url; } catch (error) { return error.name; }
});
{
    const texture = new T.Texture(); texture.uuid = 'before'; texture.name = 'original';
    class ReplacingSource extends T.TextureSource {
        toJSON(meta) {
            texture.uuid = 'after'; texture.name = 'changed'; texture.mapping = T.CubeReflectionMapping;
            texture.source = new T.TextureSource(); return super.toJSON(meta);
        }
    }
    texture.source = new ReplacingSource(); texture.source.uuid = 'original-source';
    const context = {textures: {}, images: {}}, output = texture.toJSON(context);
    expected.reentrantSourceCache = [output.uuid, output.name, output.image, output.mapping,
        'after' in context.textures, 'before' in context.textures];
    output.name = 'cached'; expected.reentrantCachedName = texture.toJSON(context).name;
}
{
    const base = new T.Material(); base.uuid = 'base'; expected.baseMaterialJSON = base.toJSON();
    const basic = new T.MeshBasicMaterial(); basic.uuid = 'basic'; expected.basicMaterialJSON = basic.toJSON();
}
{
    class CallbackTexture extends T.Texture {
        toJSON(meta) { if (this.callback) this.callback(); return super.toJSON(meta); }
    }
    const material = new T.MeshBasicMaterial(); material.uuid = 'ordered-material';
    material.name = 'before'; material.opacity = .75; material.userData = {stage:'before'};
    material.clippingPlanes = [new T.Plane(new T.Vector3(1,0,0),1)];
    const calls = [], image = new T.TextureSource(); image.uuid = 'ordered-image';
    const make = name => {
        const texture = new CallbackTexture(); texture.uuid = name; texture.name = name; texture.source = image;
        texture.callback = () => calls.push(name); return texture;
    };
    const map=make('map'), alpha=make('alpha'), light=make('light'), ao=make('ao'), specular=make('specular'), unused=make('unused'), environment=make('environment');
    Object.assign(material,{map,lightMap:light,aoMap:ao,specularMap:specular,envMap:unused});
    map.callback = () => {
        calls.push('map'); material.name='after'; material.opacity=.25; material.color.setRGB(1,0,0);
        material.alphaMap=alpha; material.userData={stage:'map'};
    };
    light.callback = () => { calls.push('light'); material.lightMapIntensity=2; };
    ao.callback = () => { calls.push('ao'); material.aoMapIntensity=3; };
    specular.callback = () => { calls.push('specular'); material.envMap=environment; };
    environment.callback = () => {
        calls.push('environment'); material.envMap=null; material.combine=T.MixOperation;
        material.envMapRotation.set(.1,.2,.3); material.reflectivity=.4; material.refractionRatio=.7;
        material.clippingPlanes=[new T.Plane(new T.Vector3(0,1,0),4)];
        material.wireframe=true; material.wireframeLinewidth=2; material.wireframeLinecap='square'; material.wireframeLinejoin='bevel'; material.fog=false;
        material.userData={stage:'environment'};
    };
    const output=material.toJSON();
    expected.materialSerializationOrder=JSON.parse(JSON.stringify(output));
    expected.materialSerializationCalls=calls;
    material.userData.later=9; expected.materialUserDataAlias=output.userData.later;
}
{
    expected.materialPrimitiveUserData=[3,true,'','abc',null].map(userData=>{
        const material=new T.Material(); material.userData=userData;
        try {return material.toJSON().userData ?? null;} catch(error) {return error.name;}
    });
    const material=new T.MeshBasicMaterial(); material.clippingPlanes=[null]; material.userData=null;
    class ClearingTexture extends T.Texture {
        toJSON(meta) { material.clippingPlanes=null; material.userData={}; return super.toJSON(meta); }
    }
    material.map=new ClearingTexture(); const output=material.toJSON();
    expected.materialCallbackClearsInvalidLateFields=!('clippingPlanes' in output) && !('userData' in output);
}
const actual = JSON.parse(execFileSync(process.argv[3], [], { encoding: 'utf8' }));
let comparisons = 0;
function compare(a, b, key = '') {
    if (typeof b === 'number' && Number.isFinite(b) && typeof a === 'number') {
        assert.ok(Math.abs(a - b) <= 2e-13 * Math.max(1, Math.abs(b)), `${key}: ${a} != ${b}`); comparisons++;
    } else if (b !== null && typeof b === 'object') {
        assert.deepEqual(Object.keys(a), Object.keys(b), key);
        for (const k of Object.keys(b)) compare(a[k], b[k], `${key}.${k}`);
    } else { assert.equal(a, b, key); comparisons++; }
}
compare(actual, JSON.parse(JSON.stringify(expected)));
console.log(`Texture parity: ${Object.keys(expected).length} scenarios / ${comparisons} comparisons passed`);
