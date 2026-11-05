import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: ProjectorLightNode } = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/ProjectorLightNode.js')).href);
const { Color } = await import(pathToFileURL(path.join(upstream, 'src/math/Color.js')).href);
const { EventDispatcher } = await import(pathToFileURL(path.join(upstream, 'src/core/EventDispatcher.js')).href);

// Install the same event protocol and shadow before construction as the native fixture.
const light = Object.assign(new EventDispatcher(), {
  color: new Color(0.25, 0.5, 0.75),
  intensity: 2,
  angle: 0.5,
  penumbra: 0.25,
  aspect: null,
  map: null,
  shadow: {}
});
const node = new ProjectorLightNode(light);
node.update({});
const expected = {
  moduleSame: true,
  name: ProjectorLightNode.name,
  length: ProjectorLightNode.length,
  staticType: ProjectorLightNode.type,
  type: node.type,
  isAnalyticLightNode: node.isAnalyticLightNode,
  shadowListenerRegistered: light.hasEventListener('dispose', node._shadowDisposeListener),
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
