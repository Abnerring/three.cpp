import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/functions/BSDF/LTC.js')).href);
const { vec3, float } = await import(pathToFileURL(path.join(upstream, 'src/nodes/tsl/TSLBase.js')).href);

// The export surface is compared, and LTC_Uv is executed so its returned node can be compared too.
// The two evaluators are guarded by If, which needs a current stack, so they are not executed; no
// numeric values are compared for them and none are claimed.
const names = ['LTC_Uv', 'LTC_Evaluate', 'LTC_Evaluate_Volume'];
const kinds = {};
for (const name of names) kinds[name] = typeof mod[name] === 'function' ? 'function' : 'other';

const uv = mod.LTC_Uv({ N: vec3(0, 0, 1), V: vec3(0, 0, 1), roughness: float(0.5) });
const expected = {
  names,
  kinds,
  uvIsNode: uv.isNode === true,
  uvType: uv.type === undefined ? 'undefined' : String(uv.type)
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);
console.log('LTC export surface and LTC_Uv node parity passed');