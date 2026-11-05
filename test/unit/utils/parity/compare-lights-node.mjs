import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/LightsNode.js')).href);
const LightsNode = mod.default;
const lights = mod.lights;

// The same stand-ins the probe builds: a builder that answers one persistent data object and an empty
// material lighting list, and lights that are already nodes so no light node class is constructed.
const makeBuilder = (data) => ({
  context: { materialLightings: [] },
  getDataFromNode: () => data
});
const nodeLight = (id, hash) => ({ isNode: true, id, getHash: () => hash });

const node = new LightsNode();
const other = new LightsNode();
const list = [{}];
const returned = node.setLights(list);

const keyNode = new LightsNode();
keyNode.setLights([
  { id: 7, castShadow: false },
  { id: 3, castShadow: true, isSpotLight: true, map: null, colorNode: null }
]);
const cacheKey = keyNode.customCacheKey();

const hashNode = new LightsNode();
hashNode.setLights([nodeLight(2, 'b'), nodeLight(1, 'a')]);
const data = {};
const builder = makeBuilder(data);
const hash = hashNode.getHash(builder);
const lightNodes = hashNode.getLightNodes(builder);

const built = lights(list);
const empty = lights();

const expected = {
  name: LightsNode.name,
  length: LightsNode.length,
  staticType: LightsNode.type,
  instanceType: node.type,
  global: node.global,
  emptyLightsLength: node._lights.length,
  sharedTotalDiffuse: node.totalDiffuseNode === other.totalDiffuseNode,
  sharedTotalSpecular: node.totalSpecularNode === other.totalSpecularNode,
  sharedOutgoingLight: node.outgoingLightNode === other.outgoingLightNode,
  isCacheable: node.isCacheable(),
  setLightsReturnsThis: returned === node,
  getLightsIsSameArray: node.getLights() === list,
  builtinIsSameArray: node.getBuiltinLights() === list,
  hasLights: node.hasLights,
  cacheKey,
  hash,
  hashCached: data.lightNodesHash,
  lightNodeCount: lightNodes.length,
  firstLightNodeId: lightNodes[0].id,
  factoryName: lights.name,
  factoryLength: lights.length,
  factoryLightsIsSameArray: built.getLights() === list,
  factoryEmptyLength: empty.getLights().length,
  setupLength: node.setup.length,
  source: LightsNode.toString()
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('LightsNode native/module parity passed');
