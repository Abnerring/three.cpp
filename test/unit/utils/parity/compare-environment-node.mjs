import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: EnvironmentNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/EnvironmentNode.js')).href);
const { vec3 } = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')).href);

// The same builder stand-in the probe builds: an environment node whose context method records the
// context objects and answers a real node, plus counters on the two assigned lighting terms.
const contexts = [];
const envNode = vec3(1, 1, 1);
envNode.context = (contextConfig) => { contexts.push(contextConfig); return envNode; };
const countingAssign = (holder) => () => { holder.count += 1; };
const radiance = { count: 0 };
radiance.addAssign = countingAssign(radiance);
const irradiance = { count: 0 };
irradiance.addAssign = countingAssign(irradiance);
const material = { useAnisotropy: false, anisotropy: 0, useRetroreflection: false, retroreflectivity: 0 };
const renderer = {};
const builder = { material, context: { radiance, iblIrradiance: irradiance, lightingModel: {} }, renderer };

const node = new EnvironmentNode(envNode);
node.setup(builder);

const radianceContext = contexts[0];
const firstUv = radianceContext.getUV();
const secondUv = radianceContext.getUV();
const irradianceContext = contexts[1];

const cacheA = node._getPMREMNodeCache(renderer);
const cacheB = node._getPMREMNodeCache(renderer);
const cacheC = node._getPMREMNodeCache({});

const expected = {
  name: EnvironmentNode.name,
  length: EnvironmentNode.length,
  staticType: EnvironmentNode.type,
  instanceType: node.type,
  isLightingNode: node.isLightingNode,
  setupLength: node.setup.length,
  cacheLength: node._getPMREMNodeCache.length,
  radianceAssigns: radiance.count,
  irradianceAssigns: irradiance.count,
  contextCount: contexts.length,
  getUvIsNode: firstUv.isNode === true,
  getUvStable: firstUv === secondUv,
  radianceLevelIsNode: radianceContext.getTextureLevel().isNode === true,
  irradianceLevelIsNode: irradianceContext.getTextureLevel().isNode === true,
  cacheStable: cacheA === cacheB,
  cachePerRenderer: cacheA !== cacheC,
  source: EnvironmentNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('EnvironmentNode native/module parity passed');
