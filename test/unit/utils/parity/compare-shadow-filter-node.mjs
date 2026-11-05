import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/lighting/ShadowFilterNode.js')).href);

// Only the export surface is compared: the three names and that each is callable.
//
// The wrapper's function length is deliberately not compared. Upstream these exports are Fn results
// whose length property is not the helper's parameter count at all, and this runtime's Fn proxy does
// not expose one, so comparing it would compare two unrelated things. The filters themselves need a
// real depth texture and, for VSM, a builder, and the node graphs they build are not evaluated here,
// so no values are claimed.
const names = ['BasicShadowFilter', 'PCFShadowFilter', 'VSMShadowFilter'];
const kinds = {};
for (const name of names) {
  kinds[name] = typeof mod[name] === 'function' ? 'function' : 'other';
}
const expected = { names, kinds };
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('ShadowFilterNode export surface parity passed');
