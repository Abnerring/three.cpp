// Actual pinned target lifecycle and renderer callback protocol. GPU commands
// and framebuffer allocation are explicit recording boundaries, not rasterization.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import { createRequire } from 'node:module';
import { execFileSync } from 'node:child_process';
import { pathToFileURL } from 'node:url';
const pinned = path.resolve(process.argv[2] ?? 'D:/Temp/surui/threejs-pinned-11738a-20261104');
const output = path.resolve(process.argv[3] ?? `D:/Temp/surui/webgl-render-targets-${Date.now()}`);
const lock = JSON.parse(fs.readFileSync(new URL('../../../../upstream.json', import.meta.url)));
assert.equal(execFileSync('git', ['-C', pinned, 'rev-parse', 'HEAD'], { encoding: 'utf8' }).trim(), lock.commit);
const load = file => import(pathToFileURL(path.join(pinned, 'src', file)).href);
const imports = Object.assign({}, ...await Promise.all([
  'renderers/WebGLRenderTarget.js', 'renderers/webgl/WebGLProperties.js', 'math/Vector4.js',
  'scenes/Scene.js', 'objects/Mesh.js', 'cameras/OrthographicCamera.js', 'geometries/PlaneGeometry.js',
  'materials/MeshBasicMaterial.js', 'constants.js'
].map(load)));
const { WebGLRenderTarget, WebGLProperties, Vector4, Scene, Mesh, OrthographicCamera,
  PlaneGeometry, MeshBasicMaterial, RGBAFormat, UnsignedByteType, LinearFilter, ClampToEdgeWrapping,
  NoColorSpace } = imports;
let assertions = 0;
const same = (a, b) => { assertions++; assert.strictEqual(a, b); };
const equal = (a, b) => { assertions++; assert.deepStrictEqual(a, b); };
const records = [];
{
  const target = new WebGLRenderTarget(32, 16), texture = target.texture;
  same(target.isRenderTarget, true); same(target.isWebGLRenderTarget, true);
  equal([target.width, target.height, target.depth, target.samples, target.textures.length], [32, 16, 1, 0, 1]);
  equal([target.depthBuffer, target.stencilBuffer, target.depthTexture, target.scissorTest], [true, false, null, false]);
  equal(target.viewport.toArray(), [0, 0, 32, 16]); equal(target.scissor.toArray(), [0, 0, 32, 16]);
  same(target.textures[0], texture); same(texture.renderTarget, target); same(texture.isRenderTargetTexture, true);
  equal([texture.format, texture.type, texture.minFilter, texture.magFilter], [RGBAFormat, UnsignedByteType, LinearFilter, LinearFilter]);
  equal([texture.wrapS, texture.wrapT, texture.colorSpace, texture.generateMipmaps, texture.flipY],
    [ClampToEdgeWrapping, ClampToEdgeWrapping, NoColorSpace, false, false]);
  equal([texture.image.width, texture.image.height, texture.image.depth], [32, 16, 1]);
  records.push({ name: 'canonical-defaults', size: [32, 16, 1], depthBuffer: true, rgba8: true, textureFlipY: false });
}
{
  const target = new WebGLRenderTarget(32, 16), texture = target.texture, source = texture.source, image = texture.image;
  const viewport = target.viewport, scissor = target.scissor, version = texture.version, events = [];
  target.addEventListener('dispose', function (event) {
    same(this, target); same(event.target, target);
    events.push({ size: [target.width, target.height, target.depth], image: [image.width, image.height, image.depth],
      viewport: viewport.toArray(), scissor: scissor.toArray() });
  });
  target.setSize(16, 32);
  equal(events, [{ size: [16, 32, 1], image: [16, 32, 1], viewport: [0, 0, 32, 16], scissor: [0, 0, 32, 16] }]);
  same(target.texture, texture); same(texture.source, source); same(texture.image, image); same(texture.version, version);
  same(target.viewport, viewport); same(target.scissor, scissor); equal(viewport.toArray(), [0, 0, 16, 32]);
  viewport.set(3, 4, 5, 6); scissor.set(7, 8, 9, 10); target.setSize(16, 32);
  same(events.length, 1); equal(viewport.toArray(), [0, 0, 16, 32]); equal(scissor.toArray(), [0, 0, 16, 32]);
  target.dispose(); same(events.length, 2); same(target.texture, texture); same(texture.source, source);
  records.push({ name: 'resize-dispose-order', events, textureSourceImageIdentityPreserved: true, textureVersionUnchanged: true });
}
{
  const target = new WebGLRenderTarget(8, 4, { count: 2, depthBuffer: false }), cloned = target.clone();
  same(cloned instanceof WebGLRenderTarget, true); same(cloned.depthBuffer, false); same(cloned.textures.length, 2);
  for (let i = 0; i < 2; i++) {
    same(cloned.textures[i] === target.textures[i], false); same(cloned.textures[i].source === target.textures[i].source, false);
    same(cloned.textures[i].image === target.textures[i].image, false); same(cloned.textures[i].renderTarget, cloned);
  }
  cloned.setSize(4, 8); equal([target.width, target.height, target.texture.image.width], [8, 4, 8]);
  records.push({ name: 'clone-attachments', independentTexturesSourcesImages: 2, sourceSize: [8, 4] });
}
const text = fs.readFileSync(path.join(pinned, 'src/renderers/WebGLRenderer.js'), 'utf8');
const { parse } = createRequire(path.join(pinned, 'package.json'))('acorn');
const body = parse(text, { sourceType: 'module', ecmaVersion: 'latest' }).body
  .find(n => n.type === 'ClassDeclaration' && n.id.name === 'WebGLRenderer').body.body.find(n => n.kind === 'constructor').value.body.body;
const assigned = name => body.find(n => n.type === 'ExpressionStatement' && n.expression.type === 'AssignmentExpression'
  && n.expression.left.object?.type === 'ThisExpression' && n.expression.left.property.name === name).expression.right;
const renderObject = body.find(n => n.type === 'FunctionDeclaration' && n.id.name === 'renderObject');
const methodNames = ['setRenderTarget', 'getRenderTarget', 'getActiveCubeFace', 'getActiveMipmapLevel'];
const factory = new Function('services', 'imports', `"use strict";
const {Vector4,DoubleSide,BackSide,FrontSide}=imports;
const {_this,_gl,properties,textures,state}=services;
const _nodesHandler=null;
let _currentRenderTarget=null,_currentActiveCubeFace=0,_currentActiveMipmapLevel=0,_currentMaterialId=-1;
const _viewport=new Vector4(0,0,64,64),_scissor=new Vector4(0,0,64,64),_pixelRatio=1,_scissorTest=false;
const _currentViewport=new Vector4(),_currentScissor=new Vector4(),_scratchFramebuffer={tag:'scratch'};
let _currentScissorTest=false;
${methodNames.map(name => `_this.${name}=${text.slice(assigned(name).start, assigned(name).end)};`).join('\n')}
${text.slice(renderObject.start, renderObject.end)}
return {renderObject};`);
function harness() {
  const events = [], draws = [], renderer = {}, properties = new WebGLProperties(); let bound = null, serial = 0;
  const name = object => object?.tag ?? null;
  const state = {
    bindFramebuffer(target, framebuffer) { events.push(['bind', target, name(framebuffer)]); const changed = bound !== framebuffer; bound = framebuffer; return changed; },
    drawBuffers(target, framebuffer) { events.push(['drawBuffers', name(target), name(framebuffer)]); },
    viewport(value) { events.push(['viewport', ...value.toArray()]); }, scissor(value) { events.push(['scissor', ...value.toArray()]); },
    setScissorTest(value) { events.push(['scissorTest', value]); }
  };
  const textures = {
    setupRenderTarget(target) {
      events.push(['allocate-boundary', name(target)]);
      properties.get(target).__webglFramebuffer = { tag: `fbo-${++serial}` };
      properties.get(target).__boundDepthTexture = target.depthTexture;
    },
    useMultisampledRTT() { return false; }
  };
  const pipeline = factory({ _this: renderer, _gl: { FRAMEBUFFER: 0x8d40 }, properties, textures, state }, imports);
  renderer.renderBufferDirect = (camera, scene, geometry, material, object, group) => {
    draws.push({ target: name(renderer.getRenderTarget()), object: name(object), color: material.color.getHex(), group });
  };
  return { renderer, pipeline, events, draws, properties };
}
{
  const h = harness(), a = new WebGLRenderTarget(32, 16); a.tag = 'a';
  a.viewport.set(2, 3, 16, 8); a.scissor.set(4, 5, 8, 4); a.scissorTest = true;
  h.renderer.setRenderTarget(a);
  equal(h.events.map(e => e[0]), ['allocate-boundary', 'bind', 'drawBuffers', 'viewport', 'scissor', 'scissorTest']);
  equal(h.events.slice(-3), [['viewport', 2, 3, 16, 8], ['scissor', 4, 5, 8, 4], ['scissorTest', true]]);
  same(h.renderer.getRenderTarget(), a); same(h.renderer.getActiveCubeFace(), 0); same(h.renderer.getActiveMipmapLevel(), 0);
  records.push({ name: 'target-switch-protocol', events: h.events });
}
{
  const h = harness(), a = new WebGLRenderTarget(), b = new WebGLRenderTarget(), c = new WebGLRenderTarget();
  a.tag = 'a'; b.tag = 'b'; c.tag = 'c'; const scene = new Scene(), camera = new OrthographicCamera(-1, 1, 1, -1, .1, 10);
  camera.position.z = 3; camera.updateMatrixWorld();
  const outer = new Mesh(new PlaneGeometry(), new MeshBasicMaterial({ color: 0xff0000 })); outer.tag = 'outer';
  const inner = new Mesh(new PlaneGeometry(), new MeshBasicMaterial({ color: 0x0000ff })); inner.tag = 'inner';
  const after = new Mesh(new PlaneGeometry(), new MeshBasicMaterial({ color: 0x00ff00 })); after.tag = 'after';
  const original = [h.renderer, scene, camera, outer.geometry, outer.material, null], callbacks = [];
  const draw = mesh => h.pipeline.renderObject(mesh, scene, camera, mesh.geometry, mesh.material, null);
  outer.onBeforeRender = function (...args) {
    same(this, outer); equal(args, original); callbacks.push(['before', h.renderer.getRenderTarget().tag]);
    h.renderer.setRenderTarget(b); draw(inner);
  };
  outer.material.onBeforeRender = function (renderer, gotScene, gotCamera, geometry, object, group) {
    same(this, outer.material); equal([renderer, gotScene, gotCamera, geometry, object, group], [h.renderer, scene, camera, outer.geometry, outer, null]);
    callbacks.push(['material', renderer.getRenderTarget().tag]);
  };
  outer.onAfterRender = function (...args) {
    same(this, outer); equal(args, original); callbacks.push(['after', h.renderer.getRenderTarget().tag]);
    h.renderer.setRenderTarget(c); draw(after);
  };
  h.renderer.setRenderTarget(a); draw(outer);
  equal(callbacks, [['before', 'a'], ['material', 'b'], ['after', 'b']]);
  equal(h.draws.map(d => [d.object, d.target, d.color]), [['inner', 'b', 0x0000ff], ['outer', 'b', 0xff0000], ['after', 'c', 0x00ff00]]);
  same(h.renderer.getRenderTarget(), c);
  records.push({ name: 'callbacks-do-not-restore-target', callbacks, draws: h.draws, finalTarget: 'c' });
  outer.onBeforeRender = function () { h.renderer.setRenderTarget(a); throw new Error('before failure'); };
  const count = h.draws.length; assertions++; assert.throws(() => draw(outer), /before failure/);
  same(h.draws.length, count); same(h.renderer.getRenderTarget(), a);
  records.push({ name: 'throw-keeps-target-switch', drawCountUnchanged: true, finalTarget: 'a' });
}
{
  const stateText = fs.readFileSync(path.join(pinned, 'src/renderers/webgl/WebGLState.js'), 'utf8');
  const stateBody = parse(stateText, { sourceType: 'module', ecmaVersion: 'latest' }).body
    .find(n => n.type === 'FunctionDeclaration' && n.id.name === 'WebGLState').body.body;
  const drawBuffers = stateBody.find(n => n.type === 'FunctionDeclaration' && n.id.name === 'drawBuffers');
  const recorded = [], framebuffer = {}, target = new WebGLRenderTarget(); let bound = framebuffer;
  const native = new Map(), gl = {
    COLOR_ATTACHMENT0: 0x8ce0, BACK: 0x0405,
    drawBuffers(values) { const copy = [...values]; native.set(bound, copy); recorded.push({ framebuffer: bound === null ? 'screen' : 'target', values: copy }); }
  };
  const sourceDrawBuffers = new Function('gl', `let currentDrawbuffers=new WeakMap(),defaultDrawbuffers=[];
    ${stateText.slice(drawBuffers.start, drawBuffers.end)};return drawBuffers;`)(gl);
  sourceDrawBuffers(target, framebuffer); equal(native.get(framebuffer), [gl.COLOR_ATTACHMENT0]);
  gl.drawBuffers([0]); bound = null; sourceDrawBuffers(null, null); equal(native.get(null), [gl.BACK]);
  bound = framebuffer; const calls = recorded.length; sourceDrawBuffers(target, framebuffer);
  same(recorded.length, calls); equal(native.get(framebuffer), [0]);
  gl.drawBuffers([gl.COLOR_ATTACHMENT0]); equal(native.get(framebuffer), [gl.COLOR_ATTACHMENT0]);
  bound = null; gl.drawBuffers([0]); const screenCalls = recorded.length; sourceDrawBuffers(null, null);
  same(recorded.length, screenCalls); equal(native.get(null), [0]); gl.drawBuffers([gl.BACK]);
  records.push({ name: 'native-drawBuffers-NONE-survives-source-cache', commands: recorded,
    targetRequiresNativeRestore: true, screenRequiresNativeRestore: true });
}
const files = ['core/RenderTarget.js', 'renderers/WebGLRenderTarget.js', 'renderers/WebGLRenderer.js', 'renderers/webgl/WebGLState.js'];
const hashes = Object.fromEntries(files.map(file => [file, crypto.createHash('sha256').update(fs.readFileSync(path.join(pinned, 'src', file))).digest('hex')]));
fs.mkdirSync(output, { recursive: true });
fs.writeFileSync(path.join(output, 'trace.json'), JSON.stringify({ pinned, hashes, assertions, recordingBoundaries: ['framebuffer allocation', 'state device methods', 'renderBufferDirect GPU command'], records }, null, 2));
console.log(JSON.stringify({ assertions, scenarios: records.length, output, evidence: 'pinned CPU lifecycle/protocol only; no GPU execution' }));
