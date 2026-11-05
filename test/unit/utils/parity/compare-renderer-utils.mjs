import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/renderers/common/RendererUtils.js')).href);

// The same stubs the probe builds: a renderer and a scene that record every call, so the differential
// compares the exact sequence of reads and writes the helpers perform.
const describe = (value) => {
  if (value === null) return 'null';
  if (typeof value === 'number') return String(value);
  if (typeof value === 'boolean') return value ? 'true' : 'false';
  if (typeof value === 'object' && value !== null && value.tag !== undefined) return String(value.tag);
  return '?';
};
const tagged = (tag) => ({ tag });
const log = [];
const record = (entry) => { log.push(entry); };
const renderer = { toneMapping: 2, toneMappingExposure: 1.5, outputColorSpace: 'srgb', autoClear: false };
const getter = (name, value) => { renderer[name] = () => { record(name); return value; }; };
const setter = (name) => {
  renderer[name] = (...args) => { record(name + '(' + args.map(describe).join(',') + ')'); };
};
getter('getRenderTarget', tagged('rt'));
getter('getActiveCubeFace', 1);
getter('getActiveMipmapLevel', 2);
getter('getRenderObjectFunction', tagged('rof'));
getter('getPixelRatio', 2);
getter('getMRT', tagged('mrt'));
getter('getClearAlpha', 0.25);
getter('getScissorTest', true);
renderer.getClearColor = (color) => {
  record('getClearColor');
  color.tag = 'cc'; color.r = 0.5; color.g = 0.25; color.b = 0.125;
  return color;
};
setter('setMRT');
setter('setRenderObjectFunction');
setter('setClearColor');
setter('setRenderTarget');
setter('setPixelRatio');
setter('setScissorTest');
const scene = { background: tagged('bg'), backgroundNode: tagged('bgn'), overrideMaterial: tagged('om') };

const serializeState = (state) => {
  const out = {};
  for (const key of ['toneMapping', 'toneMappingExposure', 'outputColorSpace', 'renderTarget',
    'activeCubeFace', 'activeMipmapLevel', 'renderObjectFunction', 'pixelRatio', 'mrt', 'clearAlpha',
    'autoClear', 'scissorTest', 'background', 'backgroundNode', 'overrideMaterial']) {
    out[key] = describe(state[key]);
  }
  out.clearColor = { r: state.clearColor.r, g: state.clearColor.g, b: state.clearColor.b };
  return out;
};
const takeLog = () => log.slice();
const clearLog = () => { log.length = 0; };

const savedState = mod.saveRendererAndSceneState(renderer, scene);
const savedFields = serializeState(savedState);
const saveCalls = takeLog();
clearLog();

const resetState = mod.resetRendererAndSceneState(renderer, scene, savedState);
const resetFields = serializeState(resetState);
const resetCalls = takeLog();
const afterReset = {
  autoClear: renderer.autoClear,
  sceneBackground: describe(scene.background),
  sceneBackgroundNode: describe(scene.backgroundNode),
  sceneOverrideMaterial: describe(scene.overrideMaterial)
};
clearLog();

mod.restoreRendererAndSceneState(renderer, scene, savedState);
const restoreCalls = takeLog();
const afterRestore = {
  toneMapping: renderer.toneMapping,
  toneMappingExposure: renderer.toneMappingExposure,
  outputColorSpace: renderer.outputColorSpace,
  autoClear: renderer.autoClear,
  sceneBackground: describe(scene.background),
  sceneOverrideMaterial: describe(scene.overrideMaterial)
};

const lengths = {};
for (const name of ['saveRendererState', 'resetRendererState', 'restoreRendererState',
  'saveSceneState', 'resetSceneState', 'restoreSceneState',
  'saveRendererAndSceneState', 'resetRendererAndSceneState', 'restoreRendererAndSceneState']) {
  lengths[name] = mod[name].length;
}

const expected = { lengths, savedFields, saveCalls, resetFields, resetCalls, afterReset, restoreCalls, afterRestore };
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('RendererUtils native/module parity passed');
