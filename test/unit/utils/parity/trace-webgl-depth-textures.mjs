// Pinned source defaults, attachment identity, parameter protocol and analytic
// goldens. VulkanManagedDepthTextures.tests.cpp supplies independent GPU evidence.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { createRequire } from 'node:module';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';

const pinned = path.resolve(process.argv[2] ?? 'D:/Temp/surui/threejs-pinned-11738a-20261104');
const output = path.resolve(process.argv[3] ?? `D:/Temp/surui/webgl-depth-textures-${Date.now()}`);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', pinned, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const load = file => import(pathToFileURL(path.join(pinned, 'src', file)).href);
const imports = Object.assign({}, ...await Promise.all([
  'textures/DepthTexture.js', 'textures/CubeDepthTexture.js', 'renderers/WebGLRenderTarget.js',
  'renderers/WebGLCubeRenderTarget.js', 'lights/PointLight.js', 'cameras/OrthographicCamera.js', 'constants.js'
].map(load)));
const { DepthTexture, CubeDepthTexture, WebGLRenderTarget, WebGLCubeRenderTarget, PointLight, OrthographicCamera, UnsignedShortType, UnsignedIntType, UnsignedInt248Type,
  FloatType, DepthFormat, DepthStencilFormat, NearestFilter, ClampToEdgeWrapping, LessEqualCompare, GreaterCompare,
  NeverCompare, LessCompare, EqualCompare, NotEqualCompare, GreaterEqualCompare, AlwaysCompare } = imports;
let assertions = 0;
const same = (a, b) => { assertions++; assert.strictEqual(a, b); };
const equal = (a, b) => { assertions++; assert.deepStrictEqual(a, b); };
const records = [];
{
  const depth = new DepthTexture(32, 16), source = depth.source, image = depth.image;
  equal([depth.isDepthTexture, depth.type, depth.format, depth.compareFunction], [true, UnsignedIntType, DepthFormat, null]);
  equal([depth.magFilter, depth.minFilter, depth.wrapS, depth.wrapT, depth.flipY, depth.generateMipmaps],
    [NearestFilter, NearestFilter, ClampToEdgeWrapping, ClampToEdgeWrapping, false, false]);
  equal([image.width, image.height, image.depth], [32, 16, 1]);
  for (const type of [UnsignedShortType, UnsignedIntType, FloatType]) {
    const typed = new DepthTexture(32, 16, type); same(typed.type, type); same(typed.format, DepthFormat);
  }
  const target = new WebGLRenderTarget(32, 16, { depthTexture: depth });
  same(target.depthTexture, depth); same(depth.renderTarget, target); same(target.depthBuffer, true);
  depth.compareFunction = LessEqualCompare;
  same(depth.source, source); same(depth.image, image); same(target.depthTexture, depth);
  const clone = depth.clone(); same(clone.compareFunction, LessEqualCompare);
  same(clone.source === source, false); same(clone.image === image, false);
  target.depthTexture = null; same(depth.renderTarget, null);
  records.push({ name: 'canonical-depth-identity', defaultType: UnsignedIntType,
    testedTypes: [UnsignedShortType, UnsignedIntType, FloatType], compareDefault: null,
    targetRetainsOriginalTextureSourceImage: true, cloneUsesIndependentSourceImage: true });
}

const sourceFile = 'renderers/webgl/WebGLTextures.js';
const text = fs.readFileSync(path.join(pinned, 'src', sourceFile), 'utf8');
const { parse } = createRequire(path.join(pinned, 'package.json'))('acorn');
const body = parse(text, { sourceType: 'module', ecmaVersion: 'latest' }).body
  .find(node => node.type === 'FunctionDeclaration' && node.id.name === 'WebGLTextures').body.body;
const declaration = name => body.find(node => node.type === 'FunctionDeclaration' && node.id.name === name);
const extract = name => { const node = declaration(name); assert.ok(node); return text.slice(node.start, node.end); };
{
  const gl = { FRAMEBUFFER: 0x8d40, TEXTURE_2D: 0x0de1, TEXTURE_CUBE_MAP_POSITIVE_X: 0x8515,
    DEPTH_ATTACHMENT: 0x8d00, DEPTH_STENCIL_ATTACHMENT: 0x821a,
    DEPTH_COMPONENT16: 0x81a5, DEPTH_COMPONENT24: 0x81a6, DEPTH_COMPONENT32F: 0x8cac,
    DEPTH24_STENCIL8: 0x88f0, DEPTH32F_STENCIL8: 0x8cad };
  const format = new Function('_gl', 'UnsignedShortType', 'UnsignedIntType', 'UnsignedInt248Type', 'FloatType',
    `${extract('getInternalDepthFormat')};return getInternalDepthFormat;`)(gl, UnsignedShortType, UnsignedIntType, UnsignedInt248Type, FloatType);
  equal([UnsignedShortType, UnsignedIntType, FloatType].map(type => format(false, type)), [0x81a5, 0x81a6, 0x8cac]);
  const depth = new DepthTexture(1, 1, UnsignedShortType), target = new WebGLRenderTarget(8, 4, { depthTexture: depth });
  const image = depth.image, source = depth.source, beforeVersion = depth.version, handle = {}, framebuffer = {}, events = [];
  const data = new WeakMap(), properties = { get(object) { if (!data.has(object)) data.set(object, {}); return data.get(object); } };
  const state = { bindFramebuffer(...args) { events.push(['bind', ...args]); } };
  const setTexture2D = (texture, unit) => { events.push(['allocate-depth-boundary', texture, unit]); properties.get(texture).__webglTexture = handle; };
  gl.framebufferTexture2D = (...args) => events.push(['attach', ...args]);
  const setup = new Function('_gl', 'properties', 'state', 'setTexture2D', 'setTextureCube', 'getRenderTargetSamples', 'useMultisampledRTT', 'DepthFormat', 'DepthStencilFormat',
    `${extract('setupDepthTexture')};return setupDepthTexture;`)(gl, properties, state, setTexture2D,
    () => assert.fail('2D depth fixture must not use cube upload'), target => target.samples, () => false, DepthFormat, DepthStencilFormat);
  setup(framebuffer, target, 0);
  same(depth.image, image); same(depth.source, source); same(target.depthTexture, depth);
  equal([image.width, image.height], [8, 4]); same(depth.version, beforeVersion + 1);
  equal(events.map(event => event[0]), ['bind', 'allocate-depth-boundary', 'attach']);
  equal(events[2], ['attach', gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.TEXTURE_2D, handle, 0]);
  records.push({ name: 'source-depth-format-and-attachment-protocol', depthFormats: [0x81a5, 0x81a6, 0x8cac],
    order: events.map(event => event[0]), attachment: gl.DEPTH_ATTACHMENT, originalTextureSourceImageRetained: true,
    resizedImage: [image.width, image.height], allocationVersionIncrement: 1 });
}
{
  const commands = [];
  const gl = { TEXTURE_2D: 0x0de1, TEXTURE_3D: 0x806f, TEXTURE_2D_ARRAY: 0x8c1a,
    TEXTURE_WRAP_S: 0x2802, TEXTURE_WRAP_T: 0x2803, TEXTURE_WRAP_R: 0x8072,
    TEXTURE_MAG_FILTER: 0x2800, TEXTURE_MIN_FILTER: 0x2801,
    TEXTURE_COMPARE_MODE: 0x884c, COMPARE_REF_TO_TEXTURE: 0x884e, TEXTURE_COMPARE_FUNC: 0x884d,
    texParameteri(...args) { commands.push(args); } };
  const compareToGL = Object.fromEntries([
    [NeverCompare, 0x0200], [LessCompare, 0x0201], [EqualCompare, 0x0202], [LessEqualCompare, 0x0203],
    [GreaterCompare, 0x0204], [NotEqualCompare, 0x0205], [GreaterEqualCompare, 0x0206], [AlwaysCompare, 0x0207]
  ]);
  const params = new Function('_gl', 'extensions', 'wrappingToGL', 'filterToGL', 'compareToGL', 'FloatType',
    `${extract('setTextureParameters')};return setTextureParameters;`)(gl, { has: () => false },
    { [ClampToEdgeWrapping]: 0x812f }, { [NearestFilter]: 0x2600 }, compareToGL, FloatType);
  const texture = new DepthTexture(2, 1, UnsignedShortType);
  params(gl.TEXTURE_2D, texture); equal(commands.filter(command => command[1] === gl.TEXTURE_COMPARE_MODE), []);
  texture.compareFunction = LessEqualCompare; commands.length = 0; params(gl.TEXTURE_2D, texture);
  equal(commands.slice(-2), [[gl.TEXTURE_2D, gl.TEXTURE_COMPARE_MODE, gl.COMPARE_REF_TO_TEXTURE], [gl.TEXTURE_2D, gl.TEXTURE_COMPARE_FUNC, 0x0203]]);
  texture.compareFunction = GreaterCompare; commands.length = 0; params(gl.TEXTURE_2D, texture);
  equal(commands.slice(-2), [[gl.TEXTURE_2D, gl.TEXTURE_COMPARE_MODE, gl.COMPARE_REF_TO_TEXTURE], [gl.TEXTURE_2D, gl.TEXTURE_COMPARE_FUNC, 0x0204]]);
  const key = new Function(`${extract('getTextureCacheKey')};return getTextureCacheKey;`)();
  const before = key(texture); texture.compareFunction = LessEqualCompare; same(key(texture), before);
  records.push({ name: 'source-comparison-parameter-protocol', lastNativeCommands: commands.slice(-2),
    compareFunctionDoesNotReplaceNativeTextureCacheKey: true });
}
{
  const stored = .25, refs = [.1, .9];
  const comparisons = {
    NEVER: () => false, LESS: (ref, value) => ref < value, EQUAL: (ref, value) => ref === value,
    LEQUAL: (ref, value) => ref <= value, GREATER: (ref, value) => ref > value,
    NOTEQUAL: (ref, value) => ref !== value, GEQUAL: (ref, value) => ref >= value, ALWAYS: () => true
  };
  const goldens = Object.fromEntries(Object.entries(comparisons).map(([name, compare]) => [name, refs.map(ref => compare(ref, stored) ? 255 : 0)]));
  equal(goldens.LEQUAL, [255, 0]); equal(goldens.GREATER, [0, 255]);
  const uploaded = [0x4000 / 65535, 0xc000 / 65535];
  equal(uploaded.map(value => .5 <= value ? 255 : 0), [0, 255]);
  equal(uploaded.map(value => Math.round(255 * value)), [64, 191]);
  const uploadedUint32 = 0x40000000 / 0xffffffff, packedD24 = Math.round(uploadedUint32 * 0xffffff);
  same(packedD24, 0x400000); same(Math.round(255 * packedD24 / 0xffffff), 64);
  equal([.1, .9].map(ref => ref <= uploadedUint32 ? 255 : 0), [255, 0]);
  equal([.1, .9].map(ref => ref <= Math.fround(.25) ? 255 : 0), [255, 0]);
  const packedD16 = [0x40000000, 0xc0000000].map(value => Math.round(value / 0xffffffff * 0xffff));
  equal(packedD16, [0x4000, 0xbfff]);
  equal(packedD16.map(value => Math.round(255 * value / 65535)), [64, 191]);
  const clamped = [-.5, 1.5].map(value => Math.min(1, Math.max(0, value)));
  equal(clamped, [0, 1]); equal(clamped.map(value => Math.round(255 * (.25 + .5 * value))), [64, 191]);
  const camera = new OrthographicCamera(-1, 1, 1, -1, .1, 10); camera.position.z = 3;
  const meshDepth = (camera.position.z - .425 - camera.near) / (camera.far - camera.near);
  same(Math.abs(meshDepth - .25) < 1e-12, true);
  records.push({ name: 'analytic-GPU-goldens', clearDepth: stored, references: refs, comparisons: goldens,
    nativeD16Input: [0x4000, 0xc000], nativeD16ShadowRefHalf: [0, 255], nativeD16OrdinaryBytes: [64, 191],
    nativeD24Uint32Input: 0x40000000, nativeD24Low24Packing: packedD24, nativeD24OrdinaryByte: 64,
    nativeD32FInput: .25, nativeD24AndD32FShadowRefsPointOnePointNine: [255, 0],
    nativeD16Uint32Input: [0x40000000, 0xc0000000], nativeD16Uint32Packed: packedD16,
    nativeD16Unpack: { rowLength: 4, skipRows: 1, skipPixels: 1, sourceBytes: 4, storageBytes: 2 },
    nativeD32FUnclampedInput: [-.5, 1.5], nativeD32FClampedStored: clamped,
    nativeD32FScaledOrdinaryBytes: [64, 191],
    meshPlaneZ: .425, meshDepth, meshLeftAndClearRightShadowRefHalf: [0, 255],
    sameFrameLEQUALThenGREATERAtRefPointOne: [255, 0], invalidSamplerOrFeedbackError: 0x0502 });
}
{
  const sample = pixel => 2 * ((pixel + .5) / 64) - 1;
  const shadowX = sample(28), directX = sample(48), y = -sample(32);
  const casterCenter = -.6, casterHalfWidth = .15;
  const covered = x => x >= casterCenter - casterHalfWidth && x <= casterCenter + casterHalfWidth;
  // Receiver z=0, caster z=1, directional light vector=(-1,0,2),
  // spot position=(-1,0,2). Test rays meet the actual planar caster.
  same(covered(shadowX - .5), true); same(covered(directX - .5), false);
  same(covered((shadowX - 1) / 2), true); same(covered((directX - 1) / 2), false);
  const lambert = (x, spot) => Math.round(255 * (spot ? 2 / Math.sqrt(4 + (x + 1) ** 2 + y ** 2) : 2 / Math.sqrt(5)));
  records.push({ name: 'default-PCF-scene-analytic-goldens', lightPosition: [-1, 0, 2],
    receiverZ: 0, casterPosition: [-.6, 0, 1], casterSize: [.3, 1.2],
    shadowPixel: [28, 32], directPixel: [48, 32], initialShadowByte: 0,
    directionalDirectByte: lambert(directX, false), spotDirectByte: lambert(directX, true),
    frozenMapShadowByte: 0, movedCasterX: .6,
    directionalRefreshedByte: lambert(shadowX, false), spotRefreshedByte: lambert(shadowX, true),
    preservesSourceCameraMaterialTargetDepthTextureIdentities: true });
}
{
  const depth = new CubeDepthTexture(16), target = new WebGLCubeRenderTarget(16);
  target.depthTexture = depth; depth.compareFunction = LessEqualCompare;
  same(target.isWebGLCubeRenderTarget, true); same(depth.isCubeDepthTexture, true);
  same(depth.isDepthTexture, true); same(depth.isCubeTexture, true);
  same(depth.renderTarget, target); same(depth.images, depth.image); same(depth.image.length, 6);
  for (const image of depth.image) { same(image, depth.image[0]); equal([image.width, image.height], [16, 16]); }
  const depths = [.1, .2, .3, .6, .7, .8], directions = [[1,0,0], [-1,0,0], [0,1,0], [0,-1,0], [0,0,1], [0,0,-1]];
  equal(depths.map(value => .5 <= value ? 255 : 0), [0, 0, 0, 255, 255, 255]);
  const point = new PointLight(0xffffff, Math.PI); same(point.shadow.isPointLightShadow, true);
  for(let face=0;face<6;face++)same(point.shadow.getCamera(face),point.shadow.camera);
  records.push({ name: 'cube-depth-face-and-default-point-goldens', directions, clearedDepths: depths,
    shadowRefHalf: [0, 0, 0, 255, 255, 255], ordinaryBytes: depths.map(value => Math.round(255 * value)),
    pointUsesOriginalCameraForAllSixFaces: true, pointFaceUpVectors: [[0,-1,0], [0,-1,0], [0,0,1], [0,0,-1], [0,-1,0], [0,-1,0]],
    pointShadowCallbackCounts: { firstUpdate: 6, frozen: 6, forcedUpdate: 12 },
    pointSceneGoldenUsesSamePerspectiveRayAndLambertValuesAsSpot: true });
}
const files = ['textures/DepthTexture.js', 'textures/CubeDepthTexture.js', 'core/RenderTarget.js', sourceFile,
  'renderers/WebGLCubeRenderTarget.js', 'renderers/webgl/WebGLShadowMap.js', 'lights/PointLightShadow.js', 'cameras/OrthographicCamera.js'];
const hashes = Object.fromEntries(files.map(file => [file, crypto.createHash('sha256').update(fs.readFileSync(path.join(pinned, 'src', file))).digest('hex')]));
fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, 'trace.json'), JSON.stringify({ pinned, commit: lock.commit, hashes, assertions,
  evidence: 'pinned CPU defaults/protocol and analytic goldens only; no GPU execution', records }, null, 2));
console.log(JSON.stringify({ assertions, scenarios: records.length, output, evidence: 'pinned CPU only; GPU test is separate' }));
