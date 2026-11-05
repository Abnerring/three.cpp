// Differential check for the native GeometryUtils port.
//
// The pinned module imports the bare "three" specifier, so its source is loaded with that import rewritten to the
// pinned build. Every generated point is compared, for the default and for a custom corner permutation, and the
// Gosper path is compared for two sizes.
import assert from 'node:assert/strict';
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const [ upstream, probe ] = process.argv.slice(2);
const lock = (await import('../../../../upstream.json', { with: { type: 'json' } })).default;
assert.equal(execFileSync('git', [ '-C', upstream, 'rev-parse', 'HEAD' ], { encoding: 'utf8' }).trim(), lock.commit);

const threeUrl = pathToFileURL(path.join(upstream, 'build/three.module.js')).href;
const THREE = await import(threeUrl);
const sourcePath = path.join(upstream, 'examples/jsm/utils/GeometryUtils.js');
const rewritten = fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`);
const moduleUrl = `data:text/javascript;base64,${Buffer.from(rewritten, 'utf8').toString('base64')}`;
const { hilbert2D, hilbert3D, gosper } = await import(moduleUrl);

const flatten = (points) => points.flatMap((point) => [ point.x, point.y, point.z ]);

const expected = {
  hilbert2DOne: flatten(hilbert2D()),
  hilbert2DTwo: flatten(hilbert2D(new THREE.Vector3(0, 0, 0), 10, 2)),
  hilbert2DCustom: flatten(hilbert2D(new THREE.Vector3(1, 2, 3), 4, 1, 3, 2, 1, 0)),
  hilbert3DOne: flatten(hilbert3D()),
  hilbert3DCustom: flatten(hilbert3D(new THREE.Vector3(1, 2, 3), 4, 1, 3, 2, 1, 0, 7, 6, 5, 4)),
  gosperOne: gosper(1),
  gosperTwo: gosper(2)
};
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
for (const key of Object.keys(expected)) {
  const e = expected[key];
  const a = actual[key];
  assert.equal(a.length, e.length, `${key} length`);
  for (let i = 0; i < e.length; i += 1) {
    assert.ok(Math.abs(a[i] - e[i]) < 1e-12, `${key}[${i}] ${a[i]} vs ${e[i]}`);
  }
}
console.log('GeometryUtils parity passed');
