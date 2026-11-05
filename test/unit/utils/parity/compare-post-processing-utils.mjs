import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);
const mod = await import(pathToFileURL(path.join(upstream, 'src/nodes/utils/PostProcessingUtils.js')).href);

// Only the export surface is compared. Every one of these is an Fn wrapper whose function length is
// the wrapper's, not the helper's, and calling them needs the upstream TSL graph, so this round
// verifies the names and that each is callable rather than the values they would compute. The
// numbers they produce are not claimed to be verified here.
const names = ['isBackgroundDepth', 'getViewPosition', 'getScreenPosition',
               'getScreenPositionFromClip', 'getNormalFromDepth',
               'interleavedGradientNoise', 'vogelDiskSample'];
const kinds = {};
for (const name of names) kinds[name] = typeof mod[name] === 'function' ? 'function' : 'other';
const expected = { names, kinds };
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.deepEqual(actual, expected);

// Upstream really exports these names and nothing beyond them is assumed here.
for (const name of names) assert.equal(typeof mod[name], 'function', name);
console.log('PostProcessingUtils export surface parity passed');
