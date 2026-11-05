// Differential check for the native SceneUtils sortInstancedMesh port.
//
// The pinned module imports the bare "three" specifier and a sibling utils module by relative path, so the rewritten
// source is written to a temporary file beside the pinned one, imported and removed again.
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

const utilsDir = path.join(upstream, 'examples/jsm/utils');
const tempPath = path.join(utilsDir, '__parity_sort_instanced.mjs');
const sourcePath = path.join(utilsDir, 'SceneUtils.js');
fs.writeFileSync(tempPath, fs.readFileSync(sourcePath, 'utf8').replace(/from 'three'/g, `from '${threeUrl}'`));
let sortInstancedMesh;
try {
  ({ sortInstancedMesh } = await import(pathToFileURL(tempPath).href + '?v=' + Date.now()));
} finally {
  fs.rmSync(tempPath, { force: true });
}

const geometry = new THREE.BoxGeometry();
const material = new THREE.MeshBasicMaterial();
const instanced = new THREE.InstancedMesh(geometry, material, 3);
const matrix = new THREE.Matrix4();
matrix.makeTranslation(3, 0, 0);
instanced.setMatrixAt(0, matrix);
matrix.makeTranslation(1, 2, 0);
instanced.setMatrixAt(1, matrix);
matrix.makeTranslation(2, 1, 0);
instanced.setMatrixAt(2, matrix);

const positions = () => {
  const values = [];
  const current = new THREE.Matrix4();
  for (let i = 0; i < instanced.count; i += 1) {
    instanced.getMatrixAt(i, current);
    values.push(current.elements[12], current.elements[13], current.elements[14]);
  }
  return values;
};

const before = positions();
sortInstancedMesh(instanced, (a, b) => {
  const left = new THREE.Matrix4();
  const right = new THREE.Matrix4();
  instanced.getMatrixAt(a, left);
  instanced.getMatrixAt(b, right);
  return left.elements[12] - right.elements[12];
});
const after = positions();

const expected = { before, after };
const actual = JSON.parse(execFileSync(path.resolve(probe), { encoding: 'utf8' }));
assert.equal(actual.before.length, expected.before.length, 'before length');
assert.equal(actual.after.length, expected.after.length, 'after length');
for (const key of [ 'before', 'after' ]) {
  for (let i = 0; i < expected[key].length; i += 1) {
    assert.ok(Math.abs(actual[key][i] - expected[key][i]) < 1e-12, `${key}[${i}] ${actual[key][i]} vs ${expected[key][i]}`);
  }
}
console.log('sortInstancedMesh parity passed');
