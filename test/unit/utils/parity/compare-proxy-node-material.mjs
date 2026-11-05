import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const { default: ProxyNodeMaterial } = await import(pathToFileURL(path.join(upstream, 'src/materials/nodes/ProxyNodeMaterial.js')).href);

// The same stub the probe builds.
const counters = { builds: 0 };
const cloneCounters = { cloned: 0 };
const makeNodeMaterial = (userData) => ({
  isNodeMaterial: true,
  uuid: 'node-uuid',
  version: 3,
  userData,
  colorNode: {},
  someValue: 42,
  build: () => { counters.builds += 1; }
});
const cloneable = { clone: () => { cloneCounters.cloned += 1; return { copy: true }; } };
const texture = { isTexture: true };
const userData = { cloneable, texture };

let rejectedPlain = false;
try {
  new ProxyNodeMaterial({});
} catch {
  rejectedPlain = true;
}

const nodeMaterial = makeNodeMaterial(userData);
const proxy = new ProxyNodeMaterial(nodeMaterial);

const beforeColor = proxy.colorNode === nodeMaterial.colorNode;
nodeMaterial.colorNode = {};
const afterColor = proxy.colorNode === nodeMaterial.colorNode;

const versionValue = proxy.version;
proxy.version = 99;
const versionAfterSet = proxy.version;

proxy.build({});

// These are observed before copy(), which replaces userData with a clone of the source user data.
const clonedHasCopy = proxy.userData.cloneable.copy;
const textureKeptByIdentity = proxy.userData.texture === texture;

const source = { uuid: 'source-uuid', version: 7, _listeners: {}, copied: 5, userData: {} };
proxy.copy(source);
const copiedValue = proxy.copied;
const uuidSkipped = proxy.uuid !== 'source-uuid';

const copy = proxy.clone();

const expected = {
  name: ProxyNodeMaterial.name,
  length: ProxyNodeMaterial.length,
  typeIsUndefined: ProxyNodeMaterial.type === undefined,
  rejectedPlain,
  proxyType: proxy.type,
  isProxyNodeMaterial: proxy.isProxyNodeMaterial,
  wrapsSame: proxy.nodeMaterial === nodeMaterial,
  someValue: proxy.someValue,
  colorBeforeChangeShows: beforeColor,
  colorAfterChangeShows: afterColor,
  version: versionValue,
  versionAfterSet,
  cacheKey: proxy.customProgramCacheKey(),
  builds: counters.builds,
  clonedCount: cloneCounters.cloned,
  clonedHasCopy,
  textureKeptByIdentity,
  copied: copiedValue,
  uuidSkipped,
  copyIsProxy: copy.isProxyNodeMaterial,
  copyWrapsSame: copy.nodeMaterial === nodeMaterial,
  copyIsNew: copy !== proxy
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ProxyNodeMaterial parity passed');
