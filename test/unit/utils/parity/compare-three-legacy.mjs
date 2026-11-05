import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

// The pinned legacy entry file is empty, so importing it yields a namespace with no exports.
const mod = await import(pathToFileURL(path.join(upstream, 'src/Three.Legacy.js')).href);
const ownKeys = Object.keys(mod);

const expected = {
  isObject: true,
  ownKeys: [],
  ownKeyCount: 0,
  upstreamKeyCount: ownKeys.length
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.equal(actual.ownKeyCount, expected.upstreamKeyCount);
assert.equal(actual.isObject, expected.isObject);
assert.deepEqual(actual.ownKeys, expected.ownKeys);
console.log('Three.Legacy parity passed');
