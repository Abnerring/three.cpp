import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const mod = await import(pathToFileURL(path.join(upstream, 'src/loaders/nodes/NodeLoader.js')).href);
const { DefaultLoadingManager } = await import(pathToFileURL(path.join(upstream, 'src/loaders/LoadingManager.js')).href);
const NodeLoader = mod.default;

const log = [];
const makeNodeClass = () => class TestNode {
  constructor() { this.uuid = ''; }
  deserialize(data) { log.push({ uuid: this.uuid, type: data.type, hadMeta: data.meta !== undefined }); }
};

const loader = new NodeLoader();
const managerIsDefault = loader.manager === DefaultLoadingManager;
const crossOrigin = loader.crossOrigin;
const withCredentials = loader.withCredentials;
const pathValue = loader.path;
const textures = {};
const setTexturesSame = loader.setTextures(textures) === loader;
const texturesStored = loader.textures === textures;

const registry = { TestNode: makeNodeClass() };
loader.setNodes(registry);
const fallback = loader.createNodeFromType('MissingNode');
const fallbackIsNode = fallback.isNode === true;
const fallbackType = fallback.nodeType;

const childJSON = { uuid: 'child-uuid', type: 'TestNode' };
const json = { uuid: 'root-uuid', type: 'TestNode', nodes: [ childJSON ] };
const node = loader.parse(json);
const parsedUuid = node.uuid;
const logLength = log.length;
const childHadMeta = log.length > 0 ? log[0].hadMeta : false;
const rootHadMeta = log.length > 1 ? log[1].hadMeta : false;
const jsonMetaRemoved = json.meta === undefined;
const childMetaRemoved = childJSON.meta === undefined;

const first = { uuid: 'a', type: 'TestNode' };
const second = { uuid: 'b', type: 'TestNode' };
const nodes = loader.parseNodes([ first, second ]);
const firstUuid = nodes.a.uuid;
const secondUuid = nodes.b.uuid;

const arityTable = {};
for (const name of [ 'load', 'parseNodes', 'parse', 'setTextures', 'setNodes',
  'createNodeFromType' ]) {
  arityTable[name] = loader[name].length;
}

const expected = {
  name: NodeLoader.name,
  length: NodeLoader.length,

  managerIsDefault,
  crossOrigin,
  withCredentials,
  path: pathValue,
  texturesIsObject: typeof loader.textures === 'object',
  nodesIsObject: typeof loader.nodes === 'object',
  setTexturesSame,
  texturesStored,
  fallbackIsNode,
  fallbackType,
  parsedUuid,
  logLength,
  childHadMeta,
  rootHadMeta,
  jsonMetaRemoved,
  childMetaRemoved,
  firstUuid,
  secondUuid,
  arityTable,
  source: NodeLoader.toString()
};
// The runtime prints TSL warnings on stdout before the JSON payload, so parse the last line and also assert
// that the duplicated chain registration warning is gone.
const stdout = execFileSync(path.resolve(probe), { encoding: 'utf8' });
assert.ok(!stdout.includes('Redefinition of method chaining'), 'duplicate chain registration warning');
const actual = JSON.parse(stdout.trim().split('\n').at(-1));
assert.deepEqual(actual, expected);
console.log('NodeLoader parity passed');
