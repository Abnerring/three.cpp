import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: IESSpotLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/IESSpotLightNode.js')).href);
// The light accessors key a weak map on the light, so a plain object stands in for a real light.
const light = {};
const node = new IESSpotLightNode(light);
const expected = {
  moduleSame: true,
  name: IESSpotLightNode.name,
  length: IESSpotLightNode.length,
  staticType: IESSpotLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  iesTextureIsNull: node._iesTextureNode === null,
  getSpotAttenuationLength: node.getSpotAttenuation.length,
  getSpotAttenuationName: node.getSpotAttenuation.name,
  updateLength: node.update.length,
  source: IESSpotLightNode.toString()
};
for (const field of ['coneCosNode', 'penumbraCosNode', 'cutoffDistanceNode', 'decayExponentNode', 'colorNode']) {
  expected[field + 'Type'] = node[field].type;
  expected[field + 'Group'] = node[field].groupNode.name;
}
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('IESSpotLightNode native/module parity passed');
