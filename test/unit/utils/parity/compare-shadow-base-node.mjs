import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/ShadowBaseNode.js')).href);
const ShadowBaseNode = mod.default;

// The same harness the probe builds, expressed with plain objects.
const counting = (holder, key) => () => { holder[key] += 1; };
const harness = (counts, shadowType) => {
  const renderer = { shadowMap: { type: shadowType }, renderObject: counting(counts, 'render') };
  const shadow = { camera: {} };
  const scene = { overrideMaterial: null };
  const object = (castShadow, receiveShadow) => ({
    castShadow,
    receiveShadow,
    onBeforeShadow: counting(counts, 'before'),
    onAfterShadow: counting(counts, 'after')
  });
  return { renderer, shadow, scene, object };
};

const light = {};
const otherLight = {};
const node = new ShadowBaseNode(light);
const property = mod.shadowPositionWorld;

const material = node.getShadowMaterial();
const cachedMaterial = node.getShadowMaterial();
const otherMaterial = new ShadowBaseNode(otherLight).getShadowMaterial();
node.disposeShadowMaterial();
const rebuiltMaterial = node.getShadowMaterial();

const counts = { before: 0, render: 0, after: 0 };
const pcf = harness(counts, 1);
const fn = node.getShadowRenderObjectFunction(pcf.renderer, pcf.shadow);
const cachedFn = node.getShadowRenderObjectFunction(pcf.renderer, pcf.shadow);
const empty = () => ({});
const callWith = (callable, target) => callable(target, pcf.scene, empty(), empty(), empty(), empty(), empty(), empty(), empty());
callWith(fn, pcf.object(true, false));
const afterCaster = { before: counts.before, render: counts.render, after: counts.after };
callWith(fn, pcf.object(false, false));
const afterNonCaster = { before: counts.before, render: counts.render };

const vsmCounts = { before: 0, render: 0, after: 0 };
const vsm = harness(vsmCounts, 3);
const vsmFn = node.getShadowRenderObjectFunction(vsm.renderer, vsm.shadow);
vsmFn(vsm.object(false, true), vsm.scene, empty(), empty(), empty(), empty(), empty(), empty(), empty());
const afterVsm = { before: vsmCounts.before, render: vsmCounts.render, after: vsmCounts.after };

const expected = {
  name: ShadowBaseNode.name,
  length: ShadowBaseNode.length,
  staticType: ShadowBaseNode.type,
  instanceType: node.type,
  isShadowBaseNode: node.isShadowBaseNode,
  updateBeforeType: node.updateBeforeType,
  isCacheable: node.isCacheable(),
  propertyType: property.type,
  propertyIsNode: property.isNode,
  materialName: material.name,
  materialIsShadowPass: material.isShadowPassMaterial,
  materialBlending: material.blending,
  materialFog: material.fog,
  materialColorNodeIsNode: material.colorNode.isNode,
  materialCached: cachedMaterial === material,
  otherLightDistinct: otherMaterial !== material,
  rebuiltAfterDispose: rebuiltMaterial !== material,
  fnLength: fn.length,
  fnShadowType: fn.shadowType,
  fnCached: cachedFn === fn,
  afterCaster,
  afterNonCaster,
  vsmShadowType: vsmFn.shadowType,
  vsmDistinct: vsmFn !== fn,
  afterVsm,
  source: ShadowBaseNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ShadowBaseNode native/module parity passed');
