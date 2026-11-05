import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/PointLightNode.js')).href);
const PointLightNode = mod.default;
const directPointLight = mod.directPointLight;
const { Color } = await import(pathToFileURL(path.join(upstream, 'src/math/Color.js')).href);

// The accessors key a weak map on the light, so it is a plain object. No shadow container is
// attached, because the analytic constructor would call addEventListener on it for one.
const light = { color: new Color(0.25, 0.5, 0.75), intensity: 2, distance: 7, decay: 1.5 };
const node = new PointLightNode(light);
const cutoffBefore = node.cutoffDistanceNode.value;
const decayBefore = node.decayExponentNode.value;
node.update({});
const expected = {
  moduleSame: true,
  name: PointLightNode.name,
  length: PointLightNode.length,
  staticType: PointLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  cutoffBefore,
  decayBefore,
  cutoffAfter: node.cutoffDistanceNode.value,
  decayAfter: node.decayExponentNode.value,
  colorR: node.color.r,
  colorG: node.color.g,
  colorB: node.color.b,
  factoryName: directPointLight.name,
  factoryLength: directPointLight.length,
  setupDirectLength: node.setupDirect.length,
  updateLength: node.update.length,
  source: PointLightNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('PointLightNode native/module parity passed');
