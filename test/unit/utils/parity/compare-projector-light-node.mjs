import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: ProjectorLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/ProjectorLightNode.js')).href);
const { Color } = await import(pathToFileURL(path.join(upstream, 'src/math/Color.js')).href);

// A light the projector's update can read. The accessors key a weak map on it, so it is an object.
// The shadow container is attached only after construction: the upstream analytic constructor calls
// addEventListener on a light that already has a shadow, and this stand-in light has no dispatcher.
const light = {
  color: new Color(0.25, 0.5, 0.75),
  intensity: 2,
  angle: 0.5,
  penumbra: 0.25,
  aspect: null,
  map: null
};
const node = new ProjectorLightNode(light);
light.shadow = {};
node.update({});
const expected = {
  moduleSame: true,
  name: ProjectorLightNode.name,
  length: ProjectorLightNode.length,
  staticType: ProjectorLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  coneCosNodeValue: node.coneCosNode.value,
  penumbraCosNodeValue: node.penumbraCosNode.value,
  colorR: node.color.r,
  shadowAspect: light.shadow.aspect,
  getSpotAttenuationLength: node.getSpotAttenuation.length,
  updateLength: node.update.length,
  source: ProjectorLightNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ProjectorLightNode native/module parity passed');
