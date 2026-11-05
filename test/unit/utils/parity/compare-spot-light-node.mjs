import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: SpotLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/SpotLightNode.js')).href);
// The accessors key a weak map on the light, so a plain object stands in for a real light.
const light = {};
const node = new SpotLightNode(light);
const expected = {
  moduleSame: true,
  name: SpotLightNode.name,
  length: SpotLightNode.length,
  staticType: SpotLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  getSpotAttenuationLength: node.getSpotAttenuation.length,
  getSpotAttenuationName: node.getSpotAttenuation.name,
  getLightCoordLength: node.getLightCoord.length,
  setupDirectLength: node.setupDirect.length,
  source: SpotLightNode.toString()
};
for (const field of ['coneCosNode', 'penumbraCosNode', 'cutoffDistanceNode', 'decayExponentNode', 'colorNode']) {
  expected[field + 'Type'] = node[field].type;
  expected[field + 'Group'] = node[field].groupNode.name;
}
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('SpotLightNode native/module parity passed');
